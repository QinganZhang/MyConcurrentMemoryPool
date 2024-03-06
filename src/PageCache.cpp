#include "Common.h"
#include "PageCache.h"

// 获取一个k个页的span
Span* PageCache::NewSpan(size_t k){
    assert(k > 0 && k < N_PAGES);
    if(_spanLists[k].Empty() == false)
        return _spanLists[k].PopFront();
    
    // 第k个桶是空的，只能继续向下找
    for(size_t i = k + 1; i < N_PAGES; ++i){
        if(_spanLists[i].Empty() == false){
            // 找到第i个桶不是空的，需要将这个span分裂成一个k个页的span和一个i-k个页的span
            Span* span = _spanLists[i].PopFront();
            Span* kSpan = new Span;
            kSpan->_pageId = span->_pageId;
            kSpan->_n = k;
            span->_pageId += k;
            span->_n -= k;
            _spanLists[span->_n].PushFront(span);
            return kSpan;
        }
    }

    // 剩下的桶全是空的，需要向堆申请一个N_PAGES-1页的span
    Span* bigSpan = new Span;
    void* ptr = SystemAlloc(N_PAGES - 1);
    bigSpan->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
    bigSpan->_n = N_PAGES - 1;
    _spanLists[N_PAGES-1].PushFront(bigSpan);
    
    return NewSpan(k);
}