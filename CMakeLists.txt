cmake_minimum_required(VERSION 3.18)

project(EBR LANGUAGES CXX)

option(USING_CONAN "Set if building through the conan script" OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
    message(STATUS "SET CMAKE_BUILD_TYPE to Release")
endif()

include(cmake/config.cmake)

set_consistent_standard(11)
setup_conan()

enable_testing()

add_library(ebr INTERFACE)
target_link_libraries(ebr INTERFACE pthread)
target_include_directories(ebr INTERFACE 
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION include)

add_executable(example example/ebrExample.cc)
target_link_libraries(example PRIVATE ebr)

install(TARGETS ebr
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include)

set(CPACK_PACKAGE_VENDOR "dePaul Miller et. al.")
SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "dePaul Miller")
set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
set(CPACK_SOURCE_IGNORE_FILES
        /.git
        /dist
        /.*build.*
        /\\\\.DS_Store
        )
include(CPack)
