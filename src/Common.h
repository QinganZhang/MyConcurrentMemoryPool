#pragma once

#include <iostream>
#include <vector>
#include <list>
#include <assert.h>
#include <time.h>

using std::cout;
using std::endl;

static const size_t PAGE_SHIFT = 12; // 一页定义为2^PAGE_SHIFT KB大小
static const size_t MAX_BYTES = 256 * 1024; // 申请内存大小<=MAX_BYTES，找threadCache申请，否则找pageCache申请
static const size_t N_FREE_LISTS = 208; // threadCache哈希桶的个数
static const size_t N_PAGES = 129; // pageCache哈希桶的个数

// 每个定长内存块的前4个字节或者前8个字节作为next指针，存储下一个定长内存块的地址
#ifdef _WIN32
    typedef unsigned long long PAGE_ID;
#else  // _WIN64 || linux
    typedef unsigned long long PAGE_ID;
#endif

// 宏WIN32, _WIN32, _WIN64的介绍：https://blog.csdn.net/wangkui1331/article/details/103491790
#if defined(_WIN32) || defined(_WIN64)
    #include <Windows.h>
    #include <thread>
#else
    #include <unistd.h>
    #include <sys/mman.h>
    #include <thread.h>
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
static void*& NextObj(void* ptr){ // 如果不加引用，返回的是右值
    return (*(void**)ptr);
    // 将ptr先强转为二级指针（原来ptr是一级指针，现在则视为ptr指向的内存地址中保存的也是一个地址），然后再解引用
}

// thread cache中的每个哈希桶对应的链表
class FreeList{
public:
    // 将一块小内存头插到对应链表
    void Push(void* obj){ 
        assert(obj);
        NextObj(obj) = _freeList;
        _freeList = obj;
    }
    // 从对应链表中获取一块小内存
    void* Pop(){
        assert(_freeList);
        void* obj = _freeList;
        _freeList = NextObj(_freeList);
        return obj;
    }
    bool Empty() {
        return _freeList == nullptr;
    }
private:
    void* _freeList = nullptr;
};

// thread cache中哈希桶的映射规则
class SizeClass{
public:
    // 获取向上对齐后的字节数
    static inline size_t RoundUp(size_t bytes){
        if (bytes <= 128)
        {
            return _RoundUp(bytes, 8);
        }
        else if (bytes <= 1024)
        {
            return _RoundUp(bytes, 16);
        }
        else if (bytes <= 8 * 1024)
        {
            return _RoundUp(bytes, 128);
        }
        else if (bytes <= 64 * 1024)
        {
            return _RoundUp(bytes, 1024);
        }
        else if (bytes <= 256 * 1024)
        {
            return _RoundUp(bytes, 8 * 1024);
        }
        else{
            assert(false);
            return -1;
        }
    }

    // 获取对应哈希桶的下标
    static inline size_t Index(size_t bytes){
        // 每个字节数范围中，哈希桶的个数
        const size_t groupArray[4] = { 16, 56, 56, 56 };
        if (bytes <= 128)
        {
            return _Index_by_alignShift(bytes, 3);
        }
        else if (bytes <= 1024)
        {
            return _Index_by_alignShift(bytes - 128, 4) + groupArray[0];
        }
        else if (bytes <= 8 * 1024)
        {
            return _Index_by_alignShift(bytes - 1024, 7) + groupArray[0] + groupArray[1];
        }
        else if (bytes <= 64 * 1024)
        {
            return _Index_by_alignShift(bytes - 8 * 1024, 10) + groupArray[0] + groupArray[1] + groupArray[2];
        }
        else if (bytes <= 256 * 1024)
        {
            return _Index_by_alignShift(bytes - 64 * 1024, 13) + groupArray[0] + groupArray[1] + groupArray[2] + groupArray[3];
        }
        else
        {
            assert(false);
            return -1;
        }
    }

    // 按一定对齐方式进行向上对齐，获取对齐之后的字节数
    static inline size_t _RoundUp(size_t bytes, size_t alignNum){
        // if(bytes % alignNum != 0)
        //     return (bytes / alignNum + 1) * alignNum;
        // else return bytes;

        // 位运算写法
        return ((bytes + alignNum - 1)&~(alignNum - 1));
    }

    // 按一定对齐方式进行向上对齐，获取哈希桶的相对下标
    static inline size_t _Index_by_alignNum(size_t bytes, size_t alignNum){
        if(bytes % alignNum != 0) return bytes / alignNum;
        else return bytes / alignNum - 1;
    }
    static inline size_t _Index_by_alignShift(size_t bytes, size_t alignShift){
        // 位运算写法: 应该传入的是alignShift, 比如alignNum = 8, 因此alignShift = 3
        return ((bytes + (1 << alignShift) - 1) >> alignShift) - 1;
    }
};