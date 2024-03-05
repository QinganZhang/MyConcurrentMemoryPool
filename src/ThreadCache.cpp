#include "ThreadCache.h"

void* ThreadCache::Allocate(size_t size){
    assert(size <= MAX_BYTES);
    size_t alignedSize = SizeClass::RoundUp(size);
    size_t index = SizeClass::Index(size);
    if(!_freeLists[index].Empty()) 
        return _freeLists[index].Pop();
    else 
        return FetchFromCentralCache(index, alignedSize);
}

void ThreadCache::Deallocate(void* ptr, size_t size){
    size_t alignedSize = SizeClass::RoundUp(size);
    size_t index = SizeClass::Index(size);
    _freeLists[index].Push(ptr);
    // ReturnBackToCentralCache();    
}

void* ThreadCache::FetchFromCentralCache(size_t index, size_t size){

}

void ThreadCache::ReturnBackToCentralCache(FreeList& list, size_t size){

}