//
// Created by XuRuiyuan on 2023/7/18.
//

#ifndef FASTER_MEOW_EVALUATE_H
#define FASTER_MEOW_EVALUATE_H

#include "Symbol.h"
#include "AST.h"
#include "util/util.h"

namespace Evaluate {

    extern IFV *evalUnaryConstExp(AST::UnaryExp *p);

    extern IFV *evalBinaryConstExp(AST::BinaryExp *p);

    extern IFV *evalConstExp(AST::Exp *pExp);

    extern int evalConstIntExp(AST::Exp *exp);

    extern float evalConstFloatExp(AST::Exp *exp);

    extern IFV *evalPrimaryExp(AST::PrimaryExp *exp);

    extern IFV *evalNumber(AST::Number *number);

    extern IFV *evalLVal(AST::LVal *lVal);

    extern IFV *binaryCalcHelper(Token *op, IFV *src1, IFV *src2);

    extern IFV *unaryCalcHelper(Token *op, IFV *src);
};

#endif //FASTER_MEOW_EVALUATE_H
