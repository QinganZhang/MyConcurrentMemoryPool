#include "ConcurrentAlloc.h"
#include "Utils.h"
#include "Common.h"
#include <random>
#include <unordered_map>
#include <thread>

void AllocTest(){
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dis(2, 1024);
    for(size_t i = 0; i < 10; ++i){
        size_t size = dis(gen);
        cout<< "size:" << size << endl;
        void* ptr = ConcurrentAlloc(size);
    }
}

void RandomMallocTest(size_t nThreads, std::vector<int>& lst, bool show = true) {
    std::vector<std::thread> myThreads(nThreads);
    std::atomic<size_t> totalTime(0);
    for (size_t k = 0; k < nThreads; ++k) {
        myThreads[k] = std::thread(
            [&, k]() {
                std::unordered_map<int, void*> ump;
                std::atomic<size_t> kThreadTestTime(0);
                size_t begin = clock();
                for (size_t i = 0; i < lst.size(); ++i) {
                    if (lst[i] > 0) {
                        if(show) cout << "thread = " << k << "  alloc " << lst[i] << endl;
                        ump[lst[i]] = malloc(lst[i]);
                    }
                    else // lst[i] < 0
                    {
                        if(show) cout << "thread = " << k << "  dealloc " << lst[i] << endl;
                        free(ump[-lst[i]]);
                    }
                }
                kThreadTestTime = clock() - begin;
                totalTime += (clock() - begin);
                if(show) printf("thread_id = %u (total %lu threads), alloc/dealloc totoal %lu times, spend time: %lu ms\n", 
                    k, nThreads, lst.size(), kThreadTestTime.load() / CLOCKS_PER_SEC);
            }
        );
    }
    for (auto& t : myThreads) {
        t.join();
    }

    printf("total %lu threads, each thread alloc/dealloc total %lu times, total alloc/dealloc %lu times, total spend time: %lu ms\n", 
        nThreads, lst.size(), lst.size() * nThreads, totalTime.load() / CLOCKS_PER_SEC);
}

void RandomConcurrentMallocTest(size_t nThreads, std::vector<int>& lst, bool show = true) {
    std::vector<std::thread> myThreads(nThreads);
    std::atomic<size_t> totalTime(0);
    for (size_t k = 0; k < nThreads; ++k) {
        myThreads[k] = std::thread(
            [&, k]() {
                std::unordered_map<int, void*> ump;
                std::atomic<size_t> kThreadTestTime(0);
                size_t begin = clock();
                for (size_t i = 0; i < lst.size(); ++i) {
                    if (lst[i] > 0) {
                        if(show) cout << "thread = " << k << "  alloc " << lst[i] << endl;
                        ump[lst[i]] = ConcurrentAlloc(lst[i]);
                    }
                    else // lst[i] < 0
                    {
                        if(show) cout << "thread = " << k << "  dealloc " << lst[i] << endl;
                        ConcurrentFree(ump[-lst[i]]);
                    }
                }
                totalTime += (clock() - begin);
                if(show) printf("thread_id = %u (total %lu threads), alloc/dealloc totoal %lu times, spend time: %lu ms\n", 
                    k, nThreads, lst.size(), kThreadTestTime.load() / CLOCKS_PER_SEC);
            }
        );
    }
    for (auto& t : myThreads) {
        t.join();
    }
    printf("total %lu threads, each thread alloc/dealloc total %lu times, total alloc/dealloc %lu times, total spend time: %lu ms\n", 
        nThreads, lst.size(), lst.size() * nThreads, totalTime.load() / CLOCKS_PER_SEC);
}

int main(){
    size_t nThreads = 8;
    size_t nTimes = 1000;
    size_t nRounds = 5;
    size_t objBytes = 16; 
    size_t len = 2000 * 2;
    size_t allocMaxBytes = 256 * 1024;
    std::vector<int> v = GenList(len / 2, allocMaxBytes);
    
    cout << "BenchMark Malloc/Free" << endl;
    RandomMallocTest(nThreads, v, false);
    // BenchMarkMalloc(nTimes, nThreads, nRounds, objBytes, v);

    cout<< endl << "BenchMark ConcurrentMalloc/ConcurrentFree" << endl;
    RandomConcurrentMallocTest(nThreads, v, false);
    // BenchMarkConcurrentMalloc(nTimes, nThreads, nRounds, objBytes, v);

    cout<< "over " << endl;
    return 0;
}