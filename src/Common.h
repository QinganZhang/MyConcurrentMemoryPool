#pragma once

#include <iostream>
#include <vector>

using std::cout;
using std::endl;

static const size_t PAGE_SHIFT = 13; // 一页定义为2^PAGE_SHIFT KB大小

// 每个定长内存块的前4个字节或者前8个字节作为next指针，存储下一个定长内存块的地址
#ifdef _WIN64
    typedef unsigned long long PAGE_ID;
#elif _WIN32
    typedef size_t PAGE_ID;
#else 
    // linux
#endif

// 宏WIN32, _WIN32, _WIN64的介绍：https://blog.csdn.net/wangkui1331/article/details/103491790
#ifdef _WIN32
    #include <Windows.h>
#else
    // 
#endif

// 直接去堆上按页申请内存
inline static void* SystemAlloc(size_t kpage){
#ifdef _WIN32
    void *ptr = VirtualAlloc(NULL, kpage << PAGE_SHIFT, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    // VirtualAlloc用于在进程的虚拟地址空间中分配动态内存
#else 
    // linux下brk mmap等
#endif
    if(ptr == nullptr) 
        throw std::bad_alloc();
    else return ptr;
}

// 直接将内存还给堆
inline static void SystemFree(void *ptr){
#ifdef _WIN32
    VirtualFree(ptr, 0, MEM_RELEASE);
#else 
    // linux下sbrk unmmap等
#endif
}

// 让一个指针在32位平台下解引用后能向后访问4个字节，在64位平台下则可以访问8个字节
static void*& NextObj(void* ptr){
    return (*(void**)ptr);
    // 将ptr先强转为二级指针（原来ptr是一级指针，现在则视为ptr指向的内存地址中保存的也是一个地址），然后再解引用
}

