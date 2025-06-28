//
// Created by start_0916 on 23-7-19.
//

#ifndef MAIN_HASHMAP_H
#define MAIN_HASHMAP_H

#include "vector"
#include "map"
#include "unordered_map"

template<typename T, typename _T>
std::vector<T> KeySet(std::map<T, _T> test)
{
    std::vector<T> keys;
    for(auto it = test.begin(); it != test.end(); ++it){
        keys.push_back(it->first);
    }
    return keys;
}

template<typename T, typename _T>
std::vector<_T> ValueSet(std::map<T, _T> test)
{
    std::vector<_T> keys;
    for(auto it = test.begin(); it != test.end(); ++it){
        keys.push_back(it->second);
    }
    return keys;
}

template<typename T, typename _T>
std::vector<T> KeySet(std::unordered_map<T, _T> test)
{
    std::vector<T> keys;
    for(auto it = test.begin(); it != test.end(); ++it){
        keys.push_back(it->first);
    }
    return keys;
}

template<typename T, typename _T>
std::vector<_T> ValueSet(std::unordered_map<T, _T> test)
{
    std::vector<_T> keys;
    for(auto it = test.begin(); it != test.end(); ++it){
        keys.push_back(it->second);
    }
    return keys;
}

#endif //MAIN_HASHMAP_H
