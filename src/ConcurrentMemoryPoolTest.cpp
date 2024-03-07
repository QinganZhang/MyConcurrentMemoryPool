#include "ConcurrentAlloc.h"
#include "Utils.h"
#include <random>
#include <unordered_map>

void AllocTest1(){
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dis(2, 1024);
    for(size_t i = 0; i < 10; ++i){
        size_t size = dis(gen);
        cout<< "size:" << size << endl;
        void* ptr = ConcurrentAlloc(size);
    }
}


void BenchMarkMalloc(size_t nTimes, size_t nThreads, size_t nRounds, size_t objBytes, std::vector<int> lst){
    std::vector<std::thread> threads(nThreads);
    std::atomic<size_t> malloc_costTime(0);
    std::atomic<size_t> free_costTime(0);
    std::atomic<size_t> testTime(0);
    
    for(size_t k = 0; k < nThreads; ++k){
        threads[k] = std::thread(
            [&, k]() {
                std::vector<void*> v;
                v.reserve(nTimes);
                for(size_t round = 0; round < nRounds; ++round){
                    
                    size_t begin1 = clock();
                    for (size_t i = 0; i < nTimes; i++)
                    {
                        v.push_back(malloc(objBytes + round * 10));
                    }
                    size_t end1 = clock();

                    size_t begin2 = clock();
                    for (size_t i = 0; i < nTimes; i++)
                    {
                        free(v[i]);
                    }
                    size_t end2 = clock();
                    v.clear();

                    malloc_costTime += (end1 - begin1);
                    free_costTime += (end2 - begin2);

                    std::unordered_map<int, void*> ump;
                    size_t begin = clock();
                    for(size_t i = 0; i < lst.size(); ++i){
                        if(lst[i] > 0)
                            ump[lst[i]] = malloc(lst[i]);
                        else // lst[i] < 0
                            free(ump[lst[i]]);
                    }
                    size_t end = clock();
                    testTime += (end - begin);
                }
            }
        );
    }

    for(auto& t: threads){
        t.join();
    }

    printf("%u threads concurrent run %u rounds, per round malloc %u times, spend time: %u ms\n",
		nThreads, nRounds, nTimes, malloc_costTime.load());
	printf("%u threads concurrent run %u rounds, per round free %u times, spend time: %u ms\n",
		nThreads, nRounds, nTimes, free_costTime.load());
	printf("%u threads concurrent malloc&free %u rounds, total spend time: %u ms\n",
		nThreads, nThreads*nRounds*nTimes, malloc_costTime.load() + free_costTime.load());
    printf("%u threads concurrently and randomly malloc&free %u rounds, per round malloc/free %u times, spend time: %u ms\n",
        nThreads, nRounds, lst.size()/2, testTime.load() );
}

void BenchMarkConcurrentMalloc(size_t nTimes, size_t nThreads, size_t nRounds, size_t objBytes, std::vector<int> lst){
    std::vector<std::thread> threads(nThreads);
    std::atomic<size_t> malloc_costTime(0);
    std::atomic<size_t> free_costTime(0);
    std::atomic<size_t> testTime(0);
    
    for(size_t k = 0; k < nThreads; ++k){
        threads[k] = std::thread(
            [&, k]() {
                std::vector<void*> v;
                v.reserve(nTimes);
                for(size_t round = 0; round < nRounds; ++round){
                    
                    size_t begin1 = clock();
                    for (size_t i = 0; i < nTimes; i++)
                    {
                        v.push_back(ConcurrentAlloc(objBytes + round * 10));
                    }
                    size_t end1 = clock();

                    size_t begin2 = clock();
                    for (size_t i = 0; i < nTimes; i++)
                    {
                        ConcurrentFree(v[i]);
                    }
                    size_t end2 = clock();
                    v.clear();

                    malloc_costTime += (end1 - begin1);
                    free_costTime += (end2 - begin2);

                    std::unordered_map<int, void*> ump;
                    size_t begin = clock();
                    for(size_t i = 0; i < lst.size(); ++i){
                        if(lst[i] > 0)
                            ump[lst[i]] = ConcurrentAlloc(lst[i]);
                        else // lst[i] < 0
                            ConcurrentFree(ump[lst[i]]);
                    }
                    size_t end = clock();
                    testTime += (end - begin);
                }
            }
        );
    }

    for(auto& t: threads){
        t.join();
    }

    printf("%u threads concurrent run %u rounds, per round malloc %u times, spend time: %u ms\n",
		nThreads, nRounds, nTimes, malloc_costTime.load());
	printf("%u threads concurrent run %u rounds, per round free %u times, spend time: %u ms\n",
		nThreads, nRounds, nTimes, free_costTime.load());
	printf("%u threads concurrent malloc&free %u rounds, total spend time: %u ms\n",
		nThreads, nThreads*nRounds*nTimes, malloc_costTime.load() + free_costTime.load());
    printf("%u threads concurrently and randomly malloc&free %u rounds, per round malloc/free %u times, spend time: %u ms\n",
        nThreads, nRounds, lst.size()/2, testTime.load() );
}


int main(){
    size_t nThreads = 4;
    size_t nTimes = 1000;
    size_t nRounds = 5;
    size_t objBytes = 16; 
    size_t len = 1000 * 2;
    std::vector<int> v = GenList(len);

    cout<< "BenchMark Malloc/Free" << endl;
    BenchMarkMalloc(nTimes, nThreads, nRounds, objBytes, v);

    cout<< endl << "BenchMark ConcurrentMalloc/ConcurrentFree" << endl;
    BenchMarkConcurrentMalloc(nTimes, nThreads, nRounds, objBytes, v);

    Sleep(2);
    return 0;
}