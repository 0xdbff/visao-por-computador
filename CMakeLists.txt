cmake_minimum_required(VERSION 3.10)
project(test_project LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -pedantic")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# Add your include directory to the include path
include_directories(include)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# Include all cpp files within the src directory
file(GLOB SOURCES "src/*.cpp")

add_executable(main ${SOURCES})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_definitions(main PRIVATE DEBUG)
endif()

target_link_libraries(main ${OpenCV_LIBS})
