cmake_minimum_required (VERSION 3.22)
project (MyConcurrentMemoryPool)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-g")
set(CMAKE_C_FLAGS "-g")

include_directories(${PROJECT_SOURCE_DIR}/include/)

set(SOURCES 
    src/ThreadCache.cpp
    src/CentralCache.cpp
    src/PageCache.cpp
)
aux_source_directory(include/ HEADER_FILES)

add_executable(ConcurrentMemoryPoolTest ${SOURCES} src/ConcurrentMemoryPoolTest.cpp )

# 链接库路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 生成动态链接库
add_library(MyConcurrentMemoryPool SHARED ${SOURCES} ${HEADER_FILES})

# 生成同名的静态链接库，先别名
add_library(MyConcurrentMemoryPool_static STATIC ${SOURCES} ${HEADER_FILES})
# 指定静态库的输出名称
set_target_properties(MyConcurrentMemoryPool_static PROPERTIES OUTPUT_NAME "MyConcurrentMemoryPool")
# 使静态库和动态库同时存在
set_target_properties(MyConcurrentMemoryPool PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(MyConcurrentMemoryPool_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)

# 指定动态库版本,VERSION 动态库版本,SOVERSION API版本
set_target_properties(MyConcurrentMemoryPool PROPERTIES VERSION 1.0 SOVERSION 1)
 
# 将动态库与动态库同时安装到lib目录中
# install (TARGETS MyConcurrentMemoryPool MyConcurrentMemoryPool_static DESTINATION ${LIBRARY_OUTPUT_PATH})


# windows下构建: cmake -G "MinGW Makefiles" ..
# MinGW Makefiles指定编译器