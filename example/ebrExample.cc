#include <EBR.h>
#include <atomic>
#include <ratio>
#include <thread>
#include <iostream>

struct Destructor {
    void operator()(int* x) {
        std::cerr << "Deleted pointer " << std::endl;
        delete x;
    }
};

int main() {
    using namespace ebr;   
    using namespace std::chrono_literals;

    EBR<int, Destructor> ebr;

    std::atomic_bool signalA{false};
    std::atomic_bool signalB{false};

    int* pointer = new int(100);

    std::thread t([&](){
            ebr.registerThread();
            signalB.store(true);
            while(!signalA);
            ebr.start();
            ebr.free(pointer);
            ebr.end();
            while(signalA);
            ebr.start();
            ebr.end();
    });

    ebr.registerThread();
    ebr.start();
    signalA.store(true);
    while(!signalB);
    std::this_thread::sleep_for(1s); 

    if(*pointer != 100) {
        return 1;
    }

    ebr.end();


    for(int i = 0; i < 10; i++) {
        ebr.start();

        if(*pointer != 100) {
            return 1;
        }

        ebr.end();
    }

    signalA = false;
    t.join();

    return 0;

}
