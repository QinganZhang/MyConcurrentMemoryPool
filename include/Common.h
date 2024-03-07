#pragma once

#include <iostream>
#include <vector>
#include <list>
#include <unordered_map>
#include <algorithm>
#include <cassert>
#include <time.h>
#include <mutex>
#include <atomic>

using std::cout;
using std::endl;

static const size_t PAGE_SHIFT = 12; // 一页定义为2^PAGE_SHIFT KB大小
static const size_t MAX_BYTES = 256 * 1024; // 申请内存大小<=MAX_BYTES，找threadCache申请，否则找pageCache申请；
    // 如果申请内存大小> (N_PAGE-1)*(2^PAGE_SHIFT)大小，则直接找堆申请
static const size_t N_FREE_LISTS = 232; // thread Cache哈希桶的个数
static const size_t N_PAGES = 129; // page Cache哈希桶的个数

// 每个定长内存块的前4个字节或者前8个字节作为next指针，存储下一个定长内存块的地址
#ifdef _WIN64 
    typedef unsigned long long PAGE_ID;
#elif _WIN32
    typedef size_t PAGE_ID;
#else // linux
    typedef unsigned long long PAGE_ID;
#endif

// 宏WIN32, _WIN32, _WIN64的介绍：https://blog.csdn.net/wangkui1331/article/details/103491790
#ifdef _WIN32 // win32位下只有_WIN32有定义，win64位下_WIN32和_WIN64都有定义
    #include <Windows.h>
    #include <thread>
#else // linux
    #include <unistd.h>
    #include <sys/mman.h>
#endif

// 直接去堆上按页申请内存
inline static void* SystemAlloc(size_t kpage){
#ifdef _WIN32
    // VirtualAlloc用于在进程的虚拟地址空间中分配动态内存
    void *ptr = VirtualAlloc(NULL, kpage << PAGE_SHIFT, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else 
    // linux下mmap
    void* ptr = mmap(NULL, kpage << PAGE_SHIFT, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
#endif
    if(ptr == nullptr) 
        throw std::bad_alloc();
    else return ptr;
}

// 直接将内存还给堆
inline static void SystemFree(void *ptr, size_t kpage = 0){
#ifdef _WIN32
    VirtualFree(ptr, 0, MEM_RELEASE); 
#else 
    munmap(ptr, kpage << PAGE_SHIFT);
#endif
}

// 让一个指针在32位平台下解引用后能向后访问4个字节，在64位平台下则可以访问8个字节
static void*& NextObj(void* ptr){ // 如果不加引用，返回的是右值
    return (*(void**)ptr);
    // 将ptr先强转为二级指针（原来ptr是一级指针，现在则视为ptr指向的内存地址中保存的也是一个地址），然后再解引用
}

// thread cache中的每个哈希桶对应的链表
class FreeList{
public:

    // 将一块小内存头插到链表中
    void Push(void* obj){ 
        assert(obj);
        NextObj(obj) = _freeList;
        _freeList = obj;
        ++_size;
    }

    // 从链表中获取一块小内存
    void* Pop(){
        assert(_freeList);
        void* obj = _freeList;
        _freeList = NextObj(_freeList);
        --_size;
        return obj;
    }

    bool Empty() {
        return _freeList == nullptr;
    }

    size_t& MaxSize(){
        return _maxSize;
    }

    size_t Size() {
        return _size;
    }

    // 将一段内存插入到链表中
    void PushRange(void* start, void* end, size_t cnt){
        assert(start);
        assert(end);
        NextObj(end) = _freeList;
        _freeList = start;
        _size += cnt;
    }

    // 将一段内存块从链表中取出
    void PopRange(void*& start, void*& end, size_t cnt){
        assert(cnt <= _size);
        start = _freeList;
        end = _freeList;
        for(size_t i = 1; i <= cnt-1; ++i){
            end = NextObj(end);
        }
        _freeList = NextObj(end);
        NextObj(end) = nullptr;
        _size -= cnt;
    }
private:
    void* _freeList = nullptr;
    size_t _size = 0; // 内存块数量
    size_t _maxSize = 1;  // 每次_freeList为空时，为了向Central Cache适当超额申请，此时打算申请_maxSize个内存块
        // 但是最后实际上向Central Cache申请的，可能没有_maxSize这么多，因为如果申请的内存块比较大，也不太适合申请过多，这是实际申请过程中的另外一个限制
};

// thread cache中哈希桶的映射规则
class SizeClass{
public:
    // 当申请bytes字节内存时，获取向上对齐后的字节数
    static inline size_t RoundUp(size_t bytes){
        if (bytes <= 128)
            return _RoundUp(bytes, 8);
        else if (bytes <= 1024)
            return _RoundUp(bytes, 16);
        else if (bytes <= 8 * 1024)
            return _RoundUp(bytes, 128);
        else if (bytes <= 64 * 1024)
            return _RoundUp(bytes, 1024);
        else if (bytes <= 256 * 1024)
            return _RoundUp(bytes, 4 * 1024);
        else
            return _RoundUp(bytes, 1 << PAGE_SHIFT);
    }

    // 当申请bytes字节空间时，获取对应哈希桶的下标
    static inline size_t Index(size_t bytes){ 
        // 每个字节数范围中，哈希桶的个数
        const size_t groupArray[4] = { 16, 56, 56, 56 };
        if (bytes <= 128)
            return _Index_by_alignShift(bytes, 3);
        else if (bytes <= 1024)
            return _Index_by_alignShift(bytes - 128, 4) + groupArray[0];
        else if (bytes <= 8 * 1024)
            return _Index_by_alignShift(bytes - 1024, 7) + groupArray[0] + groupArray[1];
        else if (bytes <= 64 * 1024)
            return _Index_by_alignShift(bytes - 8 * 1024, 10) + groupArray[0] + groupArray[1] + groupArray[2];
        else if (bytes <= 256 * 1024)
            return _Index_by_alignShift(bytes - 64 * 1024, 13) + groupArray[0] + groupArray[1] + groupArray[2] + groupArray[3];
        else
        {
            assert(false);
            return -1;
        }
    }

    // 按一定对齐方式进行向上对齐，获取对齐之后的字节数
    static inline size_t _RoundUp(size_t bytes, size_t alignBytes){
        // if(bytes % alignBytes != 0)
        //     return (bytes / alignBytes + 1) * alignBytes;
        // else return bytes;

        // 位运算写法
        return ((bytes + alignBytes - 1)&~(alignBytes - 1));
    }

    // 按一定对齐方式进行向上对齐，获取哈希桶的相对下标
    static inline size_t _Index_by_alignNum(size_t bytes, size_t alignBytes){
        if(bytes % alignBytes != 0) return bytes / alignBytes;
        else return bytes / alignBytes - 1;
    }
    static inline size_t _Index_by_alignShift(size_t bytes, size_t alignShift){
        // 位运算写法: 应该传入的是alignShift, 比如alignNum = 8, 因此alignShift = 3
        return ((bytes + (1 << alignShift) - 1) >> alignShift) - 1;
    }

    // thread cache向central cache申请内存时，申请的内存块大小是alignedBytes，应该分配内存块的数量上限
    static size_t ThreadCacheAllocFromCentralCache_MaxNum(size_t alignedBytes){
        assert(alignedBytes > 0);
        int num = MAX_BYTES / alignedBytes; 
        num = std::max(num, 2);
        num = std::min(num, 512);
        return num;
    }

    // central cache向page cache申请内存时，申请的内存块大小是alignedBytes，应该申请的Page的数量
    static size_t CentralCacheAllocFromPageCache_Num(size_t alignedBytes){
        // 申请的内存块大小为alignedBytes时，thread cache向central cache最多申请blockNum个内存块
        size_t blockNum = ThreadCacheAllocFromCentralCache_MaxNum(alignedBytes); 
        // 应该满足最多blockNum个内存块，换算到Page是pageNum个
        size_t pageNum = blockNum * alignedBytes;
        pageNum >>= PAGE_SHIFT; 
        return std::max(pageNum, (size_t)(1) );
    }
};

// central cache每个桶中存储的都是一个双链表，双链表每个节点就是Span，Span是一个管理以页为单位的大块内存
// 注意不同span大小可能不同，Span本身没有内存空间，是里面的_freeList指向内存块
// Span本身不保存内存空间，其内部_freeList指向切好的小块内存，同时通过_pageId可以隐含的得到整个大块内存，相当于Span与内存块相对应
struct Span{
    PAGE_ID _pageId = 0;        //大块内存起始页的页号，目的是方便page cache进行前后页的合并
	size_t _n = 0;              //这个Span有几个页

	Span* _next = nullptr;      //双链表结构
	Span* _prev = nullptr;

	size_t _useCount = 0;       //切好的小块内存，被分配给thread cache的计数。如果计数为0，代表当前span切出去的内存块都还回来了，因此central cache可以再将这个span还给page cache
	void* _freeList = nullptr;  //切好的小块内存的自由链表
    size_t objBytes = 0;    // 切好的小块内存的大小

    bool _isUse = false; // if true, span is on central cache; if false, span is on page cache
        // page cache 回收得到一个span时，尝试将相邻的span合并起来，前提是相邻的span在page cache中
        // 比如向前向后通过页号的加减得到相邻的span时，该span可能在page cache中，也可能在central cache中
        // 如果该span的_useCount!=0，则它肯定在central cache中；但是如果_useCount==0，则有可能是该span已经从central cache中回收到page cache，
        //      也可能是该span就没有分出去过，但是也有可能该span刚刚分给central cache，还没有使用
        // 因此不能通过_useCount来判断该span在page cache还是在central cache上
};

// 带虚拟头节点的双向循环链表
class SpanList{
public:
    SpanList(){
        _head = new Span;
        _head->_next = _head;
        _head->_prev = _head;
    }

    void Insert(Span* pos, Span* newSpan){
        assert(pos);
        assert(newSpan);
        Span* prev = pos->_prev;
        prev->_next = newSpan;
        newSpan->_prev = prev;

        newSpan->_next = pos;
        pos->_prev = newSpan;
    }

    void Erase(Span* pos){
        assert(pos);
        assert(pos != _head);
        Span* prev = pos->_prev;
        Span* next = pos->_next;
        prev->_next = next;
        next->_prev = prev;
        // 无需删除这个span，只是将这个span从链表中删除，这个span会归还到page cache
    }

    // 直接将span头插到spanlist中
    void PushFront(Span* span){
        Insert(Begin(), span);
    }

    Span* PopFront(){
        Span* front = _head->_next;
        Erase(front);
        return front;
    }

    Span* Begin(){ 
        return _head->_next; // 注意_head的下一个位置是第一个位置Begin
    }

    Span* End() {
        return _head; // 因为是双向循环链表
    }

    bool Empty() {
        return _head == _head->_next;
    }
private:
    Span* _head; // _head的下一个位置是第一个位置，初始只有一个span，因此_head的下一个仍然是自己（循环链表）
public:
    std::mutex _mtx;
};