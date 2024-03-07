#include "ThreadCache.h"
#include "CentralCache.h"

void* ThreadCache::Allocate(size_t bytes){
    assert(bytes <= MAX_BYTES);
    size_t alignedBytes = SizeClass::RoundUp(bytes);
    size_t index = SizeClass::Index(bytes);
    if(!_freeLists[index].Empty()) 
        return _freeLists[index].Pop();
    else 
        return FetchFromCentralCache(_freeLists[index], alignedBytes); // 向central cache申请
}

void ThreadCache::Deallocate(void* ptr, size_t bytes){
    assert(ptr != nullptr);
    assert(bytes <= MAX_BYTES);
    size_t alignedBytes = SizeClass::RoundUp(bytes);
    size_t index = SizeClass::Index(bytes);
    _freeLists[index].Push(ptr);
    if(_freeLists[index].Size() > _freeLists[index].MaxSize()){
        ReturnBackToCentralCache(_freeLists[index], alignedBytes);    
    }
}

void* ThreadCache::FetchFromCentralCache(FreeList& freeList, size_t alignedBytes){
    // thread cache向central cache申请bytes大小的内存块时，该分几个内存块呢？分配的原则为：
    // 1. 刚开始分配时，不管大内存块还是小内存块，都少分一点：_freeLists[index].MaxSize()
    // 2. 后来分配内存时，大内存块分配的上限低一点，小内存块分配的上限高一点：SizeClass::ThreadCacheAllocFromCentralCache_MaxNum(bytes)
    size_t blockNum = std::min(freeList.MaxSize(), SizeClass::ThreadCacheAllocFromCentralCache_MaxNum(alignedBytes));
    if(blockNum == freeList.MaxSize()){
        freeList.MaxSize() += 1;
    }
    void *start = nullptr, *end = nullptr;
    // 向central cache申请分配blockNum个bytes大小的内存块，最终分配到了allocNum个内存块，用[start, end]来记录
    size_t allocNum = CentralCache::GetInstance()->FetchRangeObj(start, end, blockNum, alignedBytes);
    assert(allocNum >= 1);
    if(allocNum == 1){
        assert(start == end);
        return start;
    }
    else{
        freeList.PushRange(NextObj(start), end, allocNum-1); // 将超额申请的内存块挂到thread cache对应的桶中
        return start;
    }
}

void ThreadCache::ReturnBackToCentralCache(FreeList& freeList, size_t alignedBytes){
    void* start = nullptr, *end = nullptr;
    freeList.PopRange(start, end, freeList.MaxSize());
    CentralCache::GetInstance()->ReturnRangeObj(start, end, freeList.MaxSize(), alignedBytes);
}