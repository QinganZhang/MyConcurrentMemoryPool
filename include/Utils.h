#pragma once

#include <random>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <iostream>
#include <algorithm>
using std::cout;
using std::endl;

std::vector<int> GenList(size_t len, size_t sampleMax = 256 * 1024){
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dis(2, sampleMax);

    std::vector<int> v;
    v.reserve(2 * len);

    // 保证每次申请的内存大小不重复
    std::unordered_set<size_t> ust;
    for(size_t i = 0; i < len; ){
        size_t val = dis(gen);
        if(ust.find(val) != ust.end()) continue;
        else{
            v.push_back(val);
            v.push_back(val * (-1));
            ust.insert(val);
            ++i;
        }
    }
    std::shuffle(v.begin(), v.end(), gen);

    std::unordered_map<int, int> ump;
    for (int i = 0; i < v.size(); ++i) {
        ump[v[i]] = i;
    }

    for (int i = 0; i < v.size(); ++i) {
        if (v[i] > 0) continue;
        else { // v[i] < 0
            int pos_idx = ump[-v[i]];
            if (pos_idx > i) {
                ump[v[i]] = pos_idx;
                ump[v[pos_idx]] = i;
                std::swap(v[i], v[pos_idx]);
            }
        }
    }

    //// 正数表示申请内存，负数表示释放内存
    //std::unordered_map<int, std::deque<int>> ump; // val to index
    //for(int i = 0; i < v.size(); ++i){
    //    if(ump.find(v[i]) == ump.end())
    //        ump[v[i]] = std::deque<int>(i);
    //    else 
    //        ump[v[i]].push_back(i);
    //}

    //for(int i = 0; i < v.size(); ++i){
    //    // cout<<i<<" "<<v[i]<<endl;
    //    if(ump[v[i]].empty() == true) continue;
    //    if(v[i] > 0) continue;
    //    else{ //v[i] < 0
    //        int pos_idx = ump[-v[i]].front();
    //        ump[-v[i]].pop_front();
    //        ump[v[i]].pop_front();
    //        if(i < pos_idx) std::swap(v[i], v[pos_idx]); 
    //    }
    //}

    return v;
}