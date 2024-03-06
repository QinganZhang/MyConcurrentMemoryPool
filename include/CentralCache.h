#pragma once

#include "Common.h"

class CentralCache{
public:    
    // 由于Central Cache在整个进程中只有一个实例，因此设置为单例模式，创建static的数据成员，并提供一个全局访问点
    static CentralCache* GetInstance(){
        static CentralCache _sInst;
        return &_sInst;
    }
    
    // 从central cache中获取n个size大小的内存块, [start, end]
    size_t FetchRangeObj(void*& start, void*& end, size_t n, size_t size); 

    // 从central cache对应的哈希桶中获取一个非空的span
    Span* GetOneSpan(SpanList& spanList, size_t size);
private:
    CentralCache() =default;
    CentralCache(const CentralCache&) =delete;
    CentralCache& operator= (const CentralCache&) =delete;
private:
    SpanList _spanLists[N_FREE_LISTS];
};