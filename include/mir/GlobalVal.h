
#ifndef FASTER_MEOW_GLOBALVAL_H
#define FASTER_MEOW_GLOBALVAL_H

#include "string"
#include "../frontend/AST.h"
#include "../frontend/Initial.h"

class Value;

class Type;

class GlobalVal : public Value {
public:
    explicit GlobalVal(Type *type);

    GlobalVal() = default;

    ~GlobalVal() override = default;

    virtual explicit operator std::string() const override = 0;

};

class GlobalValue : public GlobalVal {
public:
    ~GlobalValue() override = default;

    GlobalValue(Type *pointeeType, AST::Def *def, Initial *initial);

    GlobalValue(Type *pointeeType, std::string name, Initial *initial);

public:

    AST::Def *def = nullptr;
    Initial *initial = nullptr;
    bool local = false;

    void setCanLocal();

    bool canLocal() const;

    virtual explicit operator std::string() const override;
};

struct CompareGlobalValue {
    bool operator()(const GlobalValue *lhs, const GlobalValue *rhs) const;
};

class UndefValue : public GlobalVal {
public:
    ~UndefValue() override = default;

    UndefValue() = default;

    explicit UndefValue(Type *type) : GlobalVal(type) {}

public:

    static int undefValueCnt;
    std::string label = "";
    std::string name = "";

    std::string getLabel() const;
    virtual std::string getName() const override;

    virtual explicit operator std::string() const override;
};

class VirtualValue : public GlobalVal {
public:
    ~VirtualValue() override = default;

public:

    static int virtual_value_cnt;

    explicit VirtualValue(Type *type);

    virtual explicit operator std::string() const override;
};

#endif //FASTER_MEOW_GLOBALVAL_H
