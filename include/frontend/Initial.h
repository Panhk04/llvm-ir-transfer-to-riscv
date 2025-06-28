//
// Created by Administrator on 2023/7/16.
//

#ifndef FASTER_MEOW_INITIAL_H
#define FASTER_MEOW_INITIAL_H

#include "memory"
#include "vector"
#include "set"
#include "mir/Constant.h"

class BasicType;

class Type;

class Value;

class Flatten {
public:
    Flatten();
    int size = 0;
    class Entry : public ILinkNode {
    public:
        Value *value{};
        int count{};

        Entry() = default;

        Entry(Value *value, int count);

        bool isZero();

        bool canMerge(Entry *other);

        void merge(Entry *other);

        virtual explicit operator std::string() const;
    };

    Entry *begin;
    Entry *end;

    bool isZero();

    void concat(Flatten *that);

    void mergeAll();

    int sizeInWords();

    int sizeInBytes();

    std::map<int, Value *> *listNonZeros();

    std::set<Value *> *valueSet();

    virtual explicit operator std::string() const;
};

class Initial {
public:
    Type *type;
public:
    Initial(Type *type);

    virtual ~Initial() = default;

    virtual Flatten *flatten() = 0;

    virtual explicit operator std::string() const = 0;
};

class ArrayInit : public Initial {
public:
    std::vector<Initial *> inits;
public:
    ArrayInit(Type *type);

    ~ArrayInit() override = default;

    int length() const;

    void add(Initial *init);

    Initial *get(int idx);

    static std::string to_string();

    Flatten *flatten() override;

    virtual explicit operator std::string() const override;

};

class ValueInit : public Initial {
public:
    explicit ValueInit(Value *pFloat, Type *pType);

    Value *value;

    Flatten *flatten() override;

    virtual explicit operator std::string() const override;
};

class ZeroInit : public Initial {

public:
    Flatten * flatten() override;

    explicit ZeroInit(Type *pType);

    virtual explicit operator std::string() const override;
};

class ExpInit : public Initial {
public:
    explicit ExpInit(Value *pValue, BasicType *pType);

    Value *result;

    Flatten *flatten() override;

    virtual explicit operator std::string() const override;
};


#endif //FASTER_MEOW_INITIAL_H
