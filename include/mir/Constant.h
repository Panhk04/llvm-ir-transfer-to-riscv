//
// Created by XuRuiyuan on 2023/7/18.
//

#ifndef FASTER_MEOW_CONSTANT_H
#define FASTER_MEOW_CONSTANT_H

#include "Value.h"
#include "map"
#include <iostream>
#include "util/util.h"

class Constant : public Value {
public:
    static int const_num;
    int id = const_num++;
    IFU u;
    IFV *v;
public:
    Constant() = delete;

    Constant(Type *type);

    ~Constant() override = default;

    virtual explicit operator std::string() const override = 0;

    bool operator==(const Constant &other) const;

    IFV *getConstVal() const { return v; };
};

class ConstantInt : public Constant {
public:
    // int const_int_val;
    static std::map<int, ConstantInt *> const_int_map;
    static ConstantInt *CONST_0;
    static ConstantInt *CONST_1;

public:
    explicit ConstantInt(int iv);

    ~ConstantInt() override = default;

    static ConstantInt *get_const_int(int int_val);

    int get_const_val();

    std::string getName() const override;

    virtual explicit operator std::string() const override;
    // bool &operator==(const Value &constant_int);

};

class ConstantFloat : public Constant {
public:
    // float const_float_val;
    // static std::map<float, ConstantFloat> const_float_map;
    static ConstantFloat *CONST_0F;
    static ConstantFloat *CONST_1F;
    static int float_const_cnt;
    std::string asm_name = "_F_CONST_" + std::to_string(float_const_cnt++);
public:
    ConstantFloat(float fv);

    ~ConstantFloat() override = default;

    float get_const_val();

    std::string getName() const override;

    // int get_int_bits();

    int getIntBits();

    bool isZero() const;

    std::string getAsmName();

    virtual explicit operator std::string() const override;
};

class ConstantBool : public Constant {
public:
    explicit ConstantBool(int val);

    ~ConstantBool() override = default;

    int get_const_val();

    virtual explicit operator std::string() const override;

    std::string getName() const override;
};

#endif //FASTER_MEOW_CONSTANT_H
