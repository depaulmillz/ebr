#include <cassert>
#include <mutex>
#include <list>
#include <atomic>
#include <variant>
#define _UNIX03_THREADS
#include <pthread.h>
#include <stdexcept>
#include <thread>

extern "C" struct EBRThreadData {
    std::atomic_uint64_t ts;
    std::atomic_bool active;

    std::list<void*> freeLists[3] = {};
    uint64_t epoch[3] = {};
    int workingOn = 4;
};

template<typename T>
void destructor(void* x) {
    delete reinterpret_cast<T*>(x);
}

template<typename T>
class ThreadSpecificData {
public:
    explicit ThreadSpecificData() {
        void (*fn)(void*) = destructor<T>;
        if(pthread_key_create(&key, fn) < 0) {
            perror("Error on pthread_key_create");
            throw std::runtime_error("Error on pthread_key_create");
        }
    }
    
    /// Does nothing to keep the thread storage alive
    ~ThreadSpecificData() {}

    void set(T* x) {
        if(pthread_setspecific(key, reinterpret_cast<void*>(x)) != 0) {
            perror("Error on pthread_setspecific");
            throw std::runtime_error("Error on pthread_setspecific");
        }
    }

    T* get() {
        T* x = reinterpret_cast<T*>(pthread_getspecific(key));
        return x;
    }
private:
    pthread_key_t key;
};

template<typename T, typename Destructor>
class EBR {
public:

    EBR() : gts{0}, threadList{} {}
    
    ~EBR() {}

    void registerThread() {
        if(key.get() == nullptr) {
            auto data = new EBRThreadData();
            data->active = false;
            key.set(data);
            std::unique_lock<std::mutex> lg(listGuard);
            threadList.push_front(data);
        }
    }

    void start() {
        auto data = key.get();
        assert(data->workingOn == 4);
        int clean;
        clean = this->getClean();
        while(clean == 4) {
            std::this_thread::yield();
            clean = this->getClean();
        }
        data->workingOn = clean;
        data->ts = gts.load(std::memory_order_relaxed);
        data->active.store(true, std::memory_order_seq_cst);
    }

    void free(T* x) {
        auto data = key.get();
        int workingOn = data->workingOn;
        assert(workingOn != 4);
        data->freeLists[workingOn].push_front(x);
    }

    void end() {
        auto data = key.get();
        data->workingOn = 4;
        data->active.store(false, std::memory_order_seq_cst);
    }

private:

    int getClean() {

        auto data = key.get();

        auto canFree = fts.load();
        auto threadMinEpoch = data->epoch[0];
        
        for(int i = 1; i < 3; i++) {
            if(canFree >= data->epoch[i]) {
                for(auto& elm : data->freeLists[i]) {
                    Destructor{}(reinterpret_cast<T*>(elm)); 
                }
                data->freeLists[i] = {};
                return i;
            }
            if(threadMinEpoch > data->epoch[i]) {
                threadMinEpoch = data->epoch[i];
            }
        }

        std::unique_lock<std::mutex> lg(listGuard);
        
        // what we can free may be updated
        canFree = fts.load();
        
        if(threadMinEpoch <= canFree) {
            int i = freeWhatYouCan(canFree, data);
            if(i < 4) return i; 
        }


        auto startGts = gts.load();
        uint64_t min = startGts - 1;
        bool set = false;
        for(auto& e : threadList) {
            if(e->active) {
                auto ts = e->ts.load();
                if(!set) {
                    min = ts;
                    set = true;
                } else if(ts < min) {
                    min = ts;
                }
            }
        }

        fts.store(min - 2);  // we can free min - 2 and less
        gts++;
        lg.unlock();

        canFree = min - 2;
        if(threadMinEpoch <= canFree) {
            int i = freeWhatYouCan(canFree, data);
            if(i < 4) return i;
        }
        return 4;
    }

    inline int freeWhatYouCan(uint64_t canFree, EBRThreadData* data) {
        for(int i = 1; i < 3; i++) {
            if(canFree >= data->epoch[i]) {
                for(auto& elm : data->freeLists[i]) {
                    Destructor{}(reinterpret_cast<T*>(elm)); 
                }
                data->freeLists[i] = {};
                return i;
            }
        }
        return 4;
    }


    /// Global time stamp
    std::atomic_uint64_t gts;
    /// Free time stamp
    std::atomic_uint64_t fts;
    std::mutex listGuard; 
    std::list<EBRThreadData*> threadList;
    ThreadSpecificData<EBRThreadData> key;
};


