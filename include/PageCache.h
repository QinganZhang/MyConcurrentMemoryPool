#pragma once

#include "Common.h"
#include "ObjectPool.h"

class PageCache{
public:
    // 单例模式    
    static PageCache* GetInstance(){
        static PageCache _sInst;
        return &_sInst;
    }
    Span* NewSpan(size_t k); // 返回一个包含k个page的Span

    Span* MapObj2Span(void *obj); // 给定一个对象，返回它应该在哪个span上

    void ReturnSpanToPageCache(Span* span); // 回收空闲的span（从central cache释放的span）到page cache，并合并相邻的span
public:
    std::mutex _pageMtx;
private:
    PageCache() =default;
    PageCache(const PageCache&) =delete;
    PageCache& operator=(const PageCache&) =delete;
private:
    SpanList _spanLists[N_PAGES];
    std::unordered_map<PAGE_ID, Span*> _page2SpanMap;
    ObjectPool<Span> _spanPool; // 针对Span的定长内存池，因为Span的申请和释放都是在page cache中
};