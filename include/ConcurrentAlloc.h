#pragma once

#include "Common.h"
#include "ThreadCache.h"
#include "PageCache.h"

static void* ConcurrentAlloc(size_t bytes){
    if(bytes > MAX_BYTES){ // 向page cache申请，如果申请的内存再>=(N_PAGES-1)*(2^PAGE_SHIFT)，则page cahce中直接向堆申请内存
        size_t kPage = SizeClass::RoundUp(bytes) >> PAGE_SHIFT; // 需要申请kPage页
        
        PageCache::GetInstance()->_pageMtx.lock();
        Span* span = PageCache::GetInstance()->NewSpan(kPage);
        PageCache::GetInstance()->_pageMtx.unlock();

        return (void*)(span->_pageId << PAGE_SHIFT);
    }
    else{
        if(pTLSThreadCache == nullptr){
            // pTLSThreadCache = new ThreadCache;
            tcMtx.lock();
            pTLSThreadCache = tcPool.New();
            tcMtx.unlock();
        }
        // cout << "thread id: " << std::this_thread::get_id() << ":" << pTLSThreadCache << endl;
        return pTLSThreadCache->Allocate(bytes);
    }
    
}

static void ConcurrentFree(void* ptr, size_t bytes){
    if(bytes > MAX_BYTES){ // 向page cache释放，如果释放的内存再>=(N_PAGES-1)*(2^PAGE_SHIFT)，则page cahce中直接向堆释放内存
        Span* span = PageCache::GetInstance()->MapObj2Span(ptr);

        PageCache::GetInstance()->_pageMtx.lock();
        PageCache::GetInstance()->ReturnSpanToPageCache(span);
        PageCache::GetInstance()->_pageMtx.unlock();
    }
    else{
        assert(pTLSThreadCache);
        pTLSThreadCache->Deallocate(ptr, bytes);
    }
}

static void ConcurrentFree(void* ptr){
    Span* span = PageCache::GetInstance()->MapObj2Span(ptr);
    ConcurrentFree(ptr, span->objBytes);
}
