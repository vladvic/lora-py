cmake_minimum_required(VERSION 3.5)
project(lora-server)

include(ExternalProject)

set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${PATCH_NUMBER}")

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_CXX_STANDARD 11)

set(VERSION_MAJOR "0")
set(VERSION_MINOR "1")
set(PATCH_NUMBER  "0")

set(PYTHON_VERSION "" CACHE STRING "Python libraries version")

if(PYTHON_VERSION)
  unset(PYTHON_INCLUDE_DIR CACHE)
  unset(PYTHON_LIBRARY CACHE)
endif(PYTHON_VERSION)

set(Python_ADDITIONAL_VERSIONS ${PYTHON_VERSION})

find_package(Threads REQUIRED)
find_package(PythonLibs REQUIRED)

if(NOT PYTHON_VERSION)
  string(REGEX REPLACE "^.*python" "" PYTHON_VERSION ${PYTHON_LIBRARY})
  string(REGEX REPLACE "\\.[^\\.]*$" "" PYTHON_VERSION ${PYTHON_VERSION})
endif(NOT PYTHON_VERSION)

message(STATUS "Python version: ${PYTHON_VERSION}")
message(STATUS "Found Python includes: ${PYTHON_INCLUDE_DIR}")
message(STATUS "Found Python libraries: ${PYTHON_LIBRARY}")

#Setup CMake to run tests
enable_testing()

set(LINK_LIBS -lrt)

#compiler warnings settings 
add_compile_options(-Wall -Wno-pragmas -Wno-braced-scalar-init -Wno-narrowing)
#compiler optimization flags
add_compile_options(-g -ffinite-math-only -fno-trapping-math -O3 -march=native -msse4.2 -mpclmul)
if(CMAKE_COMPILER_IS_GNUCXX)
  # gcc specific flags
  add_compile_options(-fno-signaling-nans)
endif()

add_definitions(-DFORWARD_EXCEPTIONS -DTBB_USE_DEBUG=false -DTBB_USE_ASSERT=false)

include_directories(${PROJECT_SOURCE_DIR}/src/ ${PYTHON_INCLUDE_DIR})

file(GLOB_RECURSE SEMTECH_SOURCES ${PROJECT_SOURCE_DIR}/src/semtech/*.cpp)
file(GLOB_RECURSE NETWORK_SOURCES ${PROJECT_SOURCE_DIR}/src/net/*.cpp)
file(GLOB_RECURSE LORAWAN_SOURCES ${PROJECT_SOURCE_DIR}/src/lora/**/*.cpp)
file(GLOB_RECURSE LORAWAN_C_SOURCES ${PROJECT_SOURCE_DIR}/src/lora/**/*.c)
file(GLOB_RECURSE COMMON_SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)

set(SOURCES ${SEMTECH_SOURCES} ${NETWORK_SOURCES} ${LORAWAN_SOURCES} ${LORAWAN_C_SOURCES} ${COMMON_SOURCES})
add_executable(lora-server ${SOURCES})
target_link_libraries(lora-server ${PYTHON_LIBRARY})

