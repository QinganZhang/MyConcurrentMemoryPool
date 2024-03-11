
### 项目介绍

  本项目实现的是一个高并发的[内存池](https://so.csdn.net/so/search?q=%E5%86%85%E5%AD%98%E6%B1%A0&spm=1001.2101.3001.7020)，它的原型是 Google 的一个开源项目 tcmalloc，tcmalloc 全称 Thread-Caching Malloc，即线程缓存的 malloc，实现了高效的多线程内存管理，用于替换系统的内存分配相关函数 malloc 和 free。    

  该项目就是把 tcmalloc 中最核心的框架简化后拿出来，模拟实现出一个 mini 版的高并发内存池，目的就是学习 tcmalloc 的精华。

  该项目主要涉及 C/C++、数据结构（链表、哈希桶）、[操作系统内存管理](https://so.csdn.net/so/search?q=%E6%93%8D%E4%BD%9C%E7%B3%BB%E7%BB%9F%E5%86%85%E5%AD%98%E7%AE%A1%E7%90%86&spm=1001.2101.3001.7020)、单例模式、多线程、互斥锁等方面的技术。

### 内存池介绍

#### 池化技术

  在说内存池之前，我们得先了解一下 “池化技术”。所谓 “池化技术”，就是程序先向系统申请过量的资源，然后自己进行管理，以备不时之需.之所以要申请过量的资源，是因为申请和释放资源都有较大的开销，不如提前申请一些资源放入 “池” 中，当需要资源时直接从 “池” 中获取，不需要时就将该资源重新放回 “池” 中即可。这样使用时就会变得非常快捷，可以大大提高程序的运行效率。

  在计算机中，有很多使用 “池” 这种技术的地方，除了内存池之外，还有连接池、线程池、对象池等。以服务器上的线程池为例，它的主要思想就是：先启动若干数量的线程，让它们处于睡眠状态，当接收到客户端的请求时，唤醒池中某个睡眠的线程，让它来处理客户端的请求，当处理完这个请求后，线程又进入睡眠状态。

#### 内存池

  内存池是指程序预先向操作系统申请一块足够大的内存，此后，当程序中需要申请内存的时候，不是直接向操作系统申请，而是直接从内存池中获取；同理，当释放内存的时候，并不是真正将内存返回给操作系统，而是将内存返回给内存池。当程序退出时（或某个特定时间），内存池才将之前申请的内存真正释放。

#### 内存池主要解决的问题

  内存池主要解决的就是效率的问题，它能够避免让程序频繁的向系统申请和释放内存。其次，内存池作为系统的内存分配器，还需要尝试解决内存碎片的问题。内存碎片分为内部碎片和外部碎片：

*   外部碎片是一些空闲的小块内存区域，由于这些内存空间不连续，以至于合计的内存足够，但是不能满足一些内存分配申请需求。
*   内部碎片是由于一些对齐的需求，导致分配出去的空间中一些内存无法被利用。

**注意：** 内存池尝试解决的是外部碎片的问题，同时也尽可能的减少内部碎片的产生。

#### malloc

  C/C++ 中我们要动态申请内存并不是直接去堆申请的，而是通过 malloc 函数去申请的，包括 C++ 中的 new 实际上也是封装了 malloc 函数的。

  我们申请内存块时是先调用 malloc，malloc 再去向操作系统申请内存。malloc 实际就是一个内存池，malloc 相当于向操作系统 “批发” 了一块较大的内存空间，然后 “零售” 给程序用，当全部 “售完” 或程序有大量的内存需求时，再根据实际需求向操作系统“进货”。  

<img src="https://cdn.jsdelivr.net/gh/QinganZhang/ImageHosting/img/2024-03-11-11:15:03.png" style="zoom: 33%;" />

​		malloc 的实现方式有很多种，一般不同编译器平台用的都是不同的。比如 Windows 的 VS 系列中的 malloc 就是微软自行实现的，而 Linux 下的 gcc 用的是 glibc 中的 ptmalloc。

### 定长内存池的实现

  malloc 其实就是一个通用的内存池，在什么场景下都可以使用，但这也意味着 malloc 在什么场景下都不会有很高的性能，因为 malloc 并不是针对某种场景专门设计的。定长内存池就是针对固定大小内存块的申请和释放的内存池，由于定长内存池只需要支持固定大小内存块的申请和释放，因此我们可以将其性能做到极致，并且在实现定长内存池时不需要考虑内存碎片等问题，因为我们申请 / 释放的都是固定大小的内存块。

​		为了实现定长，可以将传入模板的类型作为“定长”，每次都是申请sizeof(T)大小的内存块。

  既然是内存池，那么我们首先得向系统申请一块内存空间，然后对其进行管理。要想直接向堆申请内存空间，在 Windows 下，可以调用 VirtualAlloc 函数；在 Linux 下，可以调用 brk 或 mmap 函数。可以使用条件编译的方式进行判断。[linux内存管理 mmap brk](https://www.cnblogs.com/beilou310/p/17037066.html)

```cpp
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
```

#### 类的设计

##### 成员变量

定长内存池当中包含三个成员变量：

* _memory：向堆申请到大块内存后，指向这个大块内存的指针。

    <img src="https://cdn.jsdelivr.net/gh/QinganZhang/ImageHosting/img/2024-03-11-11:16:03.png" style="zoom:50%;" />

* _remainBytes：大块内存切分过程中剩余字节数。

* _freeList：对象释放过程中，归还回来的定长内存块组成一个链表（自由链表），使用一个指针来记录

    <img src="https://cdn.jsdelivr.net/gh/QinganZhang/ImageHosting/img/2024-03-11-11:16:08.png" style="zoom:50%;" />

    ​		对于还回来的定长内存块，我们可以用自由链表将其链接起来，但我们并不需要为其专门定义链式结构，我们可以让内存块的前 4 个字节（32 位平台）或 8 个字节（64 位平台）作为指针，存储后面内存块的起始地址即可。

##### 方法

- 给定一个内存块，如何得到下一个内存块的地址呢？即让一个指针在 32 位平台下解引用后能向后访问 4 个字节，在 64 位平台下解引用后能向后访问 8 个字节

    ```cpp
    void*& NextObj(void* ptr){
    	return (*(void**)ptr); 
        // 将ptr先强转为二级指针（原来ptr是一级指针，现在则视为ptr指向的内存地址中保存的也是一个地址），然后再解引用，得到的就是对应平台上的指针
    }
    ```

- 在内存池中申请一个内存块：New

    - 如果自由链表中还有内存块，则优先使用归还回来的内存块，头删一个内存块

    - 如果自由链表当中没有内存块，那么我们就在大块内存中切出定长的内存块进行返回，当内存块切出后及时更新`_memory`指针的指向，以及`_remainBytes`的值即可。

        <img src="https://cdn.jsdelivr.net/gh/QinganZhang/ImageHosting/img/2024-03-11-11:16:17.png" style="zoom:40%;" />

    - 如果大块内存已经不足以切分出一个对象时，就再使用系统调用，向堆申请一块内存空间，更新`_memory`指针的指向，以及`_remainBytes`的值

    - 最后成功分到内存块后，使用定位new，显式进行构造和初始化

- 向内存池释放对象，归还一个内存块：Delete

### 高并发内存池整体框架设计

#### 背景

​	在多线程场景下，malloc分配内存，可能会由于频繁的加锁解锁，存在激烈的锁竞争，导致效率变低。项目的原型 tcmalloc 实现的就是一种在多线程高并发场景下更胜一筹的内存池。

  在实现内存池时我们一般需要考虑到效率问题和内存碎片的问题，但对于高并发内存池来说，我们还需要考虑在多线程环境下的锁竞争问题。

- 效率问题，避免频繁的系统调用
- 减少内存碎片（外部碎片）的产生
- 减少多线程环境下的锁竞争

#### 整体框架设计

<img src="https://cdn.jsdelivr.net/gh/QinganZhang/ImageHosting/img/2024-03-11-11:16:25.png" style="zoom: 33%;" />

高并发内存池主要由以下三个部分构成：

*   **thread cache：** 每个线程独有的，用于小于等于 256KB 的内存分配。
    *   thread cache主要解决锁竞争的问题，thread cache每个线程独有，也就意味着线程在 thread cache 申请内存时是不需要加锁的，而一次性申请大于 256KB 内存的情况是很少的，因此大部分情况下申请内存时都是无锁的。这也就是这个高并发内存池高效的地方。
    *   每个线程的 thread cache 会根据自己的情况向 central cache 申请或归还内存，这就避免了出现单个线程的 thread cache 占用太多内存，而其余 thread cache 出现内存不够的问题。
    *   多线程的 thread cache 可能会同时找 central cache 申请内存，此时就会涉及线程安全的问题，因此在访问 central cache 时是需要加锁的，但 central cache 实际上是一个哈希桶的结构，只有当多个线程同时访问同一个桶时才需要加锁，所以这里的锁竞争也不会很激烈。

*   **central cache：** 所有线程所共享，起到一个居中调度的作用
    *   当 thread cache 需要内存时会按需从 central cache 中获取内存，而当 thread cache 中的内存多了就会将内存还给central cache

*   **page cache：** 负责提供以页为单位的大块内存，
    *   当 central cache 需要内存时就会从 page cache 中获取内存，而当 central cache 中的内存多了就会将内存还给page cache，将回收的内存尽可能的进行合并，组成更大的连续内存块，缓解内存碎片的问题。
    *   当page cache内存不够时，就会使用系统调用去向堆申请内存


#### thread cache

##### thread cache 整体设计

###### 数据结构（类的数据成员）

- FreeList[]

  thread cache 实际上是一个哈希桶结构（自由链表FreeList的数组），每个桶中存放的都是一个自由链表，每个自由链表管理不同大小的内存块

  thread cache 支持小于等于 256KB 内存的申请，如果我们将每种字节数的内存块都用一个自由链表进行管理的话，那么此时我们就需要 20 多万个自由链表，光是存储这些自由链表的头指针就需要消耗大量内存，这显然是得不偿失的。

  这时我们可以选择做一些平衡的牺牲，让这些字节数按照某种规则进行对齐，小内存使用密集方式对齐，大内存使用稀疏方式对齐。例如我们让这些字节数都按照 8 字节进行向上对齐，那么 thread cache 的结构就是下面这样的，此时当线程申请 1~8 字节的内存时会直接给出 8 字节，而当线程申请 9~16 字节的内存时会直接给出 16 字节，以此类推。  
<img src="https://cdn.jsdelivr.net/gh/QinganZhang/ImageHosting/img/2024-03-11-21:06:06.png" style="zoom:50%;" />  
  因此当线程要申请某一大小的内存块时，基于某种对齐规则，计算出对齐后的内存块大小，进而找到对应的哈希桶，如果该哈希桶中的自由链表中有内存块，那就从自由链表中头删一个内存块进行返回；如果该自由链表已经为空了，那么就需要向下一层的 central cache 进行获取了。

> 对齐规则：如果申请一定大小（bytes）的内存块，对齐后的内存块多大？对齐后的内存块在哪个桶中？
>
> <img src="https://cdn.jsdelivr.net/gh/QinganZhang/ImageHosting/img/2024-03-11-11:31:56.png" alt="image-20240311113136119" style="zoom:50%;" />
>
> 对齐后的内存块大小 alignedBytes = RoundUp(bytes) 
>
> 对齐后的内存块所在桶的下标 index = Index*(bytes)

###### 方法

- Allocate：申请内存
    - 如果对应桶的自由链表不为空，则从自由链表中pop一块内存
    - 否则FetchFromCentralCache，从central cache中申请一块内存
- Deallocate：释放对象，回收内存
    - 将回收的内存块挂在对应桶的自由链表上
    - 如果自由链表过程，则ReturnBackToCentralCache，将一部分的内存归还到central cache
- FetchFromCentralCache：从central cache中申请内存，添加到freeList
    - 调用CentralCache的FetchRangeObj，从central cache中超额申请，尝试多申请几块内存（）
- ReturnBackToCentralCache：当freeList过于长时，将一部分的内存归还到central cache

##### thread cache TLS 无锁访问

  每个线程都要有自己的thread cache，而且要求是单个线程独享的，因此需要用到线程局部存储 TLS(Thread Local Storage)，这是一种变量的存储方法，使用该存储方法的变量在它所在的线程是全局可访问的，但是不能被其他线程访问到，这样就保持了数据的线程独立性。

```
//TLS - Thread Local Storage
static _declspec(thread) ThreadCache* pTLSThreadCache = nullptr;
// or
static thread_local ThreadCache* pTLSThreadCache = nullptr;
```

#### central cache
##### central cache 整体设计

- central cache 与 thread cache 的相同之处
    - central cache 的结构与 thread cache 是一样的，它们都是哈希桶的结构，并且它们遵循的对齐映射规则都是一样的。这样做的好处就是，当 thread cache 的某个桶中没有内存了，就可以直接到 central cache 中对应的哈希桶里去取内存就行了。

- central cache 与 thread cache 的不同之处

    - thread cache 是每个线程独享的，而 central cache 是所有线程共享的，因为每个线程的 thread cache 没有内存了都会去找 central cache，因此在访问 central cache 时是需要加锁的。但 central cache 在加锁时并不是将整个 central cache 全部锁上了，central cache 在加锁时用的是桶锁，也就是说每个桶都有一个锁。此时只有当多个线程同时访问 central cache 的同一个桶时才会存在锁竞争，如果是多个线程同时访问 central cache 的不同桶就不会存在锁竞争。

    - thread cache 的每个桶中挂的是一个个切好的内存块，而 central cache 的每个桶中挂的是一个个的 span。

        <img src="https://cdn.jsdelivr.net/gh/QinganZhang/ImageHosting/img/2024-03-11-21:06:14.png" style="zoom:40%;" />

###### 数据结构（类的数据成员）

- 数据结构Span：

    - 每个 span 管理的都是一个以页为单位的大块内存，每个 span 里面还有一个自由链表，这个自由链表里面挂的就是一个个切好了的内存块，根据span所在的哈希桶这些内存块被切成了对应的大小。

        ```cpp
        // Span本身不保存内存空间，其内部_freeList指向切好的小块内存，同时通过_pageId可以隐含的得到整个大块内存，相当于Span与内存块相对应
        struct Span{
            PAGE_ID _pageId = 0;        //大块内存起始页的页号，目的是方便page cache进行前后页的合并
        	size_t _n = 0;              //这个Span有几个页
        
        	Span* _next = nullptr;      //双链表结构
        	Span* _prev = nullptr;
        
        	size_t _useCount = 0;       //切好的小块内存，被分配给thread cache的计数。如果计数为0，代表当前span切出去的内存块都还回来了，因此central cache可以再将这个span还给page cache
        	void* _freeList = nullptr;  //切好的小块内存的自由链表
            size_t _objBytes = 0;    // 切好的小块内存的大小
        
            bool _isUse = false; // if true, span is on central cache; if false, span is on page cache
                // page cache 回收得到一个span时，尝试将相邻的span合并起来，前提是相邻的span在page cache中
                // 比如向前向后通过页号的加减得到相邻的span时，该span可能在page cache中，也可能在central cache中
                // 如果该span的_useCount!=0，则它肯定在central cache中；但是如果_useCount==0，则有可能是该span已经从central cache中回收到page cache，
                //      也可能是该span就没有分出去过，但是也有可能该span刚刚分给central cache，还没有使用
                // 因此不能通过_useCount来判断该span在page cache还是在central cache上
        };
        ```

    - 每个桶里面的若干 span 是按照双链表的形式链接起来的，形成SpanList

- 数据结构SpanList：

    - 将span安装双链表形式连接起来
    - 一个桶锁

- 类成员：SpanList[]，数组中每个元素（桶）都是一个SpanList双链表

###### 方法

- GetInstance：单例模式，返回对象

    - central cache和page cache在整个进程中只有一个，因此使用单例模式

- FetchRangeObj：从central cache中尝试获取n个alignedBytes大小的内存块

    - 调用GetOneSpan，从对应哈希桶中获取一个非空的span
    - 尝试获取n个内存块，不够也没关系

- GetOneSpan：从central cache中alignedBytes大小内存块对应的哈希桶中寻找一个非空的span

    - 从对应的哈希桶（spanlist）中寻找一个非空的span，找到就返回
    - 如果没有非空的span，则调用PageCache的NewSpan，从page cache中获取一个k页大小的span
    - 然后将这个span对应的大块内存切成小块内存，挂到span的freelist上，最后返回这个span

- ReturnRangeObj：从thread cache中回收n个alignedBytes大小的内存块

    - 内存块可以从MapObj2Span，对应到span，该span一定在central cache中

    - 将内存块添加到span中，如果该span中所有分出去的内存块都已经回收，则调用PageCache的ReturnSpanToPageCache，将该span归还给page cache


#### page cache

##### page cache 整体设计

- page cache 与 central cache 结构的相同之处
    - page cache 与 central cache 一样，它们都是哈希桶的结构，并且 page cache 的每个哈希桶中里挂的也是一个个的 span，这些 span 也是按照双链表的结构链接起来的。

- page cache 与 central cache 结构的不同之处

    - central cache 的映射规则与 thread cache 保持一致，而 page cache 的映射规则与它们都不相同。page cache 的哈希桶映射规则采用的是直接定址法，比如 1 号桶挂的都是 1 页的 span，2 号桶挂的都是 2 页的 span，以此类推。

    - central cache 每个桶中的 span 被切成了一个个对应大小的对象，以供 thread cache 申请；而 page cache 当中的 span 是没有被进一步切小的，因为 page cache 服务的是 central cache，当 central cache 没有 span 时，向 page cache 申请的是某一固定页数的 span，而如何切分申请到的这个 span 就应该由 central cache 自己来决定。  

        <img src="https://cdn.jsdelivr.net/gh/QinganZhang/ImageHosting/img/2024-03-11-16:03:49.png" style="zoom: 40%;" />

###### 数据结构（类的数据成员）

- 类成员：SpanList[]，数组中每个元素（桶）都是一个SpanList双链表
- 类成员：ObjectPool\<Span>，针对Span结构的定长内存池，因为每次分配span，最终都是在page cache中
- 类成员：_page2SpanMap，数据类型为unordered\_map或者基数树，实现内存页号到Span的映射，便于找到某个内存页在哪个span上
- 类成员：mutex，page cache的锁
    - 当每个线程的 thread cache 没有内存时都会向 central cache 申请，此时多个线程的 thread cache 如果访问的不是 central cache 的同一个桶，那么这些线程是可以同时进行访问的。这时 central cache 的多个桶就可能同时向 page cache 申请内存的，所以 page cache 也是存在线程安全问题的，因此在访问 page cache 时也必须要加锁。
    - 但是在 page cache 这里我们不能使用桶锁，因为当 central cache 向 page cache 申请内存时，page cache 可能会将其他桶当中大页的 span 切小后再给 central cache。此外，当 central cache 将某个 span 归还给 page cache 时，page cache 也会尝试将该 span 与其他桶当中的 span 进行合并。
    - 也就是说，在访问 page cache 时，我们可能需要访问 page cache 中的多个桶，如果 page cache 用桶锁就会出现大量频繁的加锁和解锁，导致程序的效率低下。因此我们在访问 page cache 时使用没有使用桶锁，而是用一个大锁将整个 page cache 给锁住。
    - 而 thread cache 在访问 central cache 时，只需要访问 central cache 中对应的哈希桶就行了，因为 central cache 的每个哈希桶中的 span 都被切分成了对应大小，thread cache 只需要根据自己所需对象的大小访问 central cache 中对应的哈希桶即可，不会访问其他哈希桶，因此 central cache 可以用桶锁。

###### 方法

- GetInstance：单例模式，返回对象

    - central cache和page cache在整个进程中只有一个，因此使用单例模式

- NewSpan：返回一个包含k个page的Span

    - 如果要分配的内存都大于page cache的管理范围，则直接向堆申请，返回

    - 否则向page cache的第k个桶申请一个span

        - 如果第k个桶不为空，则返回一个span，这个span有k个页
        - 否则继续在后面的大桶中寻找，如果遇到某个大桶spanlist不为空，则将这个大span拆分为两个span，一个k个页的span返回，另一个span插入到page cache的对应桶中，返回k个页的span

    - 如果剩下的桶都是空的，则向堆申请一个page cache可以管理的最大的span，递归调用NewSpan，此时一定可以返回一个span

        >   这里其实有一个问题：当 central cache 向 page cache 申请内存时，central cache 对应的哈希桶是处于加锁的状态的，那在访问 page cache 之前我们应不应该把 central cache 对应的桶锁解掉呢？
        >
        >   这里建议在访问 page cache 前，先把 central cache 对应的桶锁解掉。虽然此时 central cache 的这个桶当中是没有内存供其他 thread cache 申请的，但 thread cache 除了申请内存还会释放内存，如果在访问 page cache 前将 central cache 对应的桶锁解掉，那么此时当其他 thread cache 想要归还内存到 central cache 的这个桶时就不会被阻塞。
        >
        >   因此在调用 NewSpan 函数之前，我们需要先将 central cache 对应的桶锁解掉，然后再将 page cache 的大锁加上，当申请到 k 页的 span 后，我们需要将 page cache 的大锁解掉，但此时我们不需要立刻获取到 central cache 中对应的桶锁。因为 central cache 拿到 k 页的 span 后还会对其进行切分操作，因此我们可以在 span 切好后需要将其挂到 central cache 对应的桶上时，再获取对应的桶锁。
        >
        >   这里为了让代码清晰一点，只写出了加锁和解锁的逻辑，我们只需要将这些逻辑添加到之前实现的 GetOneSpan 函数的对应位置即可。
        >
        > ```cpp
        > spanList._mtx.unlock(); //解桶锁
        > PageCache::GetInstance()->_pageMtx.lock(); //加大锁
        > PageCache::GetInstance()->NewSpan(k); //从page cache申请k页的span
        > PageCache::GetInstance()->_pageMtx.unlock(); //解大锁
        > //进行span的切分...
        > 
        > spanList._mtx.lock(); //加桶锁
        > //将span挂到central cache对应的哈希桶
        > ```

- MapObj2Span：给定一个对象，返回它应该在哪个span上

- ReturnSpanToPageCache：回收空闲的span（从central cache释放的span）到page cache，并合并相邻的span

    - 如果这个span大到page cache都无法管理，则直接归还给堆，返回
    - 尝试将该span向前向后合并成一个更大的span
        - 获取当前span页号的前一个页prevpage，如果prevpage也在page cache中，则不断进行合并
        - 获取当前span页号的后一个页nextpage，如果nextpage也在page cache中，则不断进行合并
        - 合并成更大的span，插入到page cache对应的桶中

#### 外部接口的封装

- 申请内存
    - 如果申请的内存超过了thread cache的管理范围，则调用PageCache的NewSpan，直接向page cache申请一个span，返回对应页号的内存
        - 如果申请的内存超过了page cache的管理范围，则NewSpan中直接向堆申请内存
    - 否则通过线程的局部存储变量pTLSThreadCache，调用thread cache的Allocate函数
- 释放内存
    - 如果释放的内存超过了thread cache的管理范围，则调用PageCache的ReturnSpanToPageCache，直接向page cache释放一个span
        - 如果释放的内存超过了page cache的管理范围，则NewSpan中直接向堆释放内存
    - 否则通过线程的局部存储变量pTLSThreadCache，调用thread cache的Deallocate函数

### 优化与完善

#### 使用定长内存池构造对象

在新建Span、ThreadCache等数据结构时，可以使用定长内存池进行构建，不使用new或malloc，避免依赖于malloc

#### 释放对象时，不传入对象大小

在使用PageCache的MapObj2Span时，可以实现从对象所在内存页号到Span的映射，如果span中添加一个表示span中内存块大小的成员，就可以在释放对象时避免传入对象大小，实现与free类似的形式

#### 使用基数树进行优化

通过性能分析，发现当前瓶颈在PageCache::MapObj2Span的锁竞争上。可以使用基数树进行优化，此时可以做到在访问基数树时不加锁。

##### 基数树


基数树实际上就是一个分层的哈希表，根据所分层数不同可分为单层基数树、二层基数树、三层基数树等，可以替换掉unordered_map

- 单层基数树

    ​		单层基数树实际采用的就是直接定址法，每一个页号对应 span 的地址就存储数组中在以该页号为下标的位置。最坏的情况下我们需要建立所有页号与其 span 之间的映射关系，因此这个数组中元素个数应该与页号的数目相同，数组中每个位置存储的就是对应 span 的指针。

    <img src="https://cdn.jsdelivr.net/gh/QinganZhang/ImageHosting/img/2024-03-11-21:06:24.png" style="zoom: 30%;" />

- 二层基数树

    ​		这里还是以 32 位平台下，一页的大小为 4K 为例来说明，此时存储页号最多需要 20 个比特位。而二层基数树实际上就是把这 20 个比特位分为两次进行映射。比如用前 6 个比特位在基数树的第一层进行映射，映射后得到对应的第二层，然后用剩下的比特位在基数树的第二层进行映射，映射后最终得到该页号对应的 span 指针。 

    <img src="https://cdn.jsdelivr.net/gh/QinganZhang/ImageHosting/img/2024-03-11-13:42:34.png" style="zoom:30%;" />

    ​		二层基数树相比一层基数树的好处就是，一层基数树必须一开始就把整个数组开辟出来，而二层基数树一开始时只需将第一层的数组开辟出来，当需要进行某一页号映射时再开辟对应的第二层的数组就行了。

- 三层基数树

    ​		上面一层基数树和二层基数树都适用于 32 位平台，而对于 64 位的平台就需要用三层基数树了。三层基数树与二层基数树类似，三层基数树实际上就是把存储页号的若干比特位分为三次进行映射。  

    <img src="https://cdn.jsdelivr.net/gh/QinganZhang/ImageHosting/img/2024-03-11-13:44:16.png" style="zoom:33%;" />

    ​		此时只有当要建立某一页号的映射关系时，再开辟对应的数组空间，而没有建立映射的页号就可以不用开辟其对应的数组空间，此时就能在一定程度上节省内存空间。

##### 分析

```cpp
Span* PageCache::MapObj2Span(void* obj) //获取从对象到span的映射
{
	PAGE_ID id = (PAGE_ID)obj >> PAGE_SHIFT; //页号
	std::unique_lock<std::mutex> lock(_pageMtx); //构造时加锁，析构时自动解锁
    Span* ret = _page2SpanMap.get(pageId);
    assert(ret != nullptr);
    return ret;
}
```

- 为什么使用map或者unordered_map时需要加锁？

    ​		对于MapObj2Span而言，如果当前正在page cache中使用MapObj2Span，因为进入到page cache都要加锁，所以此时MapObj2Span内部无需加锁。

    ​		但是如果当前线程是在CentralCache或者ConcurrentFree中使用MapObj2Span，与此同时另一个线程进入page cache中，正在修改这个映射关系，因为 C++ 中 map 的底层数据结构是红黑树，unordered_map 的底层数据结构是哈希表，插入数据时其底层的结构都有可能会发生变化（比如红黑树在插入数据时可能会引起树的旋转，而哈希表在插入数据时可能会引起哈希表扩容），可能出现数据不一致的问题，此时访问这个映射关系就需要加速

- 为什么使用基数树就不用加锁呢？

    ​		基数树的空间一旦开辟好了就不会发生变化，因此无论什么时候去读取某个页的映射，都是对应在一个固定的位置进行读取的。并且我们不会同时对同一个页进行读取映射和建立映射的操作，因为我们只有在释放对象时才需要读取映射，而建立映射的操作都是在 page cache 进行的。也就是说，读取映射时读取的都是对应 span 的\_useCount 不等于 0 的页，而建立映射时建立的都是对应 span 的\_useCount 等于 0 的页，所以说我们不会同时对同一个页进行读取映射和建立映射的操作。

#### 打包和构建

​		实际 Google 开源的 tcmalloc 是会直接用于替换 malloc 的，不同平台替换的方式不同。比如基于 Unix 的系统上的 glibc，使用了 weak alias 的方式替换；而对于某些其他平台，需要使用 hook 的钩子技术来做。

​		一般使用时也可以构建生成动态链接库进行调用。使用cmake进行构建，生成测试的可执行文件，并生成静态链接库和动态链接库，同时`make install`可以在构建时自定义安装路径进行安装（也可以不安装）如果别的文件想使用MyConcurrentMemoryPool，可以将include目录下的头文件和src目录下的源码拷贝过去源码编译，也可以使用生成好的链接库（注意静态链接库和动态链接库不要放在一个文件中，否则会出现多重定义的错误）

申请内存过程分析
----------------

![图片1](https://cdn.jsdelivr.net/gh/QinganZhang/ImageHosting/img/2024-03-11-20:42:40.png)


释放内存过程分析
----------------

![图片2](https://cdn.jsdelivr.net/gh/QinganZhang/ImageHosting/img/2024-03-11-21:01:59.png)

   
