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
# aux_source_directory(include HEADER_FILES)
file(GLOB HEADER_FILES "include/*.h")
# message(STATUS "HEADER_FILES： ${HEADER_FILES}")
# message(STATUS "PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")

add_executable(ConcurrentMemoryPoolTest ${SOURCES} src/ConcurrentMemoryPoolTest.cpp )
message(STATUS "生成可执行文件")

# 链接库路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
# set(HEADERFILE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/include)
set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/install)

# 生成动态链接库
add_library(ConcurrentAlloc SHARED ${SOURCES} ${HEADER_FILES})

# 生成同名的静态链接库，先别名
add_library(ConcurrentAlloc_static STATIC ${SOURCES} ${HEADER_FILES})
# 指定静态库的输出名称
set_target_properties(ConcurrentAlloc_static PROPERTIES OUTPUT_NAME "ConcurrentAlloc")
# 使静态库和动态库同时存在
set_target_properties(ConcurrentAlloc PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(ConcurrentAlloc_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)

# 指定动态库版本,VERSION 动态库版本,SOVERSION API版本
set_target_properties(ConcurrentAlloc PROPERTIES VERSION 1.0 SOVERSION 1)
message(STATUS "生成动态链接库和静态链接库")

# 安装动态链接库和静态链接库，cmake构建时使用-DCMAKE_INSTALL_PREFIX=<prefix>来指定安装路径
# 动态库安装到<prefix>/lib目录下，静态库安装到<prefix>/lib目录下
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    install (TARGETS ConcurrentAlloc ConcurrentAlloc_static LIBRARY DESTINATION lib ARCHIVE DESTINATION lib/static)
else () # windows
    install (TARGETS ConcurrentAlloc ConcurrentAlloc_static LIBRARY ARCHIVE)
endif() 

# 安装头文件, 安装到<prefix>/include目录下
install (FILES ${HEADER_FILES} TYPE INCLUDE)
# message(STATUS "安装完成")

# windows下构建: cmake -G "MinGW Makefiles" ..
# MinGW Makefiles指定编译器