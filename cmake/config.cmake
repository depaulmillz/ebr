macro(SETUP_CONAN)
    message(STATUS "Running conan install ${CMAKE_SOURCE_DIR} -if ${CMAKE_BINARY_DIR} --build=missing")
    if(NOT USING_CONAN)
        execute_process(
            COMMAND conan install -s compiler.libcxx=libstdc++11 -s build_type=${CMAKE_BUILD_TYPE} ${CMAKE_SOURCE_DIR} -if ${CMAKE_BINARY_DIR} --build=missing
            RESULT_VARIABLE conan_code)
        if(NOT "${conan_code}" STREQUAL "0")
            message(FATAL_ERROR "Conan failed ${conan_code}")
        endif()
    endif()
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(TARGETS)
endmacro()
