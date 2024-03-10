#pragma once

#include <random>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <iostream>
#include <algorithm>
#include <cassert>
using std::cout;
using std::endl;

std::vector<int> GenList(size_t len, double ratio = 0.01, size_t sampleMin = 8, size_t sampleMax = 256 * 1024, double mean = 512.0, double std = 100.0, size_t maxAllocThreshold = 1024 * 1024){
    std::random_device rd;
    std::mt19937 gen(rd());
    
    std::uniform_int_distribution<size_t> uni_dis(sampleMin, sampleMax);
    std::normal_distribution<double> dis(mean, std);

    std::vector<int> v;
    v.reserve(2 * len);

    size_t uniform_len = len * ratio > 5 ? len * ratio : 5;
    size_t norm_len = len - uniform_len;
    size_t allAlloc = 0;

    // 保证每次申请的内存大小不重复
    std::unordered_set<size_t> ust;

    // 大部分正态分布
    for(size_t i = 0; i < norm_len; ++i){
        int val = (int)dis(gen);
        if(val < sampleMin) continue;
        if(val > sampleMax) continue;
        if(ust.find(val) != ust.end()) continue;
        else{
            v.push_back(val);
            allAlloc += val;
            v.push_back(val * (-1));
            ust.insert(val);
            ++i;
        }
    }
    // 小部分均匀分布
    for(size_t i = 0; i < uniform_len; ++i){
        size_t val = uni_dis(gen);
        if(ust.find(val) != ust.end()) continue;
        else{
            v.push_back(val);
            allAlloc += val;
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

    int maxAlloc = 0, nowAlloc = 0;
    for(int val: v){
        nowAlloc += val;
        maxAlloc = maxAlloc > nowAlloc ? maxAlloc : nowAlloc;
    }

    if(maxAlloc > maxAllocThreshold){
        cout << "allAlloc = " << (double)allAlloc / 1024 << " KB, maxAlloc = " << (double)maxAlloc / 1024 << " KB, more than threshold = " << (double)maxAllocThreshold / 1024 << " KB" << endl;
        assert(false);
        // return GenList(len, ratio, sampleMin, sampleMax, mean, std, maxAllocThreshold);
    }
    else{
        cout << "generate random alloc list, statics: " << endl;
        cout << "  len = " << len << ", allAlloc = " << (double)allAlloc / 1024 << " KB, maxAlloc = " << (double)maxAlloc / 1024 << " KB, less than threshold = " << (double)maxAllocThreshold / 1024 << " KB" << endl;
    }
    return v;
}