#pragma once

#include "Common.h"

class CentralCache{
public:    
    // 由于Central Cache在整个进程中只有一个实例，因此设置为单例模式，创建static的数据成员，并提供一个全局访问点
    static CentralCache* GetInstance(){
        static CentralCache _sInst;
        return &_sInst;
    }
    
    // 从central cache中获取n个alignedBytes大小的内存块, [start, end]
    size_t FetchRangeObj(void*& start, void*& end, size_t n, size_t alignedBytes); 

    // 从central cache中alignedBytes大小内存块对应的哈希桶中寻找一个非空的span
    Span* GetOneSpan(SpanList& spanList, size_t alignedBytes);

    // 从thread cache中回收n个alignedBytes大小的内存块
    void ReturnRangeObj(void* start, void* end, size_t n, size_t alignedBytes);
    
private:
    CentralCache() =default;
    CentralCache(const CentralCache&) =delete;
    CentralCache& operator= (const CentralCache&) =delete;
private:
    SpanList _spanLists[N_FREE_LISTS];
};