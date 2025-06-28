//
// Created by XuRuiyuan on 2023/7/14.
//

#ifndef FASTER_MEOW_USE_H
#define FASTER_MEOW_USE_H


// #include "Value.h"
// #include "Instr.h"
#include "../util/ILinkNode.h"

class Value;

class Instr;

class Use : public ILinkNode {
public:
    Instr * user{};
    Value *used{};
    int idx = 0;
    static int use_num;
    int id = ++use_num;
public:
    Use() = default;

    Use(Instr *user, Value *used, int idx) :
            user(user), used(used), idx(idx) {};

    ~Use() override = default;

    friend bool operator==(const Use &first, const Use &second);

    friend bool operator!=(const Use &first, const Use &second);

    friend bool operator<(const Use &first, const Use &second);

    virtual explicit operator std::string() const;
};

#endif //FASTER_MEOW_USE_H
