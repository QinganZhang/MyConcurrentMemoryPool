#pragma once

#include "Common.h"

class ThreadCache{
public:
    // 申请内存
    void* Allocate(size_t size);
    
    // 释放对象，回收内存
    void Deallocate(void* ptr, size_t size);

    // 从CentralCache中申请内存
    void* FetchFromCentralCache(size_t index, size_t size);
    
    // 回收内存累计到一定大小时，将多余的内存归还到CentralCache
    void ReturnBackToCentralCache(FreeList& list, size_t size);

private:
    FreeList _freeLists[N_FREE_LISTS]; // 哈希桶，即一个数组，数组中每个元素都是一个链表
};

// 使用线程局部存储(TLS, Thread Local Storage)，使用该方法存储的变量在它所在的线程中是全局可访问的，但是不能被其他线程访问到
static thread_local ThreadCache* pTLSThreadCache = nullptr;
// #if defined(_WIN32) || defined(_WIN64)
//     // static __declspec(thread) ThreadCache* pTLSThreadCache = nullptr;
//     static thread_local ThreadCache* pTLSThreadCache = nullptr;
// #else 
//     static __thread ThreadCache* pTLSThreadCache = nullptr;
// #endif

