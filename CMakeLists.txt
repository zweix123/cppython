cmake_minimum_required(VERSION 3.16.0)

set(CMAKE_CXX_STANDARD 20)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
    message("Setting default build type to Release")
endif()

project(act VERSION 0.0.1 LANGUAGES C CXX)

find_package(Python REQUIRED COMPONENTS Interpreter Development)
include_directories(${Python_INCLUDE_DIRS})
link_directories(${Python_LIBRARY_DIRS})
add_compile_options(${Python_DEFINITIONS})

include(etc/c_python.cmake)

include_directories(${PROJECT_SOURCE_DIR}/include)
add_subdirectory(./src)
add_subdirectory(./test)