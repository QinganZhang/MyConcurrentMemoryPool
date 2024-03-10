#include "Common.h"
#include "PageCache.h"

// 获取一个k个页的span
Span* PageCache::NewSpan(size_t k){
    assert(k > 0);
    if(k >= N_PAGES){
        void* ptr = SystemAlloc(k);
        // Span* span = new Span;
        Span* span = _spanPool.New(); // 使用定长内存池创建对象
        span->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
        span->_n = k;
        _page2SpanMap[span->_pageId] = span;
        // _page2SpanMap.set(span->_pageId, span);
        return span;
    }

    // 第k个桶正好不是空的
    if(_spanLists[k].Empty() == false){ 
        Span* kSpan = _spanLists[k].PopFront();
        for(PAGE_ID i = 0; i < kSpan->_n; ++i){
            _page2SpanMap[kSpan->_pageId + i] = kSpan;
            // _page2SpanMap.set(kSpan->_pageId + i, kSpan);
        }
        return kSpan;
    }
    
    // 第k个桶是空的，只能继续向下找
    for(size_t i = k + 1; i < N_PAGES; ++i){
        if(_spanLists[i].Empty() == false){
            // 找到第i个桶不是空的，需要将这个span分裂成一个k个页的span和一个i-k个页的span
            Span* span = _spanLists[i].PopFront();
            // Span* kSpan = new Span;
            Span* kSpan = _spanPool.New();
            kSpan->_pageId = span->_pageId;
            kSpan->_n = k;
            span->_pageId += k;
            span->_n -= k;
            _spanLists[span->_n].PushFront(span);

            // 切下来的剩下的这个span，它还在page cache中，因此只需要建立首尾页和span之间的映射即可
            // 如果该span在某回收span的前面，则使用该span的尾页页号map到该span
            // 如果该span在某回收span的后面，则使用该span的首页页号map到该span
            _page2SpanMap[span->_pageId] = span;
            _page2SpanMap[span->_pageId + span->_n - 1] = span;
            // _page2SpanMap.set(span->_pageId, span);
            // _page2SpanMap.set(span->_pageId + span->_n - 1, span);

            // 对于分给central cache的span，建立每一页页号与span的映射
            for(PAGE_ID i = 0; i < kSpan->_n; ++i){
                _page2SpanMap[kSpan->_pageId + i] = kSpan;
                // _page2SpanMap.set(kSpan->_pageId + i, kSpan);
            }
            return kSpan;
        }
    }

    // 剩下的桶全是空的，需要向堆申请一个N_PAGES-1页的span
    // Span* bigSpan = new Span;
    Span* bigSpan = _spanPool.New();
    void* ptr = SystemAlloc(N_PAGES - 1);
    bigSpan->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
    bigSpan->_n = N_PAGES - 1;
    _spanLists[N_PAGES-1].PushFront(bigSpan);
    
    return NewSpan(k);
}

Span* PageCache::MapObj2Span(void *obj){
    PAGE_ID pageId = (PAGE_ID) obj >> PAGE_SHIFT;
    
    // 如果是在page cache中使用MapObj2Span，因为进入到page cache中会加锁，所以是安全的
    // 但是如果在page cache外使用MapObj2Span，多个线程可能同时使用MapObj2Span
    // std::unique_lock<std::mutex> lock(_pageMtx); // 构造时加锁，析构时解锁
    
    // if(_page2SpanMap.find(pageId) != _page2SpanMap.end())
    //     return _page2SpanMap[pageId];
    // else{
    //     // 当通过对象的PAGE_ID查询对应的Span时，之前一定有对象所在页与其Span之间的映射关系；如果没有，则表示查询的obj有误
    //     assert(false);
    //     return nullptr;
    // }

    Span* ret = _page2SpanMap.get(pageId);
    assert(ret != nullptr);
    return ret;

    // if(ret == nullptr)
    // if(_page2SpanMap.get(pageId) != nullptr)
    //     return 
    // // if(_page2SpanMap.find(pageId) != _page2SpanMap.end())
    // //     return _page2SpanMap[pageId];
    // else {
    //     assert(false);
    //     return nullptr;
    // }
}

void PageCache::ReturnSpanToPageCache(Span* span){
    // span是从central cache回收得到的span，也有可能是大块内存直接从thread cache中回收得到的span

    if(span->_n >= N_PAGES){ // 此时span是thread cache直接从堆申请来的特大块内存，page cache也放不下，因此直接还给堆
        void* ptr = (void*)(span->_pageId << PAGE_SHIFT);
        SystemFree(ptr, span->_n);
        // delete span;
        _spanPool.Delete(span);
        return ;
    }

    // 此时span的大小可以在page cache中放得下
    // 向前合并
    while(true){
        PAGE_ID prevPageId = span->_pageId - 1;

        if(_page2SpanMap.find(prevPageId) == _page2SpanMap.end()) 
            break; // 前面一页还没有向系统申请过，停止向前合并
        // Span* prevSpan = (Span*) _page2SpanMap.get(prevPageId);
        // if(prevSpan == nullptr) break;

        Span* prevSpan = _page2SpanMap[prevPageId]; // 前面相邻的一个span
        if(prevSpan->_isUse == true)
            break; // 该span还在central cache中
        
        if(prevSpan->_n + span->_n >= N_PAGES) 
            break; // 如果合并，则总的页数过大，超出了page cache桶下标的范围
        span->_pageId = prevSpan->_pageId;
        span->_n += prevSpan->_n;
        _spanLists[prevSpan->_n].Erase(prevSpan);
        // delete prevSpan;
        _spanPool.Delete(prevSpan);
    }

    // 向后合并
    while(true){
        PAGE_ID nextPageId = span->_pageId + span->_n;
        
        if(_page2SpanMap.find(nextPageId) == _page2SpanMap.end())
            break; 
        // Span* nextSpan = (Span*)_page2SpanMap.get(nextPageId);
        // if(nextSpan == nullptr) break;

        Span* nextSpan = _page2SpanMap[nextPageId];
        if(nextSpan->_isUse == true)
            break;
        if(nextSpan->_n + span->_n >= N_PAGES)
            break;
        span->_n += nextSpan->_n;
        _spanLists[nextSpan->_n].Erase(nextSpan);
        // delete nextSpan;
        _spanPool.Delete(nextSpan);
    }

    _spanLists[span->_n].PushFront(span);
    _page2SpanMap[span->_pageId] = span;
    _page2SpanMap[span->_pageId + span->_n - 1] = span;
    // _page2SpanMap.set(span->_pageId, span);
    // _page2SpanMap.set(span->_pageId + span->_n - 1, span);
    span->_isUse = false;

}