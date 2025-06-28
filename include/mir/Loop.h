//
// Created by Administrator on 2023/7/15.
//

#ifndef FASTER_MEOW_LOOP_H
#define FASTER_MEOW_LOOP_H

#include <set>
#include "../util/util.h"

class BasicBlock;
class Function;

#include "../../include/mir/Value.h"
#include "../../include/mir/Instr.h"
#include "../../include/mir/Function.h"
#include "../../include/mir/BasicBlock.h"
#include "../../include/mir/Constant.h"
#include "../../include/mir/Use.h"
#include "../../include/mir/GlobalVal.h"
#include "../../include/mir/Loop.h"
#include "../../include/mir/CloneInfoMap.h"
#include <vector>
#include <unordered_map>
#include <set>
#include <algorithm>
class Loop {
public:
    static Loop* emptyLoop;
    static int loop_num;
    Loop();
    std::string funcName = "";
    std::string name = "";
    Loop* parentLoop;
    int loopDepth = -1;
    std::set<Loop*>* childrenLoops = new std::set<Loop*>();
    int hash;
    std::set<BasicBlock*>* nowLevelBB = new std::set<BasicBlock*>();
    BasicBlock* header;
    std::set<BasicBlock*>* enterings = new std::set<BasicBlock*>();
    std::set<BasicBlock*>* exitings = new std::set<BasicBlock*>();
    std::set<BasicBlock*>* latchs = new std::set<BasicBlock*>();
    std::set<BasicBlock*>* exits = new std::set<BasicBlock*>();
    std::unordered_map<int, std::set<Instr*>*>* conds = new std::unordered_map<int, std::set<Instr*>*>();
    Value* idcAlu;
    Value* idcPHI;
    Value* idcCmp;
    Value* idcInit;
    Value* idcEnd;
    Value* idcStep;
    bool idcSet = false;
    int idcTimes;
    bool idcTimeSet = false;
    bool isCalcLoop = false;
    Value* aluPhiEnterValue;
    Value* calcAlu;
    Value* calcPhi;
    int aluOtherIndex;
    bool isAddAndModLoop = false;
    Value* modPhi;
    Value* addIS;
    Value* remIS;
    int init;
    int base;
    int mod;
    int addConstIndex;
    bool canAggressiveParallel = false;
    void setCanAggressiveParallel(bool canAggressiveParallel);
    bool isCanAggressiveParallel();
    void clearAddAndModLoopInfo();
    void setAddAndModLoopInfo(Value* modPhi, Value* addIS, Value* remIS, int init, int base, int mod, int addConstIndex);
    Value* getModPhi();
    Value* getAddIS();
    Value* getRemIS();
    int getInit();
    int getBase();
    int getMod();
    int getAddConstIndex();
    void clearCalcLoopInfo();
    void setCalcLoopInfo(Value* aluPhiEnterValue, Value* calcAlu, Value* calcPhi, int aluOtherIndex);
    Value* getAluPhiEnterValue();
    Value* getCalcAlu();
    Value* getCalcPhi();
    int getAluOtherIndex();
    bool isArrayInit = false;
    int arrayInitDims = 0;
    Value* initArray = nullptr;
    Value* initValue = nullptr;
    std::set<Instr*>* extras = nullptr;
    void setArrayInitInfo(int arrayInitDims, Value* initArray, Value* initValue, std::set<Instr*>* extras);
    std::set<Instr*>* getExtras();
    Value* getInitValue();
    int getArrayInitDims();
    Value* getInitArray();
    Loop(Loop* parentLoop);
    int getLoopDepth();
    void setParentLoop(Loop* parentLoop);
    Loop* getParentLoop();
    bool addChildLoop(Loop* childLoop);
    std::set<Loop*>* getChildrenLoops();
    bool addBB(BasicBlock* bb);
    void removeBB(BasicBlock* bb);
    BasicBlock* getHeader();
    void setHeader(BasicBlock* basicBlock);
    std::string toString();
    void setFunc(Function* function);
    void addEntering(BasicBlock* bb);
    void addLatch(BasicBlock* bb);
    void addExiting(BasicBlock* bb);
    void addExit(BasicBlock* bb);
    void addCond(Instr* instr);
    void clearCond();
    void clear();
    std::set<BasicBlock*>* getNowLevelBB();
    std::set<BasicBlock*>* getExits();
    std::unordered_map<int, std::set<Instr*>*>* getConds();
    std::set<BasicBlock*>* getEnterings();
    std::set<BasicBlock*>* getLatchs();
    std::set<BasicBlock*>* getExitings();
    void cloneToFunc(Function* function);
    void fix();
    bool isSimpleLoop();
    std::string infoString();
    void setIdc(Value* idcAlu, Value* idcPHI, Value* idcCmp, Value* idcInit, Value* idcEnd, Value* idcStep);
    Value* getIdcAlu();
    Value* getIdcPHI();
    Value* getIdcCmp();
    Value* getIdcInit();
    Value* getIdcEnd();
    Value* getIdcStep();
    bool isIdcSet();
    void clearIdcInfo();
    void setIdcTimes(int idcTimes);
    int getIdcTimes();
    bool isIdcTimeSet();
    bool hasChildLoop();
    virtual explicit operator std::string() const {return "!loop!";}
};
#endif //FASTER_MEOW_LOOP_H
