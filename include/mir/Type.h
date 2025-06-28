//
// Created by XuRuiyuan on 2023/7/14.
//

#ifndef FASTER_MEOW_TYPE_H
#define FASTER_MEOW_TYPE_H

#include "util.h"
#include <vector>
#include "memory"
#include "string"

// class VoidType;

enum class TypeTag {
    int1_type = 0,
    int32_type = 1,
    float_type = 2,
    void_type = 3,
    pointer_type = 4,
    array_type = 5,
    bb_type = 6,
};

class Type {
public:
    TypeTag type_tag = TypeTag::void_type;
public:
    // Type() = default;

    virtual ~Type() = default;

    explicit Type(TypeTag type_tag);

    bool is_void_type() const;

    bool is_int1_type() const;

    bool is_int32_type() const;

    bool is_float_type() const;

    bool is_pointer_type() const;

    bool is_basic_type() const;

    bool is_array_type() const;

    bool is_bb_type();

    virtual bool operator==(const Type &other) const;

    virtual bool operator!=(const Type &other) const;

    //friend bool operator==(const void* other, const void* other2) const;
    // friend bool operator==(const Type *first, const Type *second);
    //
    // friend bool operator!=(const Type *first, const Type *second);
    friend std::ostream &operator<<(std::ostream &ostream, const Type *type);
    // friend bool operator<(const Type *first, const Type *second);

    virtual explicit operator std::string() const {
        return "!type!";
    }
};

//
class BasicType : public Type {
public:
    DataType data_type{DataType::I32};
    static BasicType *I32_TYPE;
    static BasicType *I1_TYPE;
    static BasicType *F32_TYPE;

public:
    ~BasicType() override = default;

    std::string to_string();

    explicit BasicType(const DataType &dataType);

    friend std::ostream &operator<<(std::ostream &ostream, const BasicType *type);

    bool operator==(const Type &other) const override;

    bool operator!=(const Type &other) const override;

    bool operator==(const BasicType &other) const;

    bool operator!=(const BasicType &other) const;

    virtual explicit operator std::string() const override {
        if (data_type == DataType::I32) {
            return "i32";
        } else if (data_type == DataType::F32) {
            return "float";
        } else {
            return "i1";
        }
    }
};

class VoidType : public Type {
public:
    static VoidType *VOID_TYPE;
public:
    VoidType() : Type(TypeTag::void_type) {}

    ~VoidType() override = default;

    bool operator==(const Type &other) const override;

    bool operator!=(const Type &other) const override;

    virtual explicit operator std::string() const override {
        return "void";
    }
};

class BBType : public Type {
public:
    static Type *BB_TYPE;
public:
    ~BBType() override = default;

    BBType() : Type(TypeTag::bb_type) {}

    bool operator==(const Type &other) const override;

    bool operator!=(const Type &other) const override;

    virtual explicit operator std::string() const override {
        return "b ";
    }
};

class ArrayType : public Type {
public:

    int size;
    Type *base_type;
    // BasicType base_ele_type;
    std::vector<int> dims;

public:
    ArrayType(int size, Type *base_type);

    ~ArrayType() override = default;

    bool operator==(const ArrayType &other) const;

    bool operator!=(const ArrayType &other) const;

    bool operator==(const Type &other) const override;

    bool operator!=(const Type &other) const override;

    // bool operator==(const ArrayType &other) const;
    //
    // bool operator==(const Type &other) const;
    int getDimSize() const;

    int getFlattenSize() const;

    BasicType *getBaseEleType() const;

    virtual explicit operator std::string() const override {
        return "[" + std::to_string(size) + " x " + std::string(*base_type) + "]";
    }
};

class PointerType : public Type {
public:
    Type *inner_type;
public:
    explicit PointerType(Type *inner_type);

    ~PointerType() override = default;

    bool operator==(const PointerType &other) const;

    bool operator!=(const PointerType &other) const;

    bool operator==(const Type &other) const override;

    bool operator!=(const Type &other) const override;

    virtual explicit operator std::string() const override {
        return std::string(*inner_type) + "*";
    }
};

#endif //FASTER_MEOW_TYPE_H
