#include "CentralCache.h"
#include "PageCache.h"

size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t n, size_t alignedBytes){
    size_t index = SizeClass::Index(alignedBytes);
    _spanLists[index]._mtx.lock(); // 每个桶有一个锁

    // 从central cache对应的哈希桶中获取一个非空的span
    Span* span = GetOneSpan(_spanLists[index], alignedBytes);
    assert(span);
    assert(span->_freeList);

    start = span->_freeList;
    end = span->_freeList;
    size_t cnt = 1; // 这边n-1是因为[begin, end], 此时已经有一个内存块了，因此cnt=1
    for(size_t i = 0; i < n-1 && NextObj(end) != nullptr; ++i){ // 有可能该span中内存块已经不够n个，此时就分配剩下的就行了
        end = NextObj(end);
        ++cnt;
    }
    // 因此有可能实际上并没有分配到n个内存块，但是并没有很大影响。因为本来thread cache只是向central cache申请特定大小的一个内存块，此时多申请了一些内存块返回去，本来就是超额分配
    span->_freeList = NextObj(end);
    span->_useCount += cnt; // 更新该span分配给thread cache的内存块数量
    NextObj(end) = nullptr;
    _spanLists[index]._mtx.unlock();
    return cnt; 
}

Span* CentralCache::GetOneSpan(SpanList& spanList, size_t alignedBytes){
    // 先在spanlist中寻找非空的span，注意此时桶已经加上了锁
    for(Span* it = spanList.Begin(); it != spanList.End(); it = it->_next){
        if(it->_freeList != nullptr) 
            return it;
    }

    // 现在遍历了整个spanlist，里面每个span都没有内存块了（即_freeList都是空的）
    // 因此只能向page cache申请，此时可以先将桶锁释放（因为可能此时桶进行回收一些内存），但是对于page cache的操作需要进行加锁
    spanList._mtx.unlock();
    PageCache::GetInstance()->_pageMtx.lock();
    Span* span = PageCache::GetInstance()->NewSpan(SizeClass::CentralCacheAllocFromPageCache_Num(alignedBytes));
    span->objBytes = alignedBytes;
    // 从page cache中申请得到了一个span，对page cache的操作完成，可以解锁
    PageCache::GetInstance()->_pageMtx.unlock();

    char* start = (char*) (span->_pageId << PAGE_SHIFT);
    size_t bytes = span->_n << PAGE_SHIFT;
    char* end = start + bytes;
    // span是一个有bytes大小(span->_n个页)的连续空间的大块内存（实际上是内部_freeList指向的空间），起始地址为start（逻辑地址），且内部_freeList为空，并没有切开

    // 现在切开，按照原来的顺序顺次切开，并修改内存块的指针
    // “切开”的意思是将内存块按照链表的结构更新指针，使之可以挂载到central cache的span中的_freeList上，但实际上这些内存块在物理上还是连续的
    span->_freeList = start;
    start += alignedBytes;
    for(void* last = span->_freeList; start < end; ){
        NextObj(last) = start;
        NextObj(start) = nullptr;
        last = start; 
        start += alignedBytes;
    }
    // 将span 头插 到spanlist中，central cache中的span顺序无所谓
    // 需要先将spanlist加桶锁
    spanList._mtx.lock();
    spanList.PushFront(span);
    
    return span;
}

void CentralCache::ReturnRangeObj(void* start, void* end, size_t n, size_t alignedBytes){
    size_t index = SizeClass::Index(alignedBytes);
    _spanLists[index]._mtx.lock(); // 每个桶有一个锁

    // 但是注意这n个内存块并不一定全部回收到一个span中，可能回收到多个span中
    // 因此问题来了，一个内存块应该放到哪个span中？
    // 如果乱放，则central cache向page cache归还内存时，page cache无法进行整理和合并内存块（因为需要连续）
    // 因此最好能知道一个内存块应该放到哪一个span中，内存块除以页大小可以知道这个内存块在哪个页上，
    // 因此可以建立页号到span的映射关系

    for(void* next = NextObj(start); start != nullptr; start = next, next = NextObj(next)){
        Span* span = PageCache::GetInstance()->MapObj2Span(start);
        NextObj(start) = span->_freeList; // 头插
        span->_freeList = start;
        --span->_useCount; // _useCount统计该span分出去的内存块的数量
        
        if(span->_useCount == 0){ // 该span所有分出去的内存块都已经回收，该span再换给page cache
            _spanLists[index].Erase(span);
            span->_freeList = nullptr; // 这个span可以通过pageId来获取其地址
            span->_next = nullptr; 
            span->_prev = nullptr;

            // 将span归还给page cache时，central cache对应桶不参与，可以释放锁
            _spanLists[index]._mtx.unlock(); 
            PageCache::GetInstance()->_pageMtx.lock(); 
            PageCache::GetInstance()->ReturnSpanToPageCache(span);
            PageCache::GetInstance()->_pageMtx.unlock();
            _spanLists[index]._mtx.lock();
        }
    }
    _spanLists[index]._mtx.unlock();

}