//
// Created by XuRuiyuan on 2023/7/14.
//

//TODO:modify all cloneToBB
#include "mir/Instr.h"
#include "mir/Use.h"
#include "mir/Type.h"
#include "mir/Function.h"
#include "mir/CloneInfoMap.h"
#include "mir/Value.h"
#include "frontend/Visitor.h"
#include <assert.h>
#include <iterator>
#include <algorithm>
#include <iostream>
#include <numeric>
#include "sstream"
bool operator==(const Instr &first, const Instr &second) {
    return first.id == second.id;
}

bool operator!=(const Instr &first, const Instr &second) {
    return first.id != second.id;
}

bool operator<(const Instr &first, const Instr &second) {
    return first.id < second.id;
}

Instr::Instr(): Value() {

}

Instr::Instr(Type *type, BasicBlock *bb, int bit): Value(type) {
    init();
    this->bb = bb;
}

Instr::Instr(Type *type, BasicBlock *curBB): Value(type) {
    init();
    this->bb = curBB;
    if (!bb->isTerminated()) {
        bb->insertAtEnd(this);
    }
}

Instr::Instr(Type *type, BasicBlock *curBB, bool Lipsum): Value(type) {
    init();
    this->bb = curBB;
    bb->insertAtHead(this);
}

Instr::Instr(Type *type, Instr* instr): Value(type){
    init();
    this->bb = instr->bb;
    instr->insert_before(this);
}

void Instr::setUse(Value *value, int idx) {
    Use* use = new Use(this, value, idx);
    value->insertAtEnd(use);
    useList.push_back(use);
    useValueList.push_back(value);
}

void Instr::modifyUse(Value *old, Value *now, int index) {
    useList[index]->remove();
    Use* use = new Use(this, now, index);
    useList[index] = use;
    useValueList[index] = now;
    now->insertAtEnd(use);
}

void Instr::modifyUse(Value *now, int index) {
    useList[index]->remove();
    Use* use = new Use(this, now, index);
    useList[index] = use;
    useValueList[index] = now;
    now->insertAtEnd(use);
}

bool Instr::isEnd() {
    return this->next == nullptr;
}

void Instr::init() {
    useList.clear();
    useValueList.clear();
    //TODO:
    inLoopCond = Visitor::VISITOR->isInLoopCond();
    condCount = Visitor::VISITOR->getCondCount();
    // hash = "Instr " + LOCAL_COUNT;
    prefix = LOCAL_PREFIX;
    name = LOCAL_NAME_PREFIX + std::to_string(LOCAL_COUNT++);
}

void Instr::removeUserUse() {
    for (int i = 0; i < useList.size(); ++i) {
        useList[i]->remove();
    }
    useValueList.clear();
    useList.clear();
}

void Instr::remove() {
    ILinkNode::remove();
    this->removeUserUse();
}

void Instr::delFromNowBB() {
    ILinkNode::remove();
}

Instr* Instr::cloneToBB(BasicBlock* bb) {
    return nullptr;
}

void Instr::setInLoopCond() {
    this->inLoopCond = true;
}

void Instr::setNotInLoopCond() {
    this->inLoopCond = false;
}

bool Instr::isInLoopCond() {
    return inLoopCond;
}

bool Instr::isCond() {
    return condCount > 0;
}

int Instr::getCondCount() {
    return condCount;
}

void Instr::setCondCount(int condCount) {
    this->condCount = condCount;
}

void Instr::setArrayInit(bool arrayInit) {
    this->arrayInit = arrayInit;
}

bool Instr::isArrayInit() {
    return arrayInit;
}

void Instr::fix() {
    int len = useValueList.size();
    for (int i = 0; i < len; ++i) {
        modifyUse(CloneInfoMap::getReflectedValue(useValueList[i]), i);
    }
}

bool Instr::isTerminator() {
    return this->isBJ() || this->isReturn();
}

bool Instr::canComb() {
    if (!check()) {
        return false;
    }
    Use* use = getBeginUse();
    while (use->next != nullptr) {
        Instr* user = use->user;
        if (!user->check()) {
            return false;
        }
        use = (Use*) use->next;
    }
    return true;
}

bool Instr::canCombFloat() {
    return false;
}

bool Instr::check() {
    if (!this->isAlu()) {
        return false;
    }
    if (!this->type->is_int32_type()) {
        return false;
    }
    if (!((INSTR::Alu*) this)->isSub() && !((INSTR::Alu*) this)->isAdd()) {
        return false;
    }
    if (!((INSTR::Alu*) this)->hasOneConst()) {
        return false;
    }
    return true;
}

std::string Instr::usesToString() const {
    std::stringstream ss;
    ss << "<";
    for (int i = 0; i < useList.size(); i++) {
        if (i > 0) ss << ", ";
        Use *use = useList[i];
        ss << std::string(*use);
    }
    ss << ">";
    return ss.str();
}

std::string Instr::useValuesToString() const {
    std::stringstream ss;
    ss << "<";
    for (int i = 0; i < useValueList.size(); i++) {
        if (i > 0) ss << ", ";
        ss << useValueList[i]->getDescriptor();
    }
    ss << ">";
    return ss.str();
}

INSTR::Alu::Alu(Type *type, Alu::Op op, Value *op1, Value *op2, BasicBlock* basicBlock)
        :Instr(type, basicBlock) {
    tag = ValueTag::bino;
    this->op = op;
    setUse(op1, 0);
    setUse(op2, 1);
}

INSTR::Alu::Alu(Type *type, Alu::Op op, Value *op1, Value *op2, Instr* insertBefore)
        :Instr(type, insertBefore) {
    tag = ValueTag::bino;
    this->op = op;
    setUse(op1, 0);
    setUse(op2, 1);
}


Value *INSTR::Alu::getRVal1() const {
    return useValueList[0];
}

Value *INSTR::Alu::getRVal2() const {
    return useValueList[1];
}

bool INSTR::Alu::isAdd() const {
    return op == Op::ADD;
}

bool INSTR::Alu::isSub() const {
    return op == Op::SUB;
}

bool INSTR::Alu::isFAdd() const {
    return op == Op::FADD;
}

bool INSTR::Alu::isFSub() const {
    return op == Op::FSUB;
}

bool INSTR::Alu::isMul() const {
    return op == Op::MUL;
}

bool INSTR::Alu::isFMul() const {
    return op == Op::FMUL;
}

bool INSTR::Alu::hasOneConst() const {
    if (useValueList[0]->isConstant() && useValueList[1]->isConstant()) {
        return false;
    }
    return (useValueList[0]->isConstant() || useValueList[1]->isConstant());
}

bool INSTR::Alu::hasTwoConst() const {
    return (useValueList[0]->isConstant() && useValueList[1]->isConstant());
}

//TODO:
Instr *INSTR::Alu::cloneToBB(BasicBlock *bb) {
    Instr* ret = new Alu(type, op, getRVal1(), getRVal2(), bb);
    CloneInfoMap::addValueReflect(this, ret);
    return ret;
}

// INSTR::Ashr::Ashr(Type *type, Value *op1, Value *op2, BasicBlock *basicBlock)
//         :Instr(type, basicBlock) {
//     tag = ValueTag::ashr;
//     setUse(op1, 0);
//     setUse(op2, 1);
// }

// INSTR::Ashr::Ashr(Type *type, Value *op1, Value *op2, Instr *insertBefore)
//         :Instr(type, insertBefore) {
//     tag = ValueTag::ashr;
//     setUse(op1, 0);
//     setUse(op2, 1);
// }

// Value *INSTR::Ashr::getRVal1() const {
//     return useValueList[0];
// }

// Value *INSTR::Ashr::getRVal2() const {
//     return useValueList[1];
// }

// bool INSTR::Ashr::hasTwoConst() {
//     return useValueList[0]->isConstant() && useValueList[1]->isConstant();
// }

// Instr *INSTR::Ashr::cloneToBB(BasicBlock *bb) {
//     Instr* ret = new Ashr(type, getRVal1(), getRVal2(), bb);
//     CloneInfoMap::addValueReflect(this, ret);
//     return ret;
// }

INSTR::Icmp::Icmp(Icmp::Op op, Value *op1, Value *op2, BasicBlock *curBB)
        :Instr(BasicType::I1_TYPE, curBB) {
    this->op = op;
    tag = ValueTag::icmp;
    setUse(op1, 0);
    setUse(op2, 1);
}

INSTR::Icmp::Icmp(Icmp::Op op, Value *op1, Value *op2, Instr *insertBefore)
        :Instr(BasicType::I1_TYPE, insertBefore) {
    this->op = op;
    tag = ValueTag::icmp;
    setUse(op1, 0);
    setUse(op2, 1);
}

Value *INSTR::Icmp::getRVal1() const {
    return useValueList[0];
}

Value *INSTR::Icmp::getRVal2() const {
    return useValueList[1];
}

Instr *INSTR::Icmp::cloneToBB(BasicBlock *bb) {
    Instr* ret = new Icmp(op, getRVal1(), getRVal2(), bb);
    CloneInfoMap::addValueReflect(this, ret);
    return ret;
}

INSTR::Fcmp::Fcmp(Fcmp::Op op, Value *op1, Value *op2, BasicBlock *curBB)
        :Instr(BasicType::I1_TYPE, curBB) {
    this->op = op;
    tag = ValueTag::fcmp;
    setUse(op1, 0);
    setUse(op2, 1);
}

INSTR::Fcmp::Fcmp(Fcmp::Op op, Value *op1, Value *op2, Instr *insertBefore)
        :Instr(BasicType::I1_TYPE, insertBefore)  {
    this->op = op;
    tag = ValueTag::fcmp;
    setUse(op1, 0);
    setUse(op2, 1);
}

Value *INSTR::Fcmp::getRVal1() const {
    return useValueList[0];
}

Value *INSTR::Fcmp::getRVal2() const {
    return useValueList[1];
}

Instr *INSTR::Fcmp::cloneToBB(BasicBlock *bb) {
    Instr* ret = new Fcmp(op, getRVal1(), getRVal2(), bb);
    CloneInfoMap::addValueReflect(this, ret);
    return ret;
}

INSTR::Zext::Zext(Value *src, BasicBlock *parentBB)
        :Instr(BasicType::I32_TYPE, parentBB) {
    tag = ValueTag::zext;
    setUse(src, 0);
}

INSTR::Zext::Zext(Value *src, Instr *insertBefore)
        :Instr(BasicType::I32_TYPE, insertBefore) {
    tag = ValueTag::zext;
    setUse(src, 0);
}

Value *INSTR::Zext::getRVal1() const {
    return useValueList[0];
}

Instr *INSTR::Zext::cloneToBB(BasicBlock *bb) {
    Instr* ret = new Zext(getRVal1(), bb);
    CloneInfoMap::addValueReflect(this, ret);
    return ret;
}

INSTR::FPtosi::FPtosi(Value *src, BasicBlock *parentBB)
        :Instr(BasicType::I32_TYPE, parentBB) {
    tag = ValueTag::fptosi;
    setUse(src, 0);
}

INSTR::FPtosi::FPtosi(Value *src, Instr *insertBefore)
        :Instr(BasicType::I32_TYPE, insertBefore) {
    tag = ValueTag::fptosi;
    setUse(src, 0);
}

Value *INSTR::FPtosi::getRVal1() const {
    return useValueList[0];
}

Instr *INSTR::FPtosi::cloneToBB(BasicBlock *bb) {
    Instr* ret = new FPtosi(getRVal1(), bb);
    CloneInfoMap::addValueReflect(this, ret);
    return ret;
}

INSTR::SItofp::SItofp(Value *src, BasicBlock *parentBB)
        :Instr(BasicType::F32_TYPE, parentBB) {
    tag = ValueTag::sitofp;
    setUse(src, 0);
}

INSTR::SItofp::SItofp(Value *src, Instr *insertBefore)
        :Instr(BasicType::F32_TYPE, insertBefore) {
    tag = ValueTag::sitofp;
    setUse(src, 0);
}

Value *INSTR::SItofp::getRVal1() const {
    return useValueList[0];
}

Instr *INSTR::SItofp::cloneToBB(BasicBlock *bb) {
    Instr* ret = new SItofp(getRVal1(), bb);
    CloneInfoMap::addValueReflect(this, ret);
    return ret;
}

INSTR::Alloc::Alloc(Type *type, BasicBlock *parentBB)
        :Instr(new PointerType(type), parentBB, true) {
    tag = ValueTag::alloc;
    this->contentType = type;
}

INSTR::Alloc::Alloc(Type *type, BasicBlock *parentBB, bool Lipsum)
        :Instr(new PointerType(type), parentBB) {
    tag = ValueTag::alloc;
    this->contentType = type;
}

void INSTR::Alloc::clearLoads() {
    loads->clear();
}

void INSTR::Alloc::addLoad(Instr *instr) {
    loads->insert(instr);
}

bool INSTR::Alloc::isArrayAlloc() {
    return contentType->is_array_type();
}

Instr *INSTR::Alloc::cloneToBB(BasicBlock *bb) {
    Instr* ret = new Alloc(contentType, bb);
    CloneInfoMap::addValueReflect(this, ret);
    return ret;
}

void INSTR::Load::clear() {
    alloc = nullptr;
    useStore = nullptr;
}

INSTR::Load::Load(Value *pointer, BasicBlock *parentBB)
        :Instr(((PointerType*) pointer->type)->inner_type, parentBB){
    tag = ValueTag::load;
//    if (type->is_pointer_type()) {
//        printf("alloc point\n");
//    }
    setUse(pointer, 0);
}

INSTR::Load::Load(Value *pointer, Instr *insertBefore)
        :Instr(((PointerType*) pointer->type)->inner_type, insertBefore){
    tag = ValueTag::load;
    setUse(pointer, 0);
}

Value * INSTR::Load::getPointer() const {
    return useValueList[0];
}

Instr *INSTR::Load::cloneToBB(BasicBlock *bb) {
    Instr* ret = new Load(getPointer(), bb);
    CloneInfoMap::addValueReflect(this, ret);
    return ret;
}

void INSTR::Store::addUser(Instr* user) {
    users->insert(user);
}

void INSTR::Store::clear() {
    alloc = nullptr;
    users->clear();
}

INSTR::Store::Store(Value *value, Value *address, BasicBlock *parent)
        :Instr(VoidType::VOID_TYPE, parent) {
    tag = ValueTag::store;
    setUse(value, 0);
    setUse(address, 1);
}

INSTR::Store::Store(Value *value, Value *address, Instr *insertBefore)
        :Instr(VoidType::VOID_TYPE, insertBefore) {
    tag = ValueTag::store;
    setUse(value, 0);
    setUse(address, 1);
}

Value * INSTR::Store::getValue() const {
    return useValueList[0];
}

Value * INSTR::Store::getPointer() const {
    return useValueList[1];
}

Instr *INSTR::Store::cloneToBB(BasicBlock *bb) {
    Instr* ret = new Store(getValue(), getPointer(), bb);
    CloneInfoMap::addValueReflect(this, ret);
    return ret;
}

INSTR::GetElementPtr::GetElementPtr(Type *pointeeType, Value *ptr,
                                    std::vector<Value *>* idxList, BasicBlock *bb)
        :Instr(new PointerType(pointeeType), bb) {
    tag = ValueTag::gep;
    setUse(ptr, 0);
//    for (int i = 0; i < idxList->size(); ++i) {
//        setUse((*idxList)[i], i);
//    }
    int i = 1;
    for (Value* idxValue : *idxList) {
        setUse(idxValue, i++);
    }
}

Type *INSTR::GetElementPtr::getPointeeType() {
    return ((PointerType*) type)->inner_type;
}

Value * INSTR::GetElementPtr::getPtr() const {
    return useValueList[0];
}

int INSTR::GetElementPtr::getOffsetCount() {
    return useValueList.size() - 1;
}


std::vector<Value *> * INSTR::GetElementPtr::getIdxList() const {
    std::vector<Value *>* ret = new std::vector<Value *>();
    for (int i = 1; i < useValueList.size(); ++i) {
        ret->push_back(useValueList[i]);
    }
    return ret;
}

Value *INSTR::GetElementPtr::getIdxValueOf(int i) {
    if (i + 1 >= useValueList.size()) {
        assert(false);
    }
    return useValueList[i + 1];
}

void INSTR::GetElementPtr::modifyPtr(Value *ptr) {
    modifyUse(ptr, 0);
}

void INSTR::GetElementPtr::modifyIndexs(std::vector<Value *>* indexs) {
    for (int i = 0; i < getIdxList()->size(); ++i) {
        modifyUse((*indexs)[i], i + 1);
    }
    for (int i = getIdxList()->size(); i < indexs->size(); ++i) {
        setUse((*indexs)[i], i + 1);
    }
}

Instr *INSTR::GetElementPtr::cloneToBB(BasicBlock *bb) {
    Instr* ret = new GetElementPtr(getPointeeType(), getPtr(), getIdxList(), bb);
    CloneInfoMap::addValueReflect(this, ret);
    return ret;
}

INSTR::BitCast::BitCast(Value *srcValue, Type *dstType, BasicBlock *parent)
        :Instr(dstType, parent) {
    tag = ValueTag::bitcast;https://github.com/Meow-Twice/sysy-perf-compare/blob/master/perf.py
    setUse(srcValue, 0);
}

Value * INSTR::BitCast::getSrcValue() const {
    return useValueList[0];
}

Type *INSTR::BitCast::getDstType() {
    return type;
}

Instr *INSTR::BitCast::cloneToBB(BasicBlock *bb) {
    Instr* ret = new BitCast(getSrcValue(), getDstType(), bb);
    CloneInfoMap::addValueReflect(this, ret);
    return ret;
}


INSTR::Call::Call(Function *func, std::vector<Value *>* paramList, BasicBlock *parent)
        :Instr(func->retType, parent) {
    tag = ValueTag::call;
    int i = 0;
    setUse(func, i++);
    for (Value *param: *paramList) {
        setUse(param, i++);
    }
    // this->paramList = paramList;
}

INSTR::Call::Call(Function *func, std::vector<Value *>* paramList, Instr *insertBefore)
        :Instr(func->retType, insertBefore) {
    tag = ValueTag::call;
    int i = 0;
    setUse(func, i++);
    for (Value *param: *paramList) {
        setUse(param, i++);
    }
    // this->paramList = paramList;
}

Function * INSTR::Call::getFunc() const {
    return (Function*) useValueList[0];
}

//TODO:for pass error check
// need modify
std::vector<Value *>* INSTR::Call::getParamList() const {
//    return new std::vector<Value *>(useValueList.begin() + 1, useValueList.end());
    auto* ret = new std::vector<Value*>();
    for (int i = 1; i < useValueList.size(); i++) {
        ret->push_back(useValueList[i]);
    }
    return ret;
}

Instr *INSTR::Call::cloneToBB(BasicBlock *bb) {
    std::vector<Value *>* params = new std::vector<Value*>();
//    for (auto i : (*getParamList())) {
//        (*params).push_back(i);
//    }
    for (int i = 1; i < useValueList.size(); i++) {
        params->push_back(useValueList[i]);
    }
    Instr* ret = new Call(getFunc(), params, bb);
    CloneInfoMap::addValueReflect(this, ret);
    return ret;
}

INSTR::Phi::Phi(Type *type, std::vector<Value *>* values, BasicBlock *parent)
        :Instr(type, parent, true) {
    tag = ValueTag::phi;
    for (int i = 0; i < (*values).size(); i++) {
        setUse((*values)[i], i);
    }
}

INSTR::Phi::Phi(Type *type, std::vector<Value *>* values, BasicBlock *parent, bool isLCSSA)
        :Instr(type, parent, true) {
    tag = ValueTag::phi;
    for (int i = 0; i < values->size(); i++) {
        setUse((*values)[i], i);
    }
    this->isLCSSA = isLCSSA;
}

//TODO: check
std::vector<Value *>* INSTR::Phi::getOptionalValues() {
    return &useValueList;
}

void INSTR::Phi::addOptionalValue(Value *value) {
    int index = useValueList.size();
    setUse(value, index);
}


Instr* INSTR::Phi::cloneToBB(BasicBlock* bb) {
    std::vector<Value*>* values = new std::vector<Value*>();
    for (auto i: (*getOptionalValues())) {
        (*values).push_back(i);
    }
    Instr* ret = new Phi(type, values, bb);
    CloneInfoMap::addValueReflect(this, ret);
    return ret;
}

void INSTR::Phi::simple(std::vector<BasicBlock *>* oldPres,
                        std::vector<BasicBlock *>* newPres) {
    std::vector<Value*> values;
    for (int i = 0; i < newPres->size(); ++i) {
        auto iter =
                std::find(oldPres->begin(), oldPres->end(), (*newPres)[i]);
        int index = iter - oldPres->begin();
        assert(index != -1);
        values.push_back(useValueList[index]);
    }
    for (int i = 0; i < useList.size(); ++i) {
        useList[i]->remove();
    }
    useList.clear();
    useValueList.clear();
    for (int i = 0; i < values.size(); ++i) {
        setUse(values[i], i);
    }
}

INSTR::PCopy::PCopy(std::vector<Value *>* LHS,
                    std::vector<Value *>* RHS,
                    BasicBlock* parent)
        :Instr(VoidType::VOID_TYPE, parent) {
    tag = ValueTag::pcopy;
    this->LHS = LHS;
    this->RHS = RHS;
}

void INSTR::PCopy::addToPC(Value *tag, Value *src) {
    (*this->LHS).push_back(tag);
    (*this->RHS).push_back(src);

}

std::vector<Value *>* INSTR::PCopy::getLHS() {
    return LHS;
}

std::vector<Value *>* INSTR::PCopy::getRHS() {
    return RHS;
}

INSTR::Move::Move(Type *type, Value *dst, Value *src, BasicBlock *parent)
        :Instr(type, parent) {
    tag = ValueTag::move;
    this->dst = dst;
    this->src = src;
}

Value *INSTR::Move::getSrc() {
    return src;
}

Value *INSTR::Move::getDst() {
    return dst;
}

INSTR::Branch::Branch(Value *cond, BasicBlock *thenTarge, BasicBlock *elseTarget, BasicBlock *parent)
        :Instr(VoidType::VOID_TYPE, parent) {
    assert(cond->type->is_int1_type());
    tag = ValueTag::branch;
    setUse(cond, 0);
    setUse(thenTarge, 1);
    setUse(elseTarget, 2);

    this->thenTarget = thenTarge;
    this->elseTarget = elseTarget;
}

Value * INSTR::Branch::getCond() const {
    return useValueList[0];
}

BasicBlock * INSTR::Branch::getThenTarget() const {
    return (BasicBlock*) useValueList[1];
}

BasicBlock * INSTR::Branch::getElseTarget() const {
    return (BasicBlock*) useValueList[2];
}

void INSTR::Branch::setThenTarget(BasicBlock* thenTarget) {
    this->thenTarget = thenTarget;
    modifyUse(thenTarget, 1);
}

void INSTR::Branch::setElseTarget(BasicBlock* elseTarget) {
    this->elseTarget = elseTarget;
    modifyUse(elseTarget, 2);
}

Instr *INSTR::Branch::cloneToBB(BasicBlock *bb) {
    Instr* ret = new Branch(getCond(), getThenTarget(), getElseTarget(), bb);
    CloneInfoMap::addValueReflect(this, ret);
    return ret;
}

INSTR::Jump::Jump(BasicBlock *target, BasicBlock* parent)
:Instr(VoidType::VOID_TYPE, parent)
{
    tag = ValueTag::jump;
    setUse(target, 0);
}

BasicBlock * INSTR::Jump::getTarget() const {
    return (BasicBlock*) useValueList[0];
}

Instr *INSTR::Jump::cloneToBB(BasicBlock *bb) {
    Instr* ret = new Jump(getTarget(), bb);
    CloneInfoMap::addValueReflect(this, ret);
    return ret;
}

INSTR::Return::Return(BasicBlock *parent)
        :Instr(VoidType::VOID_TYPE, parent) {
    tag = ValueTag::ret;
}

INSTR::Return::Return(Value *retValue, BasicBlock *parent)
        :Instr(retValue->type, parent) {
    tag = ValueTag::ret;
    setUse(retValue, 0);
}

bool INSTR::Return::hasValue() {
    return !useValueList.empty();
}

Value * INSTR::Return::getRetValue() const {
    return useValueList[0];
}

Instr *INSTR::Return::cloneToBB(BasicBlock *bb) {
    Instr* ret = nullptr;
    if (hasValue()) {
        ret = new Return(getRetValue(), bb);
    } else {
        ret = new Return(bb);
    }
    CloneInfoMap::addValueReflect(this, ret);
    return ret;
}

std::string INSTR::get_alu_op_name(Alu::Op op) {
    static std::string OpNameList[16] = {
            "add",
            "fadd",
            "sub",
            "fsub",
            "mul",
            "fmul",
            "sdiv",
            "fdiv",
            "srem",
            "frem",
            "and",
            "or",
            "xor",
            "shl",
            "lshr",
            "ashr",
    };
    return OpNameList[(int) op];
}

std::string INSTR::get_icmp_op_name(Icmp::Op op) {
    static std::string OpList[7]={
            "eq",
            "ne",
            "sgt",
            "sge",
            "slt",
            "sle",
    };
    return OpList[(int) op];
}

std::string INSTR::get_fcmp_op_name(Fcmp::Op op) {
    static std::string STRLIST[7] = {
            "oeq",
            "one",
            "ogt",
            "oge",
            "olt",
            "ole",
    };
    return STRLIST[(int) op];
}

// std::string INSTR::Alu::OpNameList[12] = {
//         "add",
//         "fadd",
//         "sub",
//         "fsub",
//         "mul",
//         "fmul",
//         "sdiv",
//         "fdiv",
//         "srem",
//         "frem",
//         "and",
//         "or",
// };

INSTR::Alu::operator std::string() const  {
    return getName() + " = " + INSTR::get_alu_op_name(op) + " " + getRVal1()->getDescriptor() + ", " + getRVal2()->getName();
}

INSTR::Fcmp::operator std::string() const {
    return getName() + " = fcmp " + INSTR::get_fcmp_op_name(op) + " " + getRVal1()->getDescriptor() + ", " + getRVal2()->getName();;
}


INSTR::Icmp::operator std::string() const {
    return getName() + " = icmp " + INSTR::get_icmp_op_name(op) + " " + getRVal1()->getDescriptor() + ", " + getRVal2()->getName();;
}

INSTR::Zext::operator std::string() const {
    return getName() + " = zext " + getRVal1()->getDescriptor() + " to " + std::string(*type);
}

INSTR::FPtosi::operator std::string() const {
    return getName() + " = fptosi " + getRVal1()->getDescriptor() + " to " + std::string(*type);
}

INSTR::SItofp::operator std::string() const {
    return getName() + " = sitofp " + getRVal1()->getDescriptor() + " to " + std::string(*type);
}

INSTR::Alloc::operator std::string() const {
    return getName() + " = alloca " + std::string(*contentType);
}

INSTR::Load::operator std::string() const {
    return getName() + " = load " + std::string(*type) + ", " + getPointer()->getDescriptor();
}

INSTR::Store::operator std::string() const {
    return "store " + getValue()->getDescriptor() + ", " + getPointer()->getDescriptor();
}

INSTR::GetElementPtr::operator std::string() const {
    std::string strBD;
    strBD.append(getName()).append(" = getelementptr inbounds ").append(std::string(*(((PointerType *) getPtr()->type)->inner_type))).append(", ").append(std::string(*getPtr()->type)).append(" ").append(getPtr()->getName());
    for (Value *idxValue : *getIdxList()) {
        strBD.append(", ").append(std::string(*idxValue->type)).append(" ").append(idxValue->getName());
    }
    return strBD;
}

INSTR::BitCast::operator std::string() const {
    return getName() + " = bitcast " + getSrcValue()->getDescriptor() + " to " + std::string(*type);
}

INSTR::Call::operator std::string() const {
    std::string prefix = "";
    std::string retType = "void";
    if (!type->is_void_type()) {
        prefix = getName() + " = ";
        retType = std::string(*type);
    }
    // std::stringstream result;
    // for (int i = 0; i < paramList->size(); ++i) {
    //     result << std::string(*(*paramList)[i]);
    //     if (i != paramList->size() - 1) {
    //         result << ", ";
    //     }
    // }
    std::vector<Value *> *params = getParamList();
    std::string result = std::accumulate(
            params->begin(), params->end(), std::string(),
            [](const std::string& s, Value *value) {
                return s.empty() ? value->getDescriptor() : s + ", " + value->getDescriptor();
            }
    );
    return prefix + "call " + retType + " @" + getFunc()->getName() + "(" + result + ")";
}

INSTR::Phi::operator std::string() const {
    std::string ret = getName() + " = phi " + std::string(*type) + " ";
    int len = useValueList.size();
    for (int i = 0; i < len; i++) {
        Value *value = useValueList[i];
        if (bb->precBBs->size() <= i) {
            return "err";
        }
        ret.append("[ ").append(value->getName()).append(", %").append(bb->precBBs->operator[](i)->label).append(" ]");
        if (i < len - 1) {
            ret.append(", ");
        }
    }
    return ret;
}

INSTR::PCopy::operator std::string() const {
    std::string ret = "PCopy ";
    int len = RHS->size();
    for (int i = 0; i < len; i++) {
        ret.append((*LHS)[i]->getName()).append(" <-- ").append((*RHS)[i]->getName());
        if (i < len - 1) {
            ret.append(", ");
        }
    }
    return ret;
}

INSTR::Move::operator std::string() const {
    std::string ret = "move ";
    ret += std::string(*type) + " " + dst->getName() + " <-- " + src->getName();
    return ret;
}
INSTR::Branch::operator std::string() const {
    return "br " + getCond()->getDescriptor() + ", label %" + getThenTarget()->label + ", label %" + getElseTarget()->label;
}
INSTR::Jump::operator std::string() const {
    return "br label %" + getTarget()->label;
}
INSTR::Return::operator std::string() const {
    if (useValueList.empty()) {
        return "ret void";
    } else {
        return "ret " + getRetValue()->getDescriptor();
    }
}

