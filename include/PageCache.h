#pragma once

#include "Common.h"

class PageCache{
public:
    // 单例模式    
    static PageCache* GetInstance(){
        static PageCache _sInst;
        return &_sInst;
    }
    Span* NewSpan(size_t k); // 返回一个包含k个page的Span
public:
    std::mutex _pageMtx;
private:
    PageCache() =default;
    PageCache(const PageCache&) =delete;
    PageCache& operator=(const PageCache&) =delete;
private:
    SpanList _spanLists[N_PAGES];
    
};