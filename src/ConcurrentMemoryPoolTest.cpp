#include "ConcurrentAlloc.h"
#include "Utils.h"
#include "Common.h"
#include <random>
#include <unordered_map>
#include <thread>
#include <cstring>
#include <sstream>

void RandomMallocTest(size_t nThreads, size_t nRounds, std::vector<int>& lst, bool show = true) {
    std::vector<std::thread> myThreads(nThreads);
    std::atomic<size_t> totalTime(0);
    for (size_t k = 0; k < nThreads; ++k) {
        myThreads[k] = std::thread(
            [&, k]() {
                for(size_t round = 0; round < nRounds; ++round){
                    std::unordered_map<int, void*> ump; // size map to address
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
                    std::ostringstream oss;
                    oss << std::this_thread::get_id() << endl;
                    if(show) printf("round = %lu, thread_id = %s (total %lu threads), malloc/free total %lu times, spend time: %lf ms\n", 
                        round, oss.str().c_str(), nThreads, lst.size(), kThreadTestTime.load() / (double)CLOCKS_PER_SEC);

                }
            }
        );
    }
    for (auto& t : myThreads) {
        t.join();
    }

    printf(" %lu threads run %lu rounds: in each round, thread malloc/free [ %lu / %lu ](per/total) times, total spend time: %lf ms\n", 
        nThreads, nRounds, lst.size(), lst.size() * nThreads, totalTime.load() / (double)CLOCKS_PER_SEC);
}

void RandomConcurrentMallocTest(size_t nThreads, size_t nRounds, std::vector<int>& lst, bool show = true) {
    std::vector<std::thread> myThreads(nThreads);
    std::atomic<size_t> totalTime(0);
    for (size_t k = 0; k < nThreads; ++k) {
        myThreads[k] = std::thread(
            [&, k]() {
                for(size_t round = 0; round < nRounds; ++round){
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
                    kThreadTestTime = clock() - begin;
                    totalTime += (clock() - begin);
                    std::ostringstream oss;
                    oss << std::this_thread::get_id() << endl;
                    if(show) printf("round = %lu, thread_id = %s (total %lu threads), concurrently malloc/free total %lu times, spend time: %lf ms\n", 
                        round, oss.str().c_str(), nThreads, lst.size(), kThreadTestTime.load() / (double)CLOCKS_PER_SEC);
                }
            }
        );
    }
    for (auto& t : myThreads) {
        t.join();
    }
    printf(" %lu threads run %lu rounds: in each round, thread concurrently malloc/free [ %lu / %lu ](per/total) times, total spend time: %lf ms\n", 
        nThreads, nRounds, lst.size(), lst.size() * nThreads, totalTime.load() / (double)CLOCKS_PER_SEC);
}

void RandomSingleConcurrentMallocTest(std::vector<int>& lst, bool show = true) {
    std::unordered_map<int, void*> ump;
    std::atomic<size_t> kThreadTestTime(0);
    size_t begin = clock();
    for (size_t i = 0; i < lst.size(); ++i) {
        if (lst[i] > 0) {
            if(show) cout << "  alloc " << lst[i] << endl;
            ump[lst[i]] = ConcurrentAlloc(lst[i]);
        }
        else // lst[i] < 0
        {
            if(show) cout << "  dealloc " << lst[i] << endl;
            ConcurrentFree(ump[-lst[i]]);
        }
    }
    kThreadTestTime = clock() - begin;
    printf("single thread alloc/dealloc total %lu times, spend time: %lf ms\n", 
        lst.size(), kThreadTestTime.load() / (double)CLOCKS_PER_SEC);
}

int main(){
    size_t nThreads = 4;
    size_t nRounds = 2;
    // size_t objBytes = 16;
    size_t len = 500 * 2;
    size_t allocMinBytes = 2;
    size_t allocMaxBytes = 128 * 6 * 1024;
    std::vector<int> v = GenList(len / 2, allocMinBytes, allocMaxBytes);
    
    cout << endl << "BenchMark " << nThreads << " Threads Malloc/Free" << endl;
    RandomMallocTest(nThreads, nRounds, v, false);
    // BenchMarkMalloc(nTimes, nThreads, nRounds, objBytes, v);

    cout << endl << "BenchMark " << nThreads << " Threads ConcurrentMalloc/ConcurrentFree" << endl;
    RandomConcurrentMallocTest(nThreads, nRounds, v, false);
    // BenchMarkConcurrentMalloc(nTimes, nThreads, nRounds, objBytes, v);

    // cout << endl << "BenchMark Single Thread ConcurrentMalloc/ConcurrentFree" << endl;
    // RandomSingleConcurrentMallocTest(v, false);

    cout<< "over " << endl;
    return 0;
}