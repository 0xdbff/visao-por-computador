cmake_minimum_required(VERSION 3.10)
project(test_project LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS_DEBUG "-g -Wall -pedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -pedantic")
set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Set Debug mode as default #!TODO change
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

file(GLOB SOURCES "*.c" "*.cpp")
add_executable(main ${SOURCES})

target_link_libraries(main m)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_definitions(main PRIVATE DEBUG)
endif()

target_compile_definitions(main PRIVATE ALIGN_MEM)
