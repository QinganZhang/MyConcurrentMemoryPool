#pragma once

#include <iostream>
#include <vector>
#include <list>

using std::cout;
using std::endl;

static const size_t PAGE_SHIFT = 12; // 一页定义为2^PAGE_SHIFT KB大小

// 每个定长内存块的前4个字节或者前8个字节作为next指针，存储下一个定长内存块的地址
#ifdef _WIN32
    typedef unsigned long long PAGE_ID;
#else  // _WIN64 || linux
    typedef unsigned long long PAGE_ID;
#endif

// 宏WIN32, _WIN32, _WIN64的介绍：https://blog.csdn.net/wangkui1331/article/details/103491790
#if defined(_WIN32) || defined(_WIN64)
    #include <Windows.h>
#else
    #include <unistd.h>
    #include <sys/mman.h>
#endif

// 直接去堆上按页申请内存
inline static void* SystemAlloc(size_t kpage){
#if defined(_WIN32) || defined(_WIN64)
    // VirtualAlloc用于在进程的虚拟地址空间中分配动态内存
    void *ptr = VirtualAlloc(NULL, kpage << PAGE_SHIFT, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else 
    // linux下mmap
    void* ptr = mmap(NULL, kpage << PAGE_SHIFT, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
#endif
    if(ptr == nullptr) 
        throw std::bad_alloc();
    else return ptr;
}

// 直接将内存还给堆
inline static void SystemFree(void *ptr, size_t kpage = 0){
#if defined(_WIN32) || defined(_WIN64)
    VirtualFree(ptr, 0, MEM_RELEASE); 
#else 
    munmap(ptr, kpage << PAGE_SHIFT);
#endif
}

// 让一个指针在32位平台下解引用后能向后访问4个字节，在64位平台下则可以访问8个字节
static void*& NextObj(void* ptr){
    return (*(void**)ptr);
    // 将ptr先强转为二级指针（原来ptr是一级指针，现在则视为ptr指向的内存地址中保存的也是一个地址），然后再解引用
}

