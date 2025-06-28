//
// Created by XuRuiyuan on 2023/7/15.
//

#include "../../include/mir/BasicBlock.h"
#include "../../include/mir/Loop.h"
#include "../../include/mir/Type.h"
#include "memory"
#include "../../include/mir/BasicBlock.h"
#include "../../include/mir/Instr.h"
#include "../../include/mir/Function.h"
#include "Visitor.h"
#include <assert.h>
#include <algorithm>
#include "stl_op.h"

BasicBlock::BasicBlock() {
    loop = Loop::emptyLoop;
    this->type = BBType::BB_TYPE;
    init();
    this->label = "EMPTY_BB_" + std::to_string(empty_bb_count++);
    this->begin->next = this->end;
    this->end->prev = this->begin;
}

BasicBlock::BasicBlock(Function *function, Loop *loop, bool insertAtEnd)
 : function(function), loop(loop){
    this->type = BBType::BB_TYPE;
    init();
    // this->loop = loop;
    this->label = "b" + std::to_string(++bb_count);
    // this->parentFunc = parentFunc;
    this->begin->next = this->end;
    this->end->prev = this->begin;
    // 放到这是因为HashSet存的时候需要计算hash，BB的hash靠label
    if (insertAtEnd) {
        function->insertAtEnd(this);
    }
    //loop->add_bb(this);
    loop->addBB(this);
}

// std::shared_ptr<BasicBlock> BasicBlock::get_this(){
//     return __GET_V_SP__(BasicBlock);
// }

void BasicBlock::setFunction(Function *function, Loop *loop) {
    this->loop = loop;
    this->function = function;
    this->label = "b" + std::to_string(++bb_count);
    function->insertAtBegin(this);
    loop->addBB(this);
}

void BasicBlock::init() {
    this->begin = new Instr();
    this->end = new Instr();
}

int BasicBlock::getLoopDep() {
    return this->loop->loopDepth;
}

void BasicBlock::insertAtEnd(Instr* in) {
    if (isTerminated()) {
        std::cout << "append after terminator instr" << in->getName() << std::endl;
    }
    in->set_prev(end->prev);
    in->set_next(end);
    end->prev->set_next(in);
    end->set_prev(in);
}

void BasicBlock::insertAtHead(Instr *in) {
    in->set_prev(begin);
    in->set_next(begin->next);
    begin->next->set_prev(in);
    begin->set_next(in);
}

Instr *BasicBlock::getBeginInstr() {
    return (Instr*) begin->next;
}

Instr *BasicBlock::getEndInstr() {
    return (Instr*) end->prev;
}

void BasicBlock::setPrecBBs(std::vector<BasicBlock *>* precBBs) {
    if (this->precBBs->size() != 0 && arrayEq(this->precBBs, precBBs)) {
//        if (this->label == "b47") {
//            printf("err");
//        }
        return;
    }
    if (this->precBBs->size() == 0) {
        this->precBBs = precBBs;
    } else {
        std::vector<BasicBlock*>* oldPres = this->precBBs;
        std::vector<BasicBlock*>* newPres = precBBs;
        if (oldPres->size() == newPres->size() && getBeginInstr()->isPhi()) {
            assert(false);
        }
        simplyPhi(oldPres, newPres);
        this->precBBs = precBBs;
    }
}

void BasicBlock::setSuccBBs(std::vector<BasicBlock *>* succBBs) {
    if (this->succBBs->size() != 0 && arrayEq(this->succBBs, succBBs)) {
        return;
    }
    this->succBBs = succBBs;
}

//void BasicBlock::setDoms(std::set<BasicBlock *> doms) {
//
//}
//
//void BasicBlock::setIdoms(std::set<BasicBlock *> idoms) {
//
//}
//
//void BasicBlock::setDF(std::set<BasicBlock *> DF) {
//
//}
//
//void BasicBlock::setDomTreeDeep(int doomTreeDeep) {
//
//}
//
//void BasicBlock::setIDominator(BasicBlock *iDominator) {
//
//}
//
//std::vector<BasicBlock *> BasicBlock::getPrecBBs() {
//    return std::vector<BasicBlock *>();
//}
//
//std::vector<BasicBlock *> BasicBlock::getSuccBBs() {
//    return std::vector<BasicBlock *>();
//}
//
//std::set<BasicBlock *> BasicBlock::getDoms() {
//    return std::set<BasicBlock *>();
//}
//
//std::set<BasicBlock *> BasicBlock::getIdoms() {
//    return std::set<BasicBlock *>();
//}
//
//std::set<BasicBlock *> BasicBlock::getDF() {
//    return std::set<BasicBlock *>();
//}
//
//int BasicBlock::getDomTreeDeep() {
//    return 0;
//}
//
//BasicBlock *BasicBlock::getIDominator() {
//    return nullptr;
//}

BasicBlock *BasicBlock::cloneToFunc(Function *function) {
    Loop* srcLoop = this->loop;
    auto tagLoop = _contains_(&(CloneInfoMap::loopMap), srcLoop)?
            CloneInfoMap::getReflectedLoop(srcLoop) :
            new Loop(CloneInfoMap::getReflectedLoop(loop->getParentLoop()));
//    tagLoop->funcName = function->getName();
    tagLoop->setFunc(function);
    CloneInfoMap::addLoopReflect(srcLoop, tagLoop);

    BasicBlock* ret = new BasicBlock(function, tagLoop);
    CloneInfoMap::addValueReflect(this, ret);
    tagLoop->addBB(ret);
    if (this->isLoopHeader) {
        tagLoop->setHeader(ret);
        ret->setLoopHeader();
    }
    for (Instr* instr = getBeginInstr(); instr->next != nullptr; instr = (Instr*) instr->next) {
        Instr* tmp = instr->cloneToBB(ret);
        if (instr->isInLoopCond()) {
            tmp->setInLoopCond();
        }
        if (instr->isCond()) {
            int srcCnt = instr->getCondCount();
            int tagCnt = _contains_(&CloneInfoMap::loopCondCntMap, srcCnt) ?
                    CloneInfoMap::getLoopCondCntReflect(srcCnt):
                    ++Visitor::VISITOR->curLoopCondCount;
            CloneInfoMap::addLoopCondCntReflect(srcCnt, tagCnt);
            tmp->setCondCount(tagCnt);
        }
    }
    return ret;
}

BasicBlock *BasicBlock::cloneToFunc(Function *function, Loop *loop) {
//    return nullptr;
    Loop* srcLoop = nullptr;
    Loop* tagLoop = nullptr;

    if (this->loop->getLoopDepth() == 0) {
        srcLoop = this->loop;
        tagLoop = loop;
        CloneInfoMap::addLoopReflect(srcLoop, tagLoop);
    } else {
        srcLoop = this->loop;
        tagLoop = _contains_(&CloneInfoMap::loopMap, srcLoop)?
                  CloneInfoMap::getReflectedLoop(srcLoop):
                  new Loop(CloneInfoMap::getReflectedLoop(srcLoop->getParentLoop()));
        tagLoop->setFunc(function);
        CloneInfoMap::addLoopReflect(srcLoop, tagLoop);
    }

//    BasicBlock* ret = new BasicBlock(function, tagLoop);
//    CloneInfoMap::addValueReflect(this, ret);
//    tagLoop->addBB(ret);
    BasicBlock* ret = new BasicBlock(function, tagLoop);
    CloneInfoMap::addValueReflect(this, ret);
    tagLoop->addBB(ret);
    if (this->isLoopHeader) {
        tagLoop->setHeader(ret);
        ret->setLoopHeader();
    }

    for (auto instr = getBeginInstr(); instr->next != nullptr; instr = (Instr*) instr->next) {
        Instr* tmp = instr->cloneToBB(ret);
        if (ret->getLoopDep() > 0) {
            tmp->setInLoopCond();
        }
        if (instr->isCond()) {
            int srcCnt = instr->getCondCount();
            int tagCnt = _contains_(&CloneInfoMap::loopCondCntMap, srcCnt) ?
                         CloneInfoMap::getLoopCondCntReflect(srcCnt):
                         ++Visitor::VISITOR->curLoopCondCount;
            CloneInfoMap::addLoopCondCntReflect(srcCnt, tagCnt);
            tmp->setCondCount(tagCnt);
        }
    }
    return ret;
}

BasicBlock *BasicBlock::cloneToFunc_LUR(Function *function, Loop *loop) {
//    return nullptr;
    Loop *tagLoop = nullptr, *srcLoop = nullptr;
    if (this->loop == loop) {
        tagLoop = this->loop;
    } else {
        srcLoop = this->loop;
        tagLoop = _contains_(&CloneInfoMap::loopMap, srcLoop)?
                CloneInfoMap::getReflectedLoop(srcLoop):
                new Loop(CloneInfoMap::getReflectedLoop(srcLoop->getParentLoop()));
        tagLoop->setFunc(function);
        CloneInfoMap::addLoopReflect(srcLoop, tagLoop);
    }
    BasicBlock* ret = new BasicBlock(function, tagLoop);
    CloneInfoMap::addValueReflect(this, ret);
    if (this->isLoopHeader) {
        tagLoop->setHeader(ret);
        ret->setLoopHeader();
    }
    for (auto instr = getBeginInstr(); instr->next != nullptr; instr = (Instr*) instr->next) {
        Instr* tmp = instr->cloneToBB(ret);
        if (instr->isInLoopCond()) {
            tmp->setInLoopCond();
        }
        if (instr->isCond()) {
            int srcCnt = instr->getCondCount();
            int tagCnt = _contains_(&CloneInfoMap::loopCondCntMap, srcCnt) ?
                         CloneInfoMap::getLoopCondCntReflect(srcCnt):
                         ++Visitor::VISITOR->curLoopCondCount;
            CloneInfoMap::addLoopCondCntReflect(srcCnt, tagCnt);
            tmp->setCondCount(tagCnt);
        }
    }
    return ret;
}


void BasicBlock::fixPreSuc(BasicBlock *oldBB) {
    std::vector<BasicBlock*>* pres = new std::vector<BasicBlock*>();
    for (BasicBlock* pre: *oldBB->precBBs) {
        pres->push_back((BasicBlock*) CloneInfoMap::getReflectedValue(pre));
    }
    modifyPres(pres);

    std::vector<BasicBlock*>* succs = new std::vector<BasicBlock*>();
    for (BasicBlock* succ: *oldBB->succBBs) {
        succs->push_back((BasicBlock*) CloneInfoMap::getReflectedValue(succ));
    }
    modifySucs(succs);

}

void BasicBlock::fix() {
    for (auto instr = getBeginInstr(); instr->next != nullptr; instr = (Instr*) instr->next) {
        instr->fix();
    }
}

void BasicBlock::modifyPre(BasicBlock *old, BasicBlock *now) {
    auto iter = std::find(precBBs->begin(), precBBs->end(), old);
    int index = iter - precBBs->begin();
    (*precBBs)[index] = now;
}

void BasicBlock::modifySuc(BasicBlock *old, BasicBlock *now) {
    auto iter = std::find(succBBs->begin(), succBBs->end(), old);
    int index = iter - succBBs->begin();
    (*succBBs)[index] = now;
}

void BasicBlock::modifyPres(std::vector<BasicBlock *>* precBBs) {
    this->precBBs = precBBs;
}

void BasicBlock::modifySucs(std::vector<BasicBlock *>* succBBs) {
    this->succBBs = succBBs;
}

void BasicBlock::addPre(BasicBlock *add) {
    precBBs->push_back(add);
}

void BasicBlock::addSuc(BasicBlock *add) {
    succBBs->push_back(add);

}

void BasicBlock::simplyPhi(std::vector<BasicBlock *>* oldPres, std::vector<BasicBlock *>* newPres) {
    for (auto instr = getBeginInstr(); instr->next != nullptr; instr = (Instr *) (instr->next)) {
        if (instr->isPhi()) {
            ((INSTR::Phi*) instr)->simple(oldPres, newPres);
        }
    }
}

bool BasicBlock::arrayEq(std::vector<BasicBlock *>* src, std::vector<BasicBlock *>* tag) {
    for (auto bb: (*src)) {
        if (std::find(tag->begin(), tag->end(), bb) == tag->end()) {
            return false;
        }
    }
    for (auto bb: (*tag)) {
        if (std::find(src->begin(), src->end(), bb) == src->end()) {
            return false;
        }
    }
    return true;
}

void BasicBlock::modifySucAToB(BasicBlock *A, BasicBlock *B) {
    //useless
}

void BasicBlock::modifyBrAToB(BasicBlock *A, BasicBlock *B) {
    Instr* instr = getEndInstr();
    if (instr->isBranch()) {
        int index = std::find(instr->useValueList.begin(),
                              instr->useValueList.end(),
                              A) - instr->useValueList.begin();
        instr->modifyUse(B, index);
    } else {
        instr->modifyUse(B, 0);
    }
}

void BasicBlock::modifyBrToJump(BasicBlock *next) {
    INSTR::Jump* jump = new INSTR::Jump(next, this);
    Instr* instr = getEndInstr();
    instr->remove();
    insertAtEnd(jump);
}

void BasicBlock::remove() {
    ILinkNode::remove();
    loop->removeBB(this);
    isLoopHeader = false;
    isLoopLatch = false;
    isExit = false;
    isLoopEntering = false;
    isLoopExiting = false;
}

Instr *BasicBlock::getEntry() {
    return (Instr*) begin->next;
}

Instr *BasicBlock::getLast() {
    return (Instr*) end->prev;
}

bool BasicBlock::isTerminated() {
    return getLast()->isTerminator();
}

void BasicBlock::setLoopStart() {
    isLoopHeader = true;
    loop->header = this;
}


void BasicBlock::setLoopHeader() {
    isLoopHeader = true;
}

void BasicBlock::setLoopEntering() {
    isLoopEntering = true;
}

void BasicBlock::setLoopExiting() {
    isLoopExiting = true;
}

void BasicBlock::setLoopLatch() {
    isLoopLatch = true;
}

void BasicBlock::setExit() {
    isExit = true;
}

void BasicBlock::setNotHead() {
    isLoopHeader = false;
}

void BasicBlock::setNotLatch() {
    isLoopLatch = false;
}

void BasicBlock::modifyLoop(Loop *loop) {
    this->loop = loop;
    loop->addBB(this);
    //展开后的部分,在原来的父循环中,只是基本的循环内部块
    //
    isLoopHeader = false;
    isLoopEntering = false;
    isLoopExiting = false;
    isLoopLatch = false;
    isExit = false;
}
BasicBlock::operator std::string() const {
    // Simplified for CACT compiler - just return the label
        return this->label;
}
