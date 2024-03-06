cmake_minimum_required (VERSION 2.8)
project (MyConcurrentMemoryPool)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-g")
set(CMAKE_C_FLAGS "-g")
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

include_directories(${PROJECT_SOURCE_DIR}/include/)

# aux_source_directory(${PROJECT_SOURCE_DIR}/src SOURCE_DIR)
# add_executable(${PROJECT_NAME} ${SOURCE_DIR})

set(SOURCES 
    src/ConcurrentMemoryPoolTest.cpp 
    src/ThreadCache.cpp
    src/CentralCache.cpp
    src/PageCache.cpp
    )
add_executable(ConcurrentMemoryPoolTest ${SOURCES})

# under windows: cmake -G "MinGW Makefiles" ..