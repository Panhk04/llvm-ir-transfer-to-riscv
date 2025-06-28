//
// Created by Administrator on 2023/7/16.
//

#include "../../include/frontend/Initial.h"
#include "iostream"
#include <sstream>
#include "mir/Type.h"
#include "mir/Instr.h"
#include <cassert>

// #define ER 29

Initial::Initial(Type *type) : type(type) {}

ArrayInit::ArrayInit(Type *type) : Initial(type) {}

int ArrayInit::length() const {
    return inits.size();
}

void ArrayInit::add(Initial *init) {
    inits.push_back(init);
}

Initial *ArrayInit::get(int idx) {
    return inits[idx];
}

std::ostream &operator<<(std::ostream &ostream, const Initial *init) {
    if (auto p = dynamic_cast<const ArrayInit *>(init)) {
        ostream << p;
    } else if (auto p1 = dynamic_cast<const ValueInit *>(init)) {
        ostream << p1;
    }
    return ostream;
}

std::ostream &operator<<(std::ostream &ostream, const ArrayInit *init) {
    ostream << init->type << std::string(" [");
    for (Initial *i: init->inits) {
        ostream << i;
    }
    ostream << std::string("]");
    return ostream;
}

std::string ArrayInit::to_string() {
    // todo
    return "!";
}

ArrayInit::operator std::string() const {
    std::stringstream ss;
    for (int i = 0; i < inits.size(); ++i) {
        ss << std::string(*inits[i]);
        if (i != inits.size() - 1) {
            ss << ", ";
        }
    }
    return std::string(*type) + " [" + ss.str() + "]";
}

Flatten *ArrayInit::flatten() {
    Flatten *flat = new Flatten();
    for (Initial *init: inits) {
        Flatten *flat1 = init->flatten();
        flat->concat(flat1);
        flat->mergeAll();
    }
    return flat;
}

ZeroInit::ZeroInit(Type *pType) : Initial(pType) {}

ZeroInit::operator std::string() const {
    return std::string(*type) + " zeroinitializer";
}

Flatten *ZeroInit::flatten() {
    int size;
    if(type->is_array_type()) {
        size = ((ArrayType *) type)->getFlattenSize();
    }else {
        assert(type->is_basic_type());
        size = 1;
    }
    Flatten *flat = new Flatten();
    flat->end->insert_before(new Flatten::Entry(ConstantInt::CONST_0, size));
    return flat;
}

ValueInit::ValueInit(Value *v, Type *pType) : Initial(pType), value(v) {}

Flatten *ValueInit::flatten() {
    Flatten *flat = new Flatten();
    flat->end->insert_before(new Flatten::Entry(value, 1));
    return flat;
}

ValueInit::operator std::string() const {
    return std::string(*type) + " " + std::string(*value);
}

ExpInit::ExpInit(Value *pValue, BasicType *pType) : Initial((Type *) pType), result(pValue) {}

Flatten *ExpInit::flatten() {
    Flatten *flat = new Flatten();
    flat->end->insert_before(new Flatten::Entry(result, 1));
    return flat;
}

ExpInit::operator std::string() const {
    exit(32);
}

bool Flatten::isZero() {
    for (Entry *e = (Entry *) begin->next; e->next != nullptr; e = (Entry *) e->next) {
        if (!e->isZero()) return false;
    }
    return true;
}

void Flatten::concat(Flatten *that) {
    if (that->begin->next->next == nullptr) {
        return;
    }
    that->begin->next->prev = end->prev;
    that->end->prev->next = end;
    end->prev->next = that->begin->next;
    end->prev = that->end->prev;
    size += that->size;
}

void Flatten::mergeAll() {
    int sizeBefore = sizeInWords();
    for (Entry *cur = (Entry *) begin->next; cur->next != nullptr; cur = (Entry *) cur->next) {
        while (cur->next->next != nullptr && cur->canMerge((Entry *) cur->next)) {
            cur->merge((Entry *) cur->next);
            size--;
        }
    }
    if (sizeInWords() != sizeBefore) {
        exit(31);
    }
}

int Flatten::sizeInWords() {
    int size = 0;
    for (Entry *cur = (Entry *) begin->next; cur->next != nullptr; cur = (Entry *) cur->next) {
        size += cur->count;
    }
    return size;
}

int Flatten::sizeInBytes() {
    return sizeInWords() * 4;
}

std::map<int, Value *> *Flatten::listNonZeros() {
    int id = 0;
    std::map<int, Value *> *nonZeros = new std::map<int, Value *>();
    for (Entry *cur = (Entry *) begin->next; cur->next != nullptr; cur = (Entry *) cur->next) {
        if (!cur->isZero()) {
            for (int k = 0; k < cur->count; k++) {
                (*nonZeros)[id] = cur->value;
                id++;
            }
        } else {
            id += cur->count;
        }
    }
    return nonZeros;
}

std::set<Value *> *Flatten::valueSet() {
    std::set<Value *> *s = new std::set<Value *>();
    for (Entry *cur = (Entry *) begin->next; cur->next != nullptr; cur = (Entry *) cur->next) {
        s->insert(cur->value);
    }
    return s;
}

Flatten::Flatten() {
    begin = new Entry;
    end = new Entry;
    begin->next = end;
    end->prev = begin;
}

bool Flatten::Entry::isZero() {
    if (CAST_IF(p1, value, ConstantInt)) {
        return p1->u.i == 0;
    } else if (CAST_IF(p2, value, ConstantFloat)) {
        return p2->u.f == 0.0f;
    } else {
        return false;
    }
}

bool Flatten::Entry::canMerge(Flatten::Entry *other) {
    return *value == *other->value;
}

void Flatten::Entry::merge(Flatten::Entry *other) {
    count += other->count;
    other->remove();
}

Flatten::Entry::operator std::string() const {
    std::string valueStr = std::to_string(((Constant *) value)->u.i);
    if (count == 1) {
        return ".word\t" + valueStr;
    } else {
        return ".fill\t" + std::to_string(count) + ", 4, " + valueStr;
    }
}

Flatten::Entry::Entry(Value *value, int count) : value(value), count(count) {}

Flatten::operator std::string() const {
    std::stringstream ss;
    for (Flatten::Entry *entry = (Flatten::Entry*) begin->next; entry->has_next(); entry = (Flatten::Entry*) entry->next) {
        ss << "\t" << std::string(*entry) << std::endl;
    }
    return ss.str();
}