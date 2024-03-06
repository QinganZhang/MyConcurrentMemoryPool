#include "ThreadCache.h"
#include "CentralCache.h"
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
    // thread cache向central cache申请size大小的内存块时，该分几个内存块呢？分配的原则为：
    // 1. 刚开始分配时，不管大内存块还是小内存块，都少分一点
    // 2. 后来分配内存时，大内存块分配的上限低一点，小内存块分配的上限高一点
    size_t batchNum = std::min(_freeLists[index].MaxSize(), SizeClass::ThreadCacheAllocFromCentralCache_MaxNum(size));
    if(batchNum == _freeLists[index].MaxSize()){
        _freeLists[index].MaxSize() += 1;
    }
    void *start = nullptr, *end = nullptr;
    size_t allocSize = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, size);
    assert(allocSize >= 1);
    if(allocSize == 1){
        assert(start == end);
        return start;
    }
    else{
        _freeLists[index].PushRange(NextObj(start), end);
        return start;
    }
}

void ThreadCache::ReturnBackToCentralCache(FreeList& list, size_t size){

}