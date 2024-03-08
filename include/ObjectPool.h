#pragma once

#include "Common.h"

template <typename T>
class ObjectPool{
public:
    T* New(){
        T* obj = nullptr;

        // 优先使用_freeList管理的定长内存块
        if(_freeList != nullptr){
            obj = (T*)_freeList;
            _freeList = NextObj(_freeList);
        }
        else{ // _freeList是空的
            // 定长内存块的大小必须能放得下一个地址（32位是4字节，64位是8字节）
            size_t objBytes = sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T);
            if(_remainBytes < objBytes){ // 如果能保证内存大小正好是对象大小的整数倍，则如果进入if语句，则一定是_remainBytes==0
                _remainBytes = 512 * 1024; // 2^9 * 2^10 = 512KB = 128Page (4KB/Page)
                _remainBytes = MAX(_remainBytes, SizeClass::RoundUp(objBytes));
                lst.push_front(std::make_pair(_memory, _remainBytes >> PAGE_SHIFT));
                _memory = (char*)SystemAlloc(_remainBytes >> PAGE_SHIFT); 
                if(_memory == nullptr) {
                    throw std::bad_alloc();
                }
            }
            // 现在大块内存肯定是够的
            obj = (T*)_memory;
            _memory += objBytes;
            _remainBytes -= objBytes;
        }

        // obj对象已经分配好了空间，然后需要进行构造，使用定位new进行构造
        new(obj)T;
        return obj;
    }

    void Delete(T* obj){
        obj->~T(); // 显示调用析构函数

        // 使用头插法，将定长内存块插入到链表头
        NextObj(obj) = _freeList;
        _freeList = obj;
    }

    ~ObjectPool(){
        for(auto& it: lst){
            SystemFree(it.first, it.second); // 析构函数中显式将每次申请的大块内存释放
        }
    }

private:
    char* _memory = nullptr; // 指向大块内存的指针
    size_t _remainBytes = 0; // 大块内存在切分过程中剩余的字节数量 
    void *_freeList = nullptr; // 释放回来的定长内存块组成的链表的指针
    std::list<std::pair<char*, size_t>> lst; // 记录不断创建的大块内存及其大小，用于销毁内存池时的释放
};
