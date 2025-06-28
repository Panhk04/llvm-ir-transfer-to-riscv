//
// Created by XuRuiyuan on 2023/7/18.
//

#ifndef FASTER_MEOW_SYMBOL_H
#define FASTER_MEOW_SYMBOL_H

#include <string>
#include <utility>
#include "Initial.h"
#include "map"

class Type;

class Value;

class Symbol {
public:
    std::string name;
    Type *type;
    Initial *initial;
    bool is_constant;
    Value *value;

    Symbol(std::string name, Type *type, Initial *initial, bool is_constant, Value *value)
            : name(std::move(name)), type(type), initial(initial), is_constant(is_constant), value(value) {}
};

class SymTable {
public:
    std::map<std::string, Symbol *> symbol_map;
    SymTable *parent{nullptr};

    SymTable() = default;

    SymTable(SymTable *parent) : parent(parent) {}

    bool has_parent() {
        return parent != nullptr;
    }

    void add(Symbol *symbol) {
        symbol_map[symbol->name] = symbol;
    }

    Symbol *got(const std::string &name) {
        auto it = symbol_map.find(name);
        if (it == symbol_map.end() && parent != nullptr) {
            return parent->got(name);
        }
        return it->second;
    }
};

#endif //FASTER_MEOW_SYMBOL_H
