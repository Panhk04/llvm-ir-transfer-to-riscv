//
// Created by XuRuiyuan on 2023/7/18.
//

#include "../../include/mir/Constant.h"
#include "../../include/mir/Type.h"
#include <iostream>
#include <sstream>
#include <iomanip>

Constant::Constant(Type *type) : Value(type) {
    this->type = type;
    _isConstant = true;
}

// std::ostream &operator<<(std::ostream &ostream, const Constant *constant) {
//     if (auto *p = dynamic_cast<const ConstantInt *>(constant)) {
//         ostream << p;
//     } else if (auto *p1 = dynamic_cast<const ConstantFloat *>(constant))
//         ostream << p1;
//     else if (auto *p2 = dynamic_cast<const ConstantBool *>(constant))
//         ostream << p2;
//     return ostream;
// }

bool Constant::operator==(const Constant &other) const {
    return false;
}

ConstantInt::operator std::string() const {
    return std::to_string(this->u.i);
}

ConstantInt::ConstantInt(int iv) : Constant(BasicType::I32_TYPE) {
    u.i = iv;
    v = new IFV(iv);
}

ConstantInt *ConstantInt::get_const_int(int int_val) {
    if (int_val == 0) return ConstantInt::CONST_0;
    if (int_val == 1) return ConstantInt::CONST_1;
    auto it = const_int_map.find(int_val);
    ConstantInt *ret;
    if (it != const_int_map.end()) {
        ret = it->second;
    } else {
        ret = new ConstantInt(int_val);
        const_int_map[int_val] = ret;
    }
    return ret;
}

int ConstantInt::get_const_val() {
    return u.i;
}

std::string ConstantInt::getName() const {
    return std::to_string(u.i);
}

ConstantFloat::ConstantFloat(float fv) : Constant(BasicType::F32_TYPE) {
    u.f = fv;
    v = new IFV(fv);
    _isConstant = true;
}

std::string ConstantFloat::getName() const {
    // 将 double 型的 constFloatVal 转换为 16 进制的字符串
    std::stringstream ss;
    double d = (double) (u.f);
    long long lbits = *(long long *) (&d);
    ss << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(16) << lbits;
    return ss.str();
}

ConstantFloat::operator std::string() const {
    std::stringstream ss;
    if (CenterControl::AlreadyBackend) {
        int intValue = this->u.i;
        ss << "0x" << std::hex << std::setfill('0') << std::setw(8) << intValue;
    } else {
        ss << this->getName();
    }
    return ss.str();
}

int ConstantFloat::getIntBits() {
    return u.i;
}

bool ConstantFloat::isZero() const {
    return getName() == "0x0";
}

std::string ConstantFloat::getAsmName() {
    if (asm_name == "") {
        asm_name = "_F_CONST_" +std::to_string(float_const_cnt++);
    }
    return asm_name;
}

float ConstantFloat::get_const_val() {
    return u.f;
}

ConstantBool::ConstantBool(int val) : Constant(BasicType::I1_TYPE) {
    u.i = val;
}

int ConstantBool::get_const_val() {
    return u.i;
}

ConstantBool::operator std::string() const {
    return std::to_string(this->u.i);
}

std::string ConstantBool::getName() const {
    return std::string(*this);
}
