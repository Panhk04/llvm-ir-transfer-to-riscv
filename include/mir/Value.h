//
// Created by Administrator on 2023/7/13.
//

#ifndef FASTER_MEOW_VALUE_H
#define FASTER_MEOW_VALUE_H

#include <string>
#include "../util/ILinkNode.h"

class Use;

enum class TypeTag;

class Type;

enum class ValueTag {
    value,
    func,
    bb,
    bino,
    ashr,
    icmp,
    fcmp,
    fneg,
    zext,
    fptosi,
    sitofp,
    alloc,
    load,
    store,
    gep,
    bitcast,
    call,
    phi,
    pcopy,
    move,
    branch,
    jump,
    ret,
};

class Value : public ILinkNode {
public:
    std::string prefix;
    std::string name;
public:
    // Value *sp;
    ValueTag tag = ValueTag::value;
    static std::string GLOBAL_PREFIX;
    static std::string LOCAL_PREFIX;
    static std::string GLOBAL_NAME_PREFIX;
    static std::string LOCAL_NAME_PREFIX;
    static std::string FPARAM_NAME_PREFIX;

    static int value_num;
    bool _isConstant = false;
    bool isGlobal = false;
    // Value *p_this;

    int id = value_num++;
    Use *beginUse{nullptr};
    Use *endUse{nullptr};
    Type *type{nullptr};

public:
    Value();

    Value(Type *type);

    ~Value() override = default;

    void insertAtEnd(Use *use);

    // Value *get_this();

    virtual std::string getName() const;

    Use *getBeginUse();

    Use *getEndUse();

    bool onlyOneUser();

    virtual std::string getDescriptor() const;

    bool isNoUse();

    bool isAlu();

    bool isStore();

    bool isLoad();

    bool isAlloc();

    bool isGep();

    bool isCall();

    bool isPhi();

    bool isIcmp();

    bool isFcmp();

    bool isBranch();

    bool isJump();

    bool isBJ();

    bool isReturn();

    bool isConstant();

    bool isConstantInt();

    bool isConstantFloat();

    bool operator==(const Value &other) const;

    bool operator!=(const Value &other) const;

    bool operator<(const Value &other) const;

    virtual explicit operator std::string() const {
        return "!value!";
    }

    void modifyAllUseThisToUseA(Value *A);

    static std::string tagStrList[24];
};

#endif //FASTER_MEOW_VALUE_H
