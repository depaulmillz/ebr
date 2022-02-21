from conans import ConanFile, CMake

class EBRConan(ConanFile):
    name = "ebr"
    version = "0.0.1"
    author = "dePaul Miller"
    url = "https://github.com/depaulmillz/ebr"
    settings={"os" : ["Linux"], "compiler" : None, "build_type" : None, "arch": ["x86_64"] }
    generators="cmake"
    license="MIT"

    exports_sources = "CMakeLists.txt", "cmake/*", "include/*", "test/*", "LICENSE"

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.definitions["USING_CONAN"] = "ON"
        cmake.definitions["CMAKE_EXPORT_COMPILE_COMMANDS"] = "ON"
        cmake.configure()
        return cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()
        cmake.test()

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()

    def package_id(self):
        self.info.header_only()

    def package_info(self):
        self.cpp_info.names["cmake_find_package"] = "ebr"
        self.cpp_info.names["cmake_find_package_multi"] = "ebr"
