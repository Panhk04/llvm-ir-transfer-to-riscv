//
// Created by start_0916 on 23-7-19.
//
#include "../../include/mir/CloneInfoMap.h"
#include "../../include/util/HashMap.h"

std::map<Loop *, Loop *> CloneInfoMap::loopMap;
std::map<Value *, Value *> CloneInfoMap::valueMap;
std::map<int, int> CloneInfoMap::loopCondCntMap;

void CloneInfoMap::addLoopReflect(Loop *srcLoop, Loop *tagLoop) {
    loopMap[srcLoop] = tagLoop;
}

Loop *CloneInfoMap::getReflectedLoop(Loop *loop) {
    auto iter = loopMap.find(loop);
    if (iter != loopMap.end()) {
        return iter->second;
    }
    return loop;
}

void CloneInfoMap::addLoopCondCntReflect(int src, int tag) {
    loopCondCntMap[src] = tag;
}

int CloneInfoMap::getLoopCondCntReflect(int cnt) {
    auto iter = loopCondCntMap.find(cnt);
    if (iter != loopCondCntMap.end()) {
        return iter->second;
    }
    return cnt;
}

void CloneInfoMap::addValueReflect(Value *src, Value *tag) {
//    if (src->getName() == "%v114") {
//        printf("%s\n", "123");
//    }
    valueMap[src] = tag;
}

Value *CloneInfoMap::getReflectedValue(Value *value) {
    auto iter = valueMap.find(value);
    if (iter != valueMap.end()) {
        return iter->second;
    }
    return value;
}

void CloneInfoMap::clear() {
    loopMap.clear();
    valueMap.clear();
    loopCondCntMap.clear();
}
