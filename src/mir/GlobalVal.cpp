//
// Created by Administrator on 2023/7/16.
//
#include <utility>

#include "../../include/mir/Type.h"
#include "../../include/mir/GlobalVal.h"
#include "../../include/mir/Value.h"

#include <sstream>

GlobalVal::GlobalVal(Type *type) : Value(type) {
    isGlobal = true;
}

void GlobalValue::setCanLocal() {
    this->local = true;
}

bool GlobalValue::canLocal() const {
    return this->local;
}

GlobalValue::GlobalValue(Type *pointeeType, AST::Def *def, Initial *initial)
        : GlobalVal(new PointerType(pointeeType)) {
    prefix = GLOBAL_PREFIX;
    name = "g_" + def->ident->content;
    this->def = def;
    this->initial = initial;
}

GlobalValue::GlobalValue(Type *pointeeType, std::string name, Initial *initial)
        : GlobalVal(new PointerType(pointeeType)) {
    prefix = GLOBAL_PREFIX;
    this->name = std::move(name);
    this->initial = initial;
}

std::string UndefValue::getLabel() const {
    return label;
}

std::string UndefValue::getName() const {
    return "undef";
}

int VirtualValue::virtual_value_cnt = 0;
VirtualValue::VirtualValue(Type *type) : GlobalVal(type) {
    prefix = LOCAL_PREFIX;
    name = GLOBAL_NAME_PREFIX + std::to_string(virtual_value_cnt++);
}

GlobalValue::operator std::string() const {
    std::stringstream ss;
    ss << "[global_value: " << def->ident->content << "]";
    return ss.str();
}

UndefValue::operator std::string() const {
    return "!undef!";
}

VirtualValue::operator std::string() const {
    return "!VirtualValue!";
}

bool CompareGlobalValue::operator()(const GlobalValue *lhs, const GlobalValue *rhs) const {
    return lhs->id < rhs->id;
}
