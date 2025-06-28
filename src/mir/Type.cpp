//
// Created by XuRuiyuan on 2023/7/14.
//
#include "../../include/mir/Instr.h"
#include "../../include/mir/Type.h"

#include <utility>
#include "memory"
#include "ostream"

#define __ComparePointer__(__typename__, a, b) (*(dynamic_cast<const __typename__ *>(a)) == *(dynamic_cast<const __typename__ *>(b)))
#define __CompareRef__(__typename__, a, b) dynamic_cast<const __typename__ &>(a) == dynamic_cast<const __typename__ &>(b)

bool VoidType::operator==(const Type &other) const {
    return type_tag == TypeTag::void_type;
}

bool VoidType::operator!=(const Type &other) const {
    return type_tag != TypeTag::void_type;
}

Type *BBType::BB_TYPE = new BBType();

bool BBType::operator==(const Type &other) const {
    return type_tag == TypeTag::bb_type;
}

bool BBType::operator!=(const Type &other) const {
    return type_tag != TypeTag::bb_type;
}
namespace TypeDeal {
    const std::string typeTag2str[7] = {"i1", "i32", "float", "void", "!p!", "!arr!", "b "};
    const TypeTag dataType2typeTag[3] = {TypeTag::int1_type, TypeTag::int32_type, TypeTag::float_type};

    std::string to_string(TypeTag tag) {
        return typeTag2str[(int) tag];
    }

}

std::string BasicType::to_string() {
    return TypeDeal::to_string(this->type_tag);
}

bool Type::is_void_type() const {
    return type_tag == TypeTag::void_type;
}

bool Type::is_basic_type() const {
    return type_tag <= TypeTag::float_type;
}

bool Type::is_float_type() const {
    return type_tag == TypeTag::float_type;
}

bool Type::is_int1_type() const {
    return type_tag == TypeTag::int1_type;
}

bool Type::is_int32_type() const {
    return type_tag == TypeTag::int32_type;
}

bool Type::is_pointer_type() const {
    return type_tag == TypeTag::pointer_type;
}

bool Type::is_array_type() const {
    return type_tag == TypeTag::array_type;
}

bool Type::is_bb_type() {
    return type_tag == TypeTag::bb_type;
}

Type::Type(TypeTag type_tag) {
    this->type_tag = type_tag;
}

bool Type::operator==(const Type &other) const {
    if (type_tag != other.type_tag) {
        return false;
    }
    // return this == &other;
    if (is_basic_type()) {
        return __CompareRef__(BasicType, *this, other);
    } else if (is_array_type()) {
        return __CompareRef__(ArrayType, *this, other);
    } else if (is_pointer_type()) {
        return __CompareRef__(PointerType, *this, other);
    } else {
        return true;
    }
}

bool Type::operator!=(const Type &other) const {
    return !(*this == other);
}

// bool operator<(const Type *first, const Type *second);

BasicType::BasicType(const DataType &_data_type) : Type(TypeDeal::dataType2typeTag[(int) _data_type]) {
    this->data_type = _data_type;
}

bool BasicType::operator==(const BasicType &other) const {
    return type_tag == other.type_tag;
}

bool BasicType::operator!=(const BasicType &other) const {
    return type_tag != other.type_tag;
}

bool ArrayType::operator==(const ArrayType &other) const {
    // return equals_arr_sp(this, other_type);
    if (this->size != other.size) return false;
    if (other.base_type->is_array_type() && this->base_type->is_array_type()) {
        return __ComparePointer__(ArrayType, this->base_type, other.base_type);
    } else if (other.base_type->is_basic_type() && this->base_type->is_basic_type()) {
        return __ComparePointer__(BasicType, this->base_type, other.base_type);
    }
    return false;
}

bool ArrayType::operator!=(const ArrayType &other) const {
    return !(*this == other);
}

bool ArrayType::operator==(const Type &other) const {
    if (other.type_tag != TypeTag::array_type) {
        return false;
    }
    return operator==(dynamic_cast<const ArrayType &>(other));
}

bool ArrayType::operator!=(const Type &other) const {
    if (other.type_tag != TypeTag::array_type) {
        return true;
    }
    return !operator==(dynamic_cast<const ArrayType &>(other));
}

ArrayType::ArrayType(int size, Type *base_type) : Type(TypeTag::array_type), size(size), base_type(base_type) {
    dims.push_back(size);
    if (base_type->is_array_type()) {
        auto arr_type = dynamic_cast<const ArrayType *>(base_type);
        for (int dim: arr_type->dims) {
            dims.push_back(dim);
        }
    }
}

int ArrayType::getDimSize() const {
    if (base_type->is_array_type()) {
        return ((ArrayType *) base_type)->getDimSize() + 1;
    }
    return 1;
}

int ArrayType::getFlattenSize() const {
    // TODO: 没有做高速缓存, 保证更改后不会出问题
    if (base_type->is_basic_type()) {
        return size;
    }
    return ((ArrayType *) base_type)->getFlattenSize() * size;
}

BasicType *ArrayType::getBaseEleType() const {
    if (base_type->is_basic_type()) {
        return (BasicType *) base_type;
    }
    return ((ArrayType *) base_type)->getBaseEleType();
}

PointerType::PointerType(Type *inner_type) : Type(TypeTag::pointer_type), inner_type(inner_type) {}

bool PointerType::operator==(const PointerType &other) const {
    return this->inner_type == other.inner_type;
}

bool PointerType::operator!=(const PointerType &other) const {
    return this->inner_type != other.inner_type;
}

bool PointerType::operator==(const Type &other) const {
    if (type_tag == TypeTag::pointer_type) {
        return operator==(dynamic_cast<const PointerType &>(other));
    }
    return false;
}

bool PointerType::operator!=(const Type &other) const {
    if (type_tag != TypeTag::pointer_type) {
        return true;
    }
    return !operator==(dynamic_cast<const PointerType &>(other));

}

std::ostream &operator<<(std::ostream &ostream, const Type *type) {
    if (type->is_array_type()) {
        ostream << dynamic_cast<const ArrayType *>(type);
    } else if (type->is_pointer_type()) {
        ostream << dynamic_cast<const PointerType *>(type);
    } else {
        ostream << TypeDeal::typeTag2str[(int) type->type_tag];
    }
    return ostream;
}

std::ostream &operator<<(std::ostream &ostream, const ArrayType *type) {
    ostream << "[" << type->size << " x " << type->base_type << "]";
    return ostream;
}

std::ostream &operator<<(std::ostream &ostream, const PointerType *type) {
    ostream << type->inner_type << "*";
    return ostream;
}

std::ostream &operator<<(std::ostream &ostream, const BasicType *type) {
    ostream << TypeDeal::typeTag2str[(int) type->type_tag];
    return ostream;
}

bool BasicType::operator==(const Type &other) const {
    return Type::operator==(other);
}

// todo
bool BasicType::operator!=(const Type &other) const {
    return Type::operator!=(other);
}
