#include "ConcurrentAlloc.h"
#include "random"

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

void AllocTest2(){
    for (size_t i = 0; i < 1024; i++)
    {
        void* p1 = ConcurrentAlloc(6);
        cout<< "i = " << i <<endl;
    }
    void* p2 = ConcurrentAlloc(6);
}
int main(){
    AllocTest2();
    return 0;
}