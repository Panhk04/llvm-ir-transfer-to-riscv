//
// Created by XuRuiyuan on 2023/7/31.
//

#include "frontend/Evaluate.h"
#include "frontend/Visitor.h"
#include <string>
#include <stdexcept>
#include <cmath>
#include <cassert>

// #define ER 17

IFV *Evaluate::evalConstExp(AST::Exp *pExp) {
    IFV *v;
    if (CAST_IF(p1, pExp, AST::BinaryExp)) {
        v = Evaluate::evalBinaryConstExp(p1);
    } else if (CAST_IF(p2, pExp, AST::UnaryExp)) {
        v = Evaluate::evalUnaryConstExp(p2);
    } else {
        exit(17 + 1);
    }
    if (std::get_if<int>(v) == nullptr && std::get_if<float>(v) == nullptr) {
        exit(17 + 2);
    }
    return v;
}

int Evaluate::evalConstIntExp(AST::Exp *exp) {
    return std::get<int>(*evalConstExp(exp));
}

float Evaluate::evalConstFloatExp(AST::Exp *exp) {
    return std::get<float>(*evalConstExp(exp));
}

IFV *Evaluate::evalBinaryConstExp(AST::BinaryExp *p) {
    IFV *v = evalConstExp(p->first);
    for (int i = 0; i < p->follows.size(); i++) {
        Token *op = p->operators[i];
        AST::Exp *follow = p->follows[i];
        v = binaryCalcHelper(op, v, evalConstExp(follow));
    }
    return v;
}

IFV *Evaluate::evalUnaryConstExp(AST::UnaryExp *p) {
    IFV *primary = evalPrimaryExp(p->primaryExp);
    for (int i = (int) p->unaryOps.size() - 1; i >= 0; i--) {
        Token *op = p->unaryOps[i];
        primary = unaryCalcHelper(op, primary);
    }
    return primary;
}

IFV *Evaluate::evalPrimaryExp(AST::PrimaryExp *exp) {
    if (CAST_IF(p1, exp, AST::Number)) {
        return evalNumber(p1);
    } else if (CAST_IF(p2, exp, AST::Exp)) {
        return evalConstExp(p2);
    } else if (CAST_IF(p3, exp, AST::LVal)) {
        return evalLVal(p3);
    } else {
        exit(17 + 3);
    }
}

IFV *Evaluate::evalNumber(AST::Number *number) {
    Token *num = number->number;
    std::string content = num->content;
    switch (num->token_type) {
        case Token::HEX_INT:
            return new IFV(std::stoi(content.substr(2), nullptr, 16));
        case Token::OCT_INT:
            return new IFV(std::stoi(content.substr(1), nullptr, 8));
        case Token::DEC_INT:
            return new IFV(std::stoi(content));
        case Token::HEX_FLOAT:
        case Token::DEC_FLOAT:
            return new IFV(std::stof(content));
        default:
            exit(17 + 4);
    }
}

IFV *Evaluate::evalLVal(AST::LVal *lVal) {
    std::string ident = lVal->ident->content;
    // 查找符号表
    Symbol *symbol = Visitor::currentSymTable->got(ident);
    // 必须初始化过，才可以编译期求值
    if (symbol->initial == nullptr) {
        exit(22);
    }
    // 如果是数组, 逐层找偏移量
    Initial *init = symbol->initial;
    std::vector<int> indexes; // eval indexes
    for (AST::Exp *index: lVal->indexes) {
        indexes.push_back(std::get<int>(*evalConstExp(index)));
    }
    for (int index: indexes) {
        if (init == nullptr) {
            exit(17 + 6);
        }
        if (__IS__(init, ZeroInit)) {
            return new IFV(0);
        }
        init = dynamic_cast<ArrayInit *>(init)->inits[index];
    }
    auto *p = dynamic_cast<ValueInit *>(init);
    if (__IS_NOT__(init, ValueInit)) {
        exit(17 + 7);
    }
    Value *ret = dynamic_cast<ValueInit *>(init)->value; // 取出初始值
    assert(ret->isConstant());
    return dynamic_cast<Constant *>(ret)->getConstVal();
}

float customFmod(float numerator, float denominator) {
    int quotient = static_cast<int>(numerator / denominator);
    float remainder = numerator - quotient * denominator;
    return remainder;
}

IFV *Evaluate::binaryCalcHelper(Token *op, IFV *src1, IFV *src2) {
    float *f1_ptr = std::get_if<float>(src1);
    float *f2_ptr = std::get_if<float>(src2);
    int *i1_ptr = std::get_if<int>(src1);
    int *i2_ptr = std::get_if<int>(src2);
    if (f1_ptr != nullptr || f2_ptr != nullptr) {
        float f1 = f1_ptr == nullptr ? ((float) *i1_ptr) : *f1_ptr;
        float f2 = f2_ptr == nullptr ? ((float) *i2_ptr) : *f2_ptr;
        if (op->token_type == Token::ADD) {
            return new IFV(f1 + f2);
        } else if (op->token_type == Token::SUB) {
            return new IFV(f1 - f2);
        } else if (op->token_type == Token::MUL) {
            return new IFV(f1 * f2);
        } else if (op->token_type == Token::DIV) {
            return new IFV(f1 / f2);
        } else if (op->token_type == Token::MOD) {
            return new IFV(customFmod(f1, f2));
        } else {
            exit(17 + 8);
        }
    } else {
        assert(i1_ptr != nullptr && i2_ptr != nullptr);
        int i1 = *i1_ptr;
        int i2 = *i2_ptr;
        switch (op->token_type) {
            case Token::ADD:
                return new IFV(i1 + i2);
            case Token::SUB:
                return new IFV(i1 - i2);
            case Token::MUL:
                return new IFV(i1 * i2);
            case Token::DIV:
                return new IFV(i1 / i2);
            case Token::MOD:
                return new IFV(i1 % i2);
            default:
                exit(17 + 9);
        }
    }
}

IFV *Evaluate::unaryCalcHelper(Token *op, IFV *src) {
    int *int_ptr = std::get_if<int>(src);
    float *float_ptr = std::get_if<float>(src);
    if (int_ptr != nullptr) {
        int intConst = *int_ptr;
        switch (op->token_type) {
            case Token::ADD:
                return new IFV(intConst);
            case Token::SUB:
                return new IFV(-intConst);
            case Token::NOT:
                return new IFV(intConst == 0 ? 1 : 0);
            default:
                exit(17 + 9);
        }
    } else if (float_ptr != nullptr) {
        float floatConst = *float_ptr;
        switch (op->token_type) {
            case Token::ADD:
                return new IFV(floatConst);
            case Token::SUB:
                return new IFV(-floatConst);
            case Token::NOT:
                return new IFV(floatConst == 0.0f ? 1.0f : 0.0f);
            default:
                exit(17 + 10);
        }
    } else {
        exit(17 + 11);
    }
}
