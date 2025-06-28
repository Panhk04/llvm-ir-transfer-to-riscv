//
// Created by start_0916 on 23-7-19.
//

#ifndef MAIN_CLONEINFOMAP_H
#define MAIN_CLONEINFOMAP_H

#include <map>
#include "Loop.h"
#include "Value.h"
class CloneInfoMap {
public:
    static std::map<Loop*, Loop*> loopMap;
    static std::map<Value*, Value*> valueMap;
    static std::map<int, int> loopCondCntMap;
    static void addLoopReflect(Loop* srcLoop, Loop* tagLoop);
    static Loop* getReflectedLoop(Loop* loop);
    static void addLoopCondCntReflect(int src, int tag);
    static int getLoopCondCntReflect(int cnt);
    static void addValueReflect(Value* src, Value* tag);
    static Value* getReflectedValue(Value* value);
    static void clear();
};


#endif //MAIN_CLONEINFOMAP_H
