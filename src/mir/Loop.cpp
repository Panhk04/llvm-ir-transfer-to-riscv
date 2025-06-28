#include "../../include/mir/Loop.h"
#include "../../include/util/CenterControl.h"


Loop* Loop::emptyLoop = new Loop();
int Loop::loop_num = 0;

Loop::Loop() {
    this->hash = -1;
}




//同一循环内的BB 进一步划分为 entering header exiting latch exit
//entering为header的前驱



//归纳变量相关信息:








void Loop::setCanAggressiveParallel(bool canAggressiveParallel) {
    this->canAggressiveParallel = canAggressiveParallel;
}

bool Loop::isCanAggressiveParallel() {
    return canAggressiveParallel;
}

void Loop::clearAddAndModLoopInfo() {
    this->isAddAndModLoop = false;
    this->modPhi = nullptr;
    this->addIS = nullptr;
    this->remIS = nullptr;
    this->init = 0;
    this->base = 0;
    this->mod = 0;
    this->addConstIndex = 0;
}

void Loop::setAddAndModLoopInfo(Value* modPhi, Value* addIS, Value* remIS, int init, int base, int mod, int addConstIndex) {
    this->isAddAndModLoop = true;
    this->modPhi = modPhi;
    this->addIS = addIS;
    this->remIS = remIS;
    this->init = init;
    this->base = base;
    this->mod = mod;
    this->addConstIndex = addConstIndex;
}


Value* Loop::getModPhi() {
    return modPhi;
}

Value* Loop::getAddIS() {
    return addIS;
}

Value* Loop::getRemIS() {
    return remIS;
}

int Loop::getInit() {
    return init;
}

int Loop::getBase() {
    return base;
}

int Loop::getMod() {
    return mod;
}

int Loop::getAddConstIndex() {
    return addConstIndex;
}

void Loop::clearCalcLoopInfo() {
    isCalcLoop = false;
    this->aluPhiEnterValue = nullptr;
    this->calcAlu = nullptr;
    this->calcPhi = nullptr;
    this->aluOtherIndex = 0;
}

void Loop::setCalcLoopInfo(Value* aluPhiEnterValue, Value* calcAlu, Value* calcPhi, int aluOtherIndex) {
    isCalcLoop = true;
    this->aluPhiEnterValue = aluPhiEnterValue;
    this->calcAlu = calcAlu;
    this->calcPhi = calcPhi;
    this->aluOtherIndex = aluOtherIndex;
}


Value* Loop::getAluPhiEnterValue() {
    return aluPhiEnterValue;
}

Value* Loop::getCalcAlu() {
    return calcAlu;
}

Value* Loop::getCalcPhi() {
    return calcPhi;
}

int Loop::getAluOtherIndex() {
    return aluOtherIndex;
}


void Loop::setArrayInitInfo(int arrayInitDims, Value* initArray, Value* initValue, std::set<Instr*>* extras) {
    this->isArrayInit = true;
    this->arrayInitDims = arrayInitDims;
    this->initArray = initArray;
    this->initValue = initValue;
    this->extras = extras;
}

std::set<Instr*>* Loop::getExtras() {
    return extras;
}


Value* Loop::getInitValue() {
    return initValue;
}

int Loop::getArrayInitDims() {
    return arrayInitDims;
}

Value* Loop::getInitArray() {
    return initArray;
}

Loop::Loop(Loop* parentLoop) {
    this->hash = loop_num++;
    this->parentLoop = parentLoop;
    this->loopDepth = parentLoop->loopDepth + 1;
    parentLoop->addChildLoop(this);
    name = parentLoop->name + "-" + (loopDepth == 0 ?
            std::to_string(0) : std::to_string(parentLoop->getChildrenLoops()->size()));
}

int Loop::getLoopDepth(){
    int ret = 0;
    Loop* loop = this;
    while (loop->getParentLoop() != emptyLoop) {
        loop = loop->getParentLoop();
        ret++;
    }
    return ret;
}

// 慎用，visitor用不到到这个
void Loop::setParentLoop(Loop* parentLoop) {
    this->parentLoop = parentLoop;
    this->parentLoop->addChildLoop(this);
}

Loop* Loop::getParentLoop() {
    return parentLoop;
}


/***
 * @param childLoop
 * @return true if this set: *childrenLoops did not already contain the childLoop
 * true表示之前没有，成功插进去了
 */
bool Loop::addChildLoop(Loop* childLoop) {
    bool ret = true;
    if (childrenLoops->find(childLoop) != childrenLoops->end()) {
        ret = false;
    }
    childrenLoops->insert(childLoop);
    return ret;
}

std::set<Loop*>* Loop::getChildrenLoops() {
    return childrenLoops;
}

/***
 * @param bb
 * @return true if this set: *nowLevelBB did not already contain the bb
 * true表示之前没有，成功插进去了
 */
bool Loop::addBB(BasicBlock* bb) {
    bool ret = true;
    if (nowLevelBB->find(bb) != nowLevelBB->end()) {
        ret = false;
    }
    nowLevelBB->insert(bb);
    return ret;
}

void Loop::removeBB(BasicBlock* bb) {
    nowLevelBB->erase(bb);
}

BasicBlock* Loop::getHeader() {
    return header;
}

void Loop::setHeader(BasicBlock* basicBlock) {
    header = basicBlock;
}


std::string Loop::toString() {
    return funcName + name;
}

void Loop::setFunc(Function* function) {
    funcName = function->name;
}


void Loop::addEntering(BasicBlock* bb) {
    enterings->insert(bb);
    bb->setLoopEntering();
}

void Loop::addLatch(BasicBlock* bb) {
    latchs->insert(bb);
    bb->setLoopLatch();
}

void Loop::addExiting(BasicBlock* bb) {
    exitings->insert(bb);
    bb->setLoopExiting();
}

void Loop::addExit(BasicBlock* bb) {
    exits->insert(bb);
    bb->setExit();
}

void Loop::addCond(Instr* instr) {
    int condNum = instr->getCondCount();
    if (!(conds->find(condNum) != conds->end())) {
        (*conds)[condNum] = new std::set<Instr*>();
    }
    (*conds)[condNum]->insert(instr);
}

void Loop::clearCond() {
    conds->clear();
}

void Loop::clear() {
    exits->clear();
    exitings->clear();
    enterings->clear();
    latchs->clear();
    //TODO:nowlevelBB手动维护(new BB 和 bb->remove时维护)
    //nowLevelBB->clear();
}

std::set<BasicBlock*>* Loop::getNowLevelBB() {
    return nowLevelBB;
}

std::set<BasicBlock*>* Loop::getExits() {
    return exits;
}


std::unordered_map<int, std::set<Instr*>*>* Loop::getConds() {
    return conds;
}

std::set<BasicBlock*>* Loop::getEnterings() {
    return enterings;
}

std::set<BasicBlock*>* Loop::getLatchs() {
    return latchs;
}

std::set<BasicBlock*>* Loop::getExitings() {
    return exitings;
}

//把一个循环复制到指定函数
void Loop::cloneToFunc(Function* function) {
    for (BasicBlock* bb: *nowLevelBB) {
        bb->cloneToFunc(function);
    }

    for (Loop* next: *childrenLoops) {
        next->cloneToFunc(function);
    }
}

//修正当前BB对应BB的use-def,同时修正简单的数据流:前驱后继关系
void Loop::fix() {
    for (BasicBlock* bb: *nowLevelBB) {
        //assert CloneInfoMap->valueMap.containsKey(bb);
        BasicBlock* needFixBB = (BasicBlock*) CloneInfoMap::getReflectedValue(bb);

        std::vector<BasicBlock*>* pres = new std::vector<BasicBlock*>();
        for (BasicBlock* pre: *(bb->precBBs)) {
            pres->push_back((BasicBlock*) CloneInfoMap::getReflectedValue(pre));
        }
        //needFixBB->setPrecBBs(pres);
        needFixBB->modifyPres(pres);

        std::vector<BasicBlock*>* succs = new std::vector<BasicBlock*>();
        for (BasicBlock* succ: *(bb->succBBs)) {
            succs->push_back((BasicBlock*) CloneInfoMap::getReflectedValue(succ));
        }
        //needFixBB->setSuccBBs(succs);
        needFixBB->modifySucs(succs);

        needFixBB->fix();
        needFixBB->loop = (CloneInfoMap::getReflectedLoop(bb->loop));
        CloneInfoMap::getReflectedLoop(bb->loop)->addBB(needFixBB);
        if (bb->isLoopHeader) {
            needFixBB->setLoopHeader();
        }
    }
    for (Loop* next: *childrenLoops) {
        next->fix();
        //TODO:check
        CloneInfoMap::getReflectedLoop(this)->addChildLoop(CloneInfoMap::getReflectedLoop(next));
    }
}

//简单循环符合下述形式:
//for(int i = X; i < Y; i = i + Z) {
//  xxx
//  i不能被更改,没有break,continue
//}
// head = exiting need?
bool Loop::isSimpleLoop() {
    return header->precBBs->size() == 2 && latchs->size() == 1 && exitings->size() == 1 && exits->size() == 1;
}

//std::string Loop::infoString() {
//    std::string ret = "\n";
//    ret += "Hash: ";
//    ret += String->valueOf(hash);
//    ret += "\n";
//
//    ret += "Deep: ";
//    ret += String->valueOf(getLoopDepth());
//    ret += "\n";
//
//
//    ret += "Header: ";
//    ret += header->getLabel() + " pre_num: " + String->valueOf(header->precBBs->size());
//    ret += "\n";
//
//    ret += "latch: ";
//    for (BasicBlock* bb: *latchs) {
//        ret += " " + bb->getLabel();
//    }
//    ret += "\n";
//
//    ret += "exiting: ";
//    for (BasicBlock* bb: *exitings) {
//        ret += " " + bb->getLabel();
//    }ret += "\n";
//
//    ret += "exit: ";
//    for (BasicBlock* bb: *exits) {
//        ret += " " + bb->getLabel();
//    }
//    ret += "\n";
//
//    ret += "now_level_BB: ";
//    for (BasicBlock* bb: *nowLevelBB) {
//        ret += " " + bb->getLabel();
//    }
//    ret += "\n";
//
//    if (isSimpleLoop() && isIdcSet()) {
//        ret += "idcAlu: " + idcAlu->toString() + "\n";
//        ret += "idcPHI: " + idcPHI->toString() + "\n";
//        ret += "idcCmp: " + idcCmp->toString() + "\n";
//        ret += "idcInit: " + idcInit->toString() + "\n";
//        ret += "idcEnd: " + idcEnd->toString() + "\n";
//        ret += "idcStep: " + idcStep->toString() + "\n";
//    }
//
//    return ret;
//}

void Loop::setIdc(Value* idcAlu, Value* idcPHI, Value* idcCmp, Value* idcInit, Value* idcEnd, Value* idcStep) {
    this->idcAlu = idcAlu;
    this->idcPHI = idcPHI;
    this->idcCmp = idcCmp;
    this->idcInit = idcInit;
    this->idcEnd = idcEnd;
    this->idcStep = idcStep;
    this->idcSet = true;
}

Value* Loop::getIdcAlu() {
    return idcAlu;
}

Value* Loop::getIdcPHI() {
    return idcPHI;
}

Value* Loop::getIdcCmp() {
    return idcCmp;
}

Value* Loop::getIdcInit() {
    return idcInit;
}

Value* Loop::getIdcEnd() {
    return idcEnd;
}

Value* Loop::getIdcStep() {
    return idcStep;
}

bool Loop::isIdcSet() {
    return idcSet;
}

void Loop::clearIdcInfo() {
    this->idcSet = false;
}

void Loop::setIdcTimes(int idcTimes) {
    this->idcTimes = idcTimes;
    this->idcTimeSet = true;
}

int Loop::getIdcTimes() {
    return idcTimes;
}

bool Loop::isIdcTimeSet() {
    return idcTimeSet;
}

bool Loop::hasChildLoop() {
    return childrenLoops->size() != 0;
}

std::string Loop::infoString() {
    std::string ret = "\n";
//    ret += "Hash: ";
//    ret += std::to_string(hash);
//    ret += "\n";

    ret += "Deep: ";
    ret += std::to_string(getLoopDepth());
    ret += "\n";


    ret += "Header: ";
    ret += header->label+ " pre_num: " + std::to_string(header->precBBs->size());
    ret += "\n";

    ret += "latch: ";
    for (BasicBlock *bb: *latchs) {
        ret += " " + bb->label;
    }
    ret += "\n";

    ret += "exiting: ";
    for (BasicBlock *bb: *exitings) {
        ret += " " + bb->label;
    }ret += "\n";

    ret += "exit: ";
    for (BasicBlock *bb: *exits) {
        ret += " " + bb->label;
    }
    ret += "\n";

    ret += "now_level_BB: ";
    for (BasicBlock *bb: *nowLevelBB) {
        ret += " " + bb->label;
    }
    ret += "\n";

    if (isSimpleLoop() && isIdcSet()) {
        ret += "idcAlu: " + std::string(*idcAlu) + "\n";
        ret += "idcPHI: " + std::string(*idcPHI) + "\n";
        ret += "idcCmp: " + std::string(*idcCmp) + "\n";
        ret += "idcInit: " + std::string(*idcInit) + "\n";
        ret += "idcEnd: " + std::string(*idcEnd) + "\n";
        ret += "idcStep: " + std::string(*idcStep) + "\n";
    }

    return ret;
}
