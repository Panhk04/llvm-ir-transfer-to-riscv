//
// Created by Administrator on 2023/7/28.
//

#include "frontend/Visitor.h"
#include "util/util.h"
#include "frontend/Initial.h"

using namespace INSTR;

Value *Visitor::trimTo(Value *value, BasicType *targetType) const {
    if (!(value->type->is_basic_type())) {
        exit(6);
    }
    // Value res = value;
    // if (value->token_type.equals(targetType)) {
    //     System.err.printf("Try to trim %s to %s\n", value, targetType);
    //     // return value;
    // }
    DataType dataType = ((BasicType *) value->type)->data_type;
    if (dataType == DataType::I1) {
        if (targetType->data_type == DataType::I32) {
            return new Zext(value, curBB);
        } else if (targetType->data_type == DataType::F32) {
            return new SItofp(new Zext(value, curBB), curBB);
        } else { return value; }
    } else if (dataType == DataType::I32) {
        // TODO: 关于ConstVal是新建一个实例还是复用同一个常数，有待考察，此处使用唯一常量
        if (targetType->data_type == DataType::I1) {
            return new Icmp(Icmp::Op::NE, value, ConstantInt::ConstantInt::CONST_0, curBB);
        } else if (targetType->data_type == DataType::F32) {
            return new SItofp(value, curBB);
        } else { return value; }
    } else {
        if (targetType->data_type == DataType::I1) {
            return new Fcmp(Fcmp::Op::ONE, value, ConstantFloat::ConstantFloat::CONST_0F, curBB);
        } else if (targetType->data_type == DataType::I32) {
            return new FPtosi(value, curBB);
        } else { return value; }
    }
}


INSTR::Alu::Op Visitor::aluOpHelper(Token *token) {
    if (token->token_type == Token::ADD)return INSTR::Alu::Op::ADD;
    else if (token->token_type == Token::SUB)return INSTR::Alu::Op::SUB;
    else if (token->token_type == Token::MUL)return INSTR::Alu::Op::MUL;
    else if (token->token_type == Token::DIV)return INSTR::Alu::Op::DIV;
    else if (token->token_type == Token::MOD)return INSTR::Alu::Op::REM;
    else exit(7);
}

INSTR::Alu::Op Visitor::aluOpHelper(Token *token, bool needFloat) {
    if (token->token_type == Token::ADD)return INSTR::Alu::Op::FADD;
    else if (token->token_type == Token::SUB)return INSTR::Alu::Op::FSUB;
    else if (token->token_type == Token::MUL)return INSTR::Alu::Op::FMUL;
    else if (token->token_type == Token::DIV)return INSTR::Alu::Op::FDIV;
    else if (token->token_type == Token::MOD)return INSTR::Alu::Op::FREM;
    else exit(8);
}

INSTR::Icmp::Op Visitor::icmpOpHelper(Token *token) {
    if (token->token_type == Token::GE) return INSTR::Icmp::Op::SGE;
    else if (token->token_type == Token::GT) return INSTR::Icmp::Op::SGT;
    else if (token->token_type == Token::LE) return INSTR::Icmp::Op::SLE;
    else if (token->token_type == Token::LT) return INSTR::Icmp::Op::SLT;
    else if (token->token_type == Token::EQ) return INSTR::Icmp::Op::EQ;
    else if (token->token_type == Token::NE) return INSTR::Icmp::Op::NE;
    else exit(9);
}

INSTR::Fcmp::Op Visitor::fcmpOpHelper(Token *token) {
    if (token->token_type == Token::GE) return INSTR::Fcmp::Op::OGE;
    else if (token->token_type == Token::GT) return INSTR::Fcmp::Op::OGT;
    else if (token->token_type == Token::LE) return INSTR::Fcmp::Op::OLE;
    else if (token->token_type == Token::LT) return INSTR::Fcmp::Op::OLT;
    else if (token->token_type == Token::EQ) return INSTR::Fcmp::Op::OEQ;
    else if (token->token_type == Token::NE) return INSTR::Fcmp::Op::ONE;
    else exit(9);
}

void Visitor::visitAst(AST::Ast *ast) {
    for (auto unit: ast->units) {
        visitCompUnit(unit);
    }
}

Value *Visitor::visitBinaryExp(AST::BinaryExp *exp) {
    Value *first = visitExp(exp->first);
    for (int i = 0; i < exp->operators.size(); i++) {
        AST::Exp *nextExp = exp->follows[i];
        Token *op = exp->operators[i];
        if (isAlu(op)) {
            Value *second = visitExp(nextExp);
            if (hasFloatType(first, second)) {
                first = trimTo(first, BasicType::F32_TYPE);
                second = trimTo(second, BasicType::F32_TYPE);
//                    if (op->token_type == Token::MOD) {
//                        // a % b = a - a / b;
//                        Value *tmp = new INSTR::Alu(BasicType::F32_TYPE, INSTR::Alu::Op::FDIV, first, second, curBB);
//                        tmp = new INSTR::Alu(BasicType::F32_TYPE, INSTR::Alu::Op::FMUL, tmp, second, curBB);
//                        first = new INSTR::Alu(BasicType::F32_TYPE, INSTR::Alu::Op::FSUB, first, tmp, curBB);
//                    } else {
//                        INSTR::Alu::Op aluOp = aluOpHelper(op, true);
//                        first = new INSTR::Alu(BasicType::F32_TYPE, aluOp, first, second, curBB);
//                    }
                INSTR::Alu::Op aluOp = aluOpHelper(op, true);
                first = new INSTR::Alu(BasicType::F32_TYPE, aluOp, first, second, curBB);
            } else {
                first = trimTo(first, BasicType::I32_TYPE);
                second = trimTo(second, BasicType::I32_TYPE);
//                    if (op->token_type == Token::MOD) {
//                        // a % b = a - a / b * b;
//                        Value *tmp = new INSTR::Alu(BasicType::I32_TYPE, INSTR::Alu::Op::DIV, first, second, curBB);
//                        tmp = new INSTR::Alu(BasicType::I32_TYPE, INSTR::Alu::Op::MUL, tmp, second, curBB);
//                        first = new INSTR::Alu(BasicType::I32_TYPE, INSTR::Alu::Op::SUB, first, tmp, curBB);
//                    } else {
//                        INSTR::Alu::Op aluOp = aluOpHelper(op);
//                        first = new INSTR::Alu(BasicType::I32_TYPE, aluOp, first, second, curBB);
//                    }
                INSTR::Alu::Op aluOp = aluOpHelper(op);
                first = new INSTR::Alu(BasicType::I32_TYPE, aluOp, first, second, curBB);
            }
        } else if (isCmp(op)) {
            Value *second = visitExp(nextExp);
            if (hasFloatType(first, second)) {
                first = trimTo(first, BasicType::F32_TYPE);
                second = trimTo(second, BasicType::F32_TYPE);
                INSTR::Fcmp::Op
                        fcmpOp = fcmpOpHelper(op);
                first = new INSTR::Fcmp(fcmpOp, first, second, curBB);
            } else {
                first = trimTo(first, BasicType::I32_TYPE);
                second = trimTo(second, BasicType::I32_TYPE);
                INSTR::Icmp::Op
                        icmpOp = icmpOpHelper(op);
                first = new INSTR::Icmp(icmpOp, first, second, curBB);
            }
        } else if (isLogical(op)) {
            exit(8);
        } else {
            exit(9);
        }
    }
    return first;
}

Value *Visitor::visitUnaryExp(AST::UnaryExp *exp) {
    Value *primary = visitPrimaryExp(exp->primaryExp);
    // 从右向左结合
    for (int i = exp->unaryOps.size() - 1; i >= 0; i--) {
        Token *op = exp->unaryOps[i];
        if (op->token_type == Token::NOT) {
            // TODO: 不确定这个是不是对, 与LLVM IR一致
            if (primary->type->is_float_type())
                primary = new INSTR::Fcmp(INSTR::Fcmp::Op::OEQ, primary, ConstantFloat::CONST_0F, curBB);
            else if (primary->type->is_int32_type())
                primary = new INSTR::Icmp(INSTR::Icmp::Op::EQ, primary, ConstantInt::CONST_0, curBB);
            else {
                primary = trimTo(primary, BasicType::I32_TYPE);
                primary = new INSTR::Icmp(INSTR::Icmp::Op::EQ, primary, ConstantInt::CONST_0, curBB);
                // TODO: 大胆而激进的做法
                // 有非仅下面一条指令调用的风险
                // if(primary dynamic_cast<INSTR::Icmp){
                //     INSTR::Icmp icmp = (INSTR::Icmp) primary;
                //     switch (icmp.getOp()){
                //         else if (token->token_type == Token::EQ -> icmp.setOp(INSTR::Icmp::Op::NE);
                //         else if (token->token_type == Token::NE -> icmp.setOp(INSTR::Icmp::Op::EQ);
                //         else if (token->token_type == Token::SGE -> icmp.setOp()
                //     }
                // }
                // throw new AssertionError(std::string.format("Bad primary ! %s", primary));
            }
        } else {
            if (op->token_type == Token::SUB) {
                if (primary->type->is_float_type())
                    primary = new INSTR::Alu(BasicType::F32_TYPE, INSTR::Alu::Op::FSUB, ConstantFloat::CONST_0F,
                                             primary, curBB);
                else if (primary->type->is_int32_type())
                    primary = new INSTR::Alu(BasicType::I32_TYPE, INSTR::Alu::Op::SUB, ConstantInt::CONST_0, primary,
                                             curBB);
                else if (primary->type->is_int1_type()) {
                    primary = new INSTR::Alu(BasicType::I32_TYPE, INSTR::Alu::Op::SUB, ConstantInt::CONST_0,
                                             trimTo(primary, BasicType::I32_TYPE),
                                             curBB);
                } else {
                    exit(9);
                }
            }
        }
    }
    return primary;
}


Value *Visitor::visitExp(AST::Exp *exp) {
    if (auto *e1 = dynamic_cast<AST::BinaryExp *>(exp)) {
        return visitBinaryExp(e1);
    } else if (auto *e2 = dynamic_cast<AST::UnaryExp *>(exp)) {
        return visitUnaryExp(e2);
    } else {
        exit(10);
    }
}

Value *Visitor::visitNumber(AST::Number *number) {
    if (number->isFloatConst) {
        return new ConstantFloat(number->floatConstVal);
    } else if (number->isIntConst) {
        return new ConstantInt(number->intConstVal);
    } else {
        exit(11);
    }
}

Value *Visitor::visitPrimaryExp(AST::PrimaryExp *exp) {
    if (auto p1 = dynamic_cast<AST::Exp *>( exp)) {
        return visitExp(p1);
    } else if (auto p2 = dynamic_cast<AST::LVal *>(exp)) {
        return visitLVal(p2, false);
    } else if (auto p3 = dynamic_cast<AST::Number *>(exp)) {
        return visitNumber(p3);
    } else if (auto p4 = dynamic_cast<AST::Call *>(exp)) {
        return visitCall(p4);
    } else {
        exit(12);
    }
}

Value *Visitor::visitLVal(AST::LVal *lVal, bool needPointer) {
    // 去符号表拿出指向这个左值的指针
    std::string ident = lVal->ident->content;
    Symbol *symbol = currentSymTable->got(ident);
    if (symbol->is_constant && lVal->indexes.empty()) {
        // return symbol.getInitial();
        // assert !needPointer;
        return ((ValueInit *) symbol->initial)->value;
    }
    Value *pointer = symbol->value;
    // assert pointer dynamic_cast<Alloc;
    std::vector<Value *> *idxList = new std::vector<Value *>();
    idxList->push_back(ConstantInt::CONST_0);
    bool flag = false;
    Type *innerType = ((PointerType *) pointer->type)->inner_type; // 实体的类型
    for (AST::Exp *exp: lVal->indexes) {
        Value *offset = trimTo(visitExp(exp), BasicType::I32_TYPE);
        if (innerType->is_pointer_type()) {
            // 参数, 如int a[][...][...]...
            Instr *loadInst = new Load(pointer, curBB);
            innerType = ((PointerType *) innerType)->inner_type;
            // assert innerType->is_array_type();
            // innerType = ((ArrayType *) innerType).getBase();
#ifdef __ONLY_PARSE_OUTSIDE_DIM
            std::vector<Value *> *v1 = new std::vector<Value *>{offset};
            pointer = new GetElementPtr(innerType, loadInst, v1, curBB);
#else
            pointer = loadInst;
            flag = true;
            idxList->clear();
            idxList->push_back(offset);
#endif
        } else if (innerType->is_array_type()) {
            // 数组
            innerType = ((ArrayType *) innerType)->base_type;
#ifdef __ONLY_PARSE_OUTSIDE_DIM
            std::vector<Value *> v2 = {ConstantInt::CONST_0, offset};
                pointer = new GetElementPtr(innerType, pointer, v2, curBB);
#else
            flag = true;
            idxList->push_back(offset);
#endif
        } else {
            exit(13);
        }
    }
#ifndef __ONLY_PARSE_OUTSIDE_DIM
    if (flag) pointer = new GetElementPtr(innerType, pointer, idxList, curBB);
#endif
    if (needPointer) {
        return pointer; // 返回一个可以直接 store的指针
    }
    // 如果是数组元素或者普通的值, 就 load; 如果是部分数组(仅用于函数传参), 应该得到一个降维的数组指针;
    // 如果是将数组指针作为参数继续传递, 也需要 load 来解引用
    if (innerType->is_basic_type() || innerType->is_pointer_type()) {
        return new Load(pointer, curBB);
    } else if (innerType->is_array_type()) {
        // TODO
        std::vector<Value *> *v3 = new std::vector<Value *>{ConstantInt::CONST_0, ConstantInt::CONST_0};
        return new GetElementPtr(((ArrayType *) innerType)->base_type, pointer, v3, curBB);
    } else {
        exit(5);
    }
}

Value *Visitor::visitCall(AST::Call *call) {
    std::string ident = call->ident->content;
    Function *function = (*Manager::functions)[ident];
    if (function->isTimeFunc) {
        std::vector<Value *> *v = new std::vector<Value *>{new ConstantInt(call->lineno)};
        return new Call(function, v, curBB);
    } else {
        std::vector<Value *> *params = new std::vector<Value *>();
        int idx = 0;
        for (AST::Exp *exp: call->params) {
            Function::Param *p = (*function->params)[idx++];
            Value *expValue = visitExp(exp);
            if (p->type->is_basic_type()) {
                expValue = trimTo(expValue, (BasicType *) p->type);
            }
            params->push_back(expValue);
        }
        return new Call(function, params, curBB);
    }
}

void Visitor::visitAssign(AST::Assign *assign) {
    Value *right = visitExp(assign->right);
    Value *left = visitLVal(assign->left, true);
    Type *innerType = ((PointerType *) left->type)->inner_type;
    right = trimTo(right, (BasicType *) innerType);
    // assert right->token_type.equals(innerType); // 分析出来的右值一定是左值指针解引用的类型
    new Store(right, left, curBB);
}

void Visitor::visitExpStmt(AST::ExpStmt *expStmt) {
    AST::Exp *exp = expStmt->exp;
    if (exp != nullptr) {
        visitExp(expStmt->exp);
    }
}

Value *Visitor::visitCondLAnd(AST::Exp *exp, BasicBlock *falseBlock) {
    auto p = dynamic_cast<AST::BinaryExp *>(exp);
    if (p == nullptr) {
        return trimTo(visitExp(exp), BasicType::I1_TYPE);
    }
    AST::BinaryExp *lAndExp = (AST::BinaryExp *) exp;
    Value *first = trimTo(visitExp(lAndExp->first), BasicType::I1_TYPE);
    // Iterator <Token> iterOp = lAndExp.getOperators().listIterator();
    int i = 0;
    for (AST::Exp *nextExp: lAndExp->follows) {
        Token *op = lAndExp->operators[i++];
        BasicBlock *nextBlock = new BasicBlock(curFunc, curLoop); // 实为trueBlock的前驱
        new Branch(first, nextBlock, falseBlock, curBB);
        curBB = nextBlock;
        dealCondCount();
        first = trimTo(visitExp(nextExp), BasicType::I1_TYPE);
    }
    return first;
}

Value *Visitor::visitCondLOr(AST::Exp *exp, BasicBlock *trueBlock, BasicBlock *falseBlock) {
    auto p = dynamic_cast<AST::BinaryExp *>(exp);
    if (p == nullptr) {
        return trimTo(visitExp(exp), BasicType::I1_TYPE);
    }
    AST::BinaryExp *lOrExp = (AST::BinaryExp *) exp;
    bool flag = false;
    BasicBlock *nextBlock = falseBlock;
    BasicBlock *prevFalseBlock = falseBlock;
    if (!((AST::BinaryExp *) exp)->follows.empty()) {
        nextBlock = new BasicBlock(curFunc, curLoop); // 实为trueBlock的前驱
        flag = true;
    }
    Value *first = visitCondLAnd(lOrExp->first, nextBlock);
    int i = 0;
    for (AST::Exp *nextExp: lOrExp->follows) {
        Token *op = lOrExp->operators[i++];
        if (flag) {
            flag = false;
        } else {
            nextBlock = new BasicBlock(curFunc, curLoop); // 实为trueBlock的前驱
        }
        new Branch(first, trueBlock, nextBlock, curBB);
        curBB = nextBlock;
        dealCondCount();
        if (i < lOrExp->operators.size()) {
            flag = true;
            falseBlock = new BasicBlock(curFunc, curLoop);
            nextBlock = falseBlock;
        } else {
            falseBlock = prevFalseBlock;
        }
        first = visitCondLAnd(nextExp, falseBlock);
    }
    return first;
}

void Visitor::visitIfStmt(AST::IfStmt *ifStmt) {
    AST::Stmt *thenTarget = ifStmt->thenTarget;
    AST::Stmt *elseTarget = ifStmt->elseTarget;
    bool hasElseBlock = elseTarget != nullptr;
    BasicBlock *thenBlock = new BasicBlock(curFunc, curLoop);
    BasicBlock *elseBlock = nullptr;
    if (hasElseBlock) {
        elseBlock = new BasicBlock(curFunc, curLoop);
    }
    BasicBlock *followBlock = new BasicBlock(curFunc, curLoop);
    if (hasElseBlock) {
        inCond = true;
        dealCondCount();
        Value *cond = visitCondLOr(ifStmt->cond, thenBlock, elseBlock);
        new Branch(cond, thenBlock, elseBlock, curBB);
        curBB = thenBlock;
        inCond = false;
        visitStmt(thenTarget);
        new Jump(followBlock, curBB);
        curBB = elseBlock;
        visitStmt(elseTarget);
    } else {
        inCond = true;
        dealCondCount();
        Value *cond = visitCondLOr(ifStmt->cond, thenBlock, followBlock);
        new Branch(cond, thenBlock, followBlock, curBB);
        curBB = thenBlock;
        inCond = false;
        visitStmt(thenTarget); // currentBlock may be modified
    }
    new Jump(followBlock, curBB);
    curBB = followBlock;
}

void Visitor::visitWhileStmt(AST::WhileStmt *whileStmt) {
    curLoop = new Loop(curLoop);
    curLoop->setFunc(curFunc);
    BasicBlock *condBlock = new BasicBlock(curFunc, curLoop);
    condBlock->setLoopStart();
    new Jump(condBlock, curBB);
    BasicBlock *body = new BasicBlock(curFunc, curLoop);
    BasicBlock *follow = new BasicBlock(curFunc, curLoop->parentLoop);
    curBB = condBlock;
    inLoop = true;
    inCond = true;
    dealCondCount();
    Value *cond = visitCondLOr(whileStmt->cond, body, follow);
    new Branch(cond, body, follow, curBB);
    curBB = body;
    inCond = false;
    loopHeads.push(condBlock);
    loopFollows.push(follow);
    visitStmt(whileStmt->body);
    loopHeads.pop();
    loopFollows.pop();
    new Jump(condBlock, curBB);
    inLoop = false;
    curBB = follow;
    curLoop = curLoop->parentLoop;
}

void Visitor::visitBreak() {
    if (loopFollows.empty()) {
        exit(13);
    }
    new Jump(loopFollows.top(), curBB);
}

void Visitor::visitContinue() {
    if (loopHeads.empty()) {
        exit(14);
    }
    new Jump(loopHeads.top(), curBB);
}

void Visitor::visitReturn(AST::Return *ret) {
    AST::Exp *retExp = ret->value;
    if (retExp == nullptr) {
        new Return(curBB);
    } else {
        Value *retValue = visitExp(retExp);
        new Return(trimTo(retValue, (BasicType *) curBB->function->retType), curBB
        );
    }
}

void Visitor::visitStmt(AST::Stmt *stmt) {
    if (auto p1 = dynamic_cast<AST::Assign *>(stmt)) {
        visitAssign(p1);
    } else if (auto p2 = dynamic_cast<AST::ExpStmt *>(stmt)) {
        visitExpStmt(p2);
    } else if (auto p3 = dynamic_cast<AST::IfStmt *>(stmt)) {
        visitIfStmt(p3);
    } else if (auto p4 = dynamic_cast<AST::WhileStmt *>(stmt)) {
        visitWhileStmt(p4);
    } else if (auto p5 = dynamic_cast<AST::Break *>(stmt)) {
        visitBreak();
    } else if (auto p6 = dynamic_cast<AST::Continue *>(stmt)) {
        visitContinue();
    } else if (auto p7 = dynamic_cast<AST::Return *>(stmt)) {
        visitReturn(p7);
    } else if (auto p8 = dynamic_cast<AST::Block *>(stmt)) {
        visitBlock(p8, true);
    } else {
        exit(15);
    }
}

void Visitor::visitBlockItem(AST::BlockItem *item) {
    if (auto p1 = dynamic_cast<AST::Stmt *>(item)) {
        visitStmt(p1);
    } else if (auto p2 = dynamic_cast<AST::Decl *>(item)) {
        visitDecl(p2);
    } else {
        exit(16);
    }
}

void Visitor::visitBlock(AST::Block *block, bool sym) {
    if (sym) {
        currentSymTable = new SymTable(currentSymTable);
    } // 新开一层符号表
    for (AST::BlockItem *item: block->items) {
        visitBlockItem(item);
    }
    if (sym) {
        currentSymTable = currentSymTable->parent;
    } // 退出一层符号表
}

void Visitor::visitDecl(AST::Decl *decl) {
    for (AST::Def *def: decl->defs) {
        visitDef(def, decl->constant);
    }
}

void Visitor::initZeroHelper(Value *pointer, Type *pointeeType) {
    Value *ptr = pointer;
    int size = 4;
    std::vector<Value *> *idxList = new std::vector<Value *>();
    idxList->push_back(ConstantInt::CONST_0);
    while (pointeeType->is_array_type()) {
        size *= ((ArrayType *) pointeeType)->size;
        pointeeType = ((ArrayType *) pointeeType)->base_type;
#ifdef __ONLY_PARSE_OUTSIDE_DIM
        ptr = new GetElementPtr(pointeeType, ptr, v, curBB);
#else
        idxList->push_back(ConstantInt::CONST_0);
#endif
        std::vector<Value *> v = {ConstantInt::CONST_0, ConstantInt::CONST_0};
    }
#ifndef __ONLY_PARSE_OUTSIDE_DIM
    ptr = new GetElementPtr(pointeeType, ptr, idxList, curBB);
#endif
    // assert(((PointerType *) ptr->opdType)->inner_type->is_basic_type());
    if (((PointerType *) ptr->type)->inner_type->is_float_type()) {
        ptr = new BitCast(ptr, new PointerType(BasicType::I32_TYPE), curBB);
    }
    // CACT doesn't support memset, use store loop instead
    // TODO: Implement array initialization with explicit stores
    // For now, skip memset call since CACT doesn't provide it
    // std::vector<Value *> *v1 = new std::vector<Value *>{ptr, ConstantInt::CONST_0, new ConstantInt(size)};
    // new Call(ExternFunction::MEM_SET, v1, curBB);
}

void Visitor::initLocalVarHelper(Value *pointer, Initial *init) {
    Type *type = pointer->type;
    Type *baseType = ((PointerType *) type)->inner_type;
    if (auto arrayInit = dynamic_cast<ArrayInit *>(init)) {
        int len = arrayInit->length();
        for (int i = 0; i < len; i++) {
            std::vector<Value *> *v2 = new std::vector<Value *>{ConstantInt::CONST_0, new ConstantInt(i)};
            // PointerType * pointeeType = new PointerType *(((ArrayType *) baseType).getBase());
            Value *ptr = new GetElementPtr(((ArrayType *) baseType)->base_type, pointer, v2, curBB);
            initLocalVarHelper(ptr, arrayInit->get(i));
        }
    } else if (auto p1 = dynamic_cast<ZeroInit *>(init)) {
        initZeroHelper(pointer, baseType);
    } else {
        Value *v;
        if (auto p2 = dynamic_cast<ExpInit *>(init)) {
            v = trimTo(p2->result, (BasicType *) baseType);
        } else if (auto p3 = dynamic_cast<ValueInit *>(init)) {
            v = trimTo(p3->value, (BasicType *) baseType);
        } else {
            exit(15);
        }
        new Store(v, pointer, curBB);
    }
}

void Visitor::visitDef(AST::Def *def, bool constant) {
    bool eval = constant || isGlobal;
    std::string ident = def->ident->content;
    Type *pointeeType = (def->bType == Token::INT) ?
                        BasicType::I32_TYPE : BasicType::F32_TYPE;
    // 编译期计算数组每一维的长度，然后从右向左"组装"成数组类型
    std::vector<int> lengths;
    for (AST::Exp *len: def->indexes) {
        lengths.push_back(Evaluate::evalConstIntExp(len));
    }
    for (int i = lengths.size() - 1; i >= 0; i--) {
        int len = lengths[i];
        pointeeType = new ArrayType(len, pointeeType);
    }
    // 解析其初始化内容
    AST::Init *astInit = def->init;
    Initial *init = nullptr;
    if (astInit != nullptr) {
        if (pointeeType->is_basic_type()) {
            // if (!(astInit dynamic_cast<AST::Exp *)) {
            //     throw new SemanticException("Value variable not init by value");
            // }
            if (eval) {
                init = visitInitVal((BasicType *) pointeeType, dynamic_cast<AST::Exp *>(astInit));
            } else {
                init = visitInitExp((BasicType *) pointeeType, dynamic_cast<AST::Exp *>(astInit));
            }
        } else {
            // ArrayType *
            // if (!(astInit dynamic_cast<AST::AST::InitArray * *)) {
            //     throw new SemanticException("Array variable not init by a list");
            // }
            if (((AST::InitArray *) astInit)->init.empty()) {
                init = new ZeroInit(pointeeType);
            } else {
                init = visitInitArray((AST::InitArray *) astInit, (ArrayType *) pointeeType, constant, eval);
            }
        }
    }
    // 如果是全局变量且没有初始化，则初始化为零
    if ((isGlobal || _initAll) && init == nullptr) {
        if (pointeeType->is_basic_type()) {
            if (pointeeType == BasicType::F32_TYPE) {
                init = new ValueInit(ConstantFloat::CONST_0F, pointeeType);
            } else {
                init = new ValueInit(ConstantInt::CONST_0, pointeeType);
            }
        } else {
            init = new ZeroInit(pointeeType);
        }
    }
    // 全局: 直接给出初始化结果, 局部: 分配空间, store + memset 初始化的值
    Value *pointer;
    if (!isGlobal) {
        // 局部
        // 分配的空间指向 pointer
        pointer = new Alloc(pointeeType, curBB);
#ifdef __ONLY_PARSE_OUTSIDE_DIM
        if (pointeeType->is_array_type()) {
            initZeroHelper(pointer, pointeeType);
        }
        if (init != nullptr) {
            initLocalVarHelper(pointer, init); // 生成初始化
        }
#else
        // Type pointeeType = ((PointerType *) pointer->token_type)->inner_type;
        if (init != nullptr) {
            if (auto p1 = dynamic_cast<ExpInit *>(init)) {
                Value *v = trimTo(p1->result, (BasicType *) pointeeType);
                new Store(v, pointer, curBB);
            } else if (auto p2 = dynamic_cast<ValueInit *>(init)) {
                Value *v = trimTo(p2->value, (BasicType *) pointeeType);
                new Store(v, pointer, curBB);
            } else if (auto p3 = dynamic_cast<ZeroInit *>(init)) {
                initZeroHelper(pointer, pointeeType);
            } else {
                // assert init dynamic_cast<ArrayInit;
                // todo
                Flatten *flattenInit = init->flatten();
                std::set<Value *> *valueSet = flattenInit->valueSet();
                bool allZero = flattenInit->isZero();
                if (allZero) {
                    initZeroHelper(pointer, pointeeType);
                } else {
                    bool afterMemset = false;
                    if (flattenInit->sizeInWords() / 5 > valueSet->size() / 3) {
                        initZeroHelper(pointer, pointeeType);
                        afterMemset = true;
                    }
                    std::vector<Value *> *dimList = new std::vector<Value *>();
                    for (int i = 0; i <= ((ArrayType *) pointeeType)->getDimSize(); i++) {
                        dimList->push_back(ConstantInt::CONST_0);
                    }
                    BasicType *basicType = ((ArrayType *) pointeeType)->getBaseEleType();
                    Value *ptr = new GetElementPtr(basicType, pointer, dimList, curBB);
                    if (!(((Flatten::Entry *) flattenInit->begin->next)->isZero() && afterMemset)) {
                        auto *p = new Store(trimTo(((Flatten::Entry *) flattenInit->begin->next)->value, basicType),
                                            ptr, curBB);
                    }
                    std::map<int, Value *> *stores = flattenInit->listNonZeros();
                    if (!afterMemset) {
                        for (int i = 1; i < flattenInit->sizeInWords(); i++) {
                            if ((*stores)[i] == nullptr) {
                                (*stores)[i] = ConstantInt::CONST_0;
                            }
                        }
                    }
                    // System.err.println(stores);
                    for (auto entry: *stores) {
                        if (entry.first == 0) {
                            continue;
                        }
                        // System.err.println(entry);
                        dimList = new std::vector<Value *>();
                        dimList->push_back(ConstantInt::get_const_int(entry.first));
                        Value *p = new GetElementPtr(basicType, ptr, dimList, curBB);
                        new Store(trimTo(entry.second, basicType), p, curBB);
                    }
                }
            }
        }
#endif
    } else {
        pointer = new GlobalValue(pointeeType, def, init);
    }
    // 构建符号表项并插入符号表
    Symbol *symbol = new Symbol(ident, pointeeType, init, constant, pointer);
    currentSymTable->add(symbol);
    if (isGlobal) {
        manager->addGlobal(symbol);
    }
}

ArrayInit *Visitor::visitInitArray(AST::InitArray *initial, ArrayType *type, bool constant, bool eval) {
    ArrayInit *arrayInit = new ArrayInit(type);
    BasicType *baseEleType = type->getBaseEleType();
    Type *baseType = type->base_type;
    int needSize = type->size;
    int count = 0;
    while (count < needSize && initial->nowIdx < initial->init.size()) {
        AST::Init *init = initial->getNowInit();
        if (baseType->is_array_type()) {
            Initial *innerInit;
            if (auto p1 = dynamic_cast<AST::InitArray *>(init)) {
                innerInit = visitInitArray((AST::InitArray *) init, (ArrayType *) baseType, constant, eval);
                initial->nowIdx++;
            } else {
                innerInit = visitInitArray(initial, (ArrayType *) baseType, constant, eval);
            }
            arrayInit->add(innerInit);
        } else {
            AST::Exp *singleInit;
            if (auto p2 = dynamic_cast<AST::Exp *>(init)) {
                singleInit = p2;
            } else {
                AST::Init *preInit = ((AST::InitArray *) init)->init[0];
                singleInit = dynamic_cast<AST::Exp *>(preInit);
            }
            if (eval) {
                // 必须编译期计算
                arrayInit->add(visitInitVal(baseEleType, singleInit));
            } else {
                arrayInit->add(visitInitExp(baseEleType, singleInit));
            }
            initial->nowIdx++;
        }
        count++; // 统计已经初始化了多少个
    }
    while (count < type->size) {
        // 初始化个数小于当前维度的长度，补零
        count++;
        if (auto p5 = dynamic_cast<BasicType *>(type->base_type)) {
            if (type->base_type->is_float_type()) {
                arrayInit->add(new ValueInit(ConstantFloat::CONST_0F, BasicType::F32_TYPE));
            } else {
                arrayInit->add(new ValueInit(ConstantInt::CONST_0, BasicType::I32_TYPE));
            }
        } else {
            arrayInit->add(new ZeroInit(type->base_type));
        }
    }
    return arrayInit;
}

ValueInit *Visitor::visitInitVal(BasicType *basicType, AST::Exp *exp) {
    IFV *eval = Evaluate::evalConstExp(exp);
    if (int *int_ptr = std::get_if<int>(eval)) {
        if (basicType->is_int32_type()) {
            return new ValueInit(new ConstantInt(*int_ptr), BasicType::I32_TYPE);
        } else if (basicType->is_float_type()) {
            return new ValueInit(new ConstantFloat((float) *int_ptr), BasicType::F32_TYPE);
        } else {
            exit(18);
        }
    } else if (float *float_ptr = std::get_if<float>(eval)) {
        if (basicType->is_int32_type()) {
            return new ValueInit(new ConstantInt((int) *float_ptr), BasicType::I32_TYPE);
        } else if (basicType->is_float_type()) {
            return new ValueInit(new ConstantFloat(*float_ptr), BasicType::F32_TYPE);
        } else {
            exit(19);
        }
    } else {
        exit(28);
    }
}

ExpInit *Visitor::visitInitExp(BasicType *basicType, AST::Exp *exp) {
    Value *eval = visitExp(exp); // 运行期才计算
    return new ExpInit(eval, basicType);
}

void Visitor::visitFuncDef(AST::FuncDef *def) {
    curLoop = new Loop(Loop::emptyLoop);
    Token::TokenType funcTypeTk = def->type->token_type;
    Type *retType;
    if (funcTypeTk == Token::VOID) {
        retType = VoidType::VOID_TYPE;
    } else if (funcTypeTk == Token::INT) {
        retType = BasicType::I32_TYPE;
    } else if (funcTypeTk == Token::FLOAT) {
        retType = BasicType::F32_TYPE;
    } else {
        exit(17);
    }
    std::string ident = def->ident->content;
    isGlobal = false;
    // 入口基本块
    BasicBlock *entry = new BasicBlock();
    curBB = entry;
    curLoop->setHeader(entry);
    // 构造形参层符号表
    currentSymTable = new SymTable(currentSymTable);
    // 形参表
    std::vector<Function::Param *> *params = new std::vector<Function::Param *>();
    int idx = 0;
    for (AST::FuncFParam *fParam: def->fParams) {
        Type *paramType = visitFuncFParam(fParam);
        Function::Param *param = new Function::Param(paramType, idx++);
        params->push_back(param); // 形参变量
        Value *paramPointer = new Alloc(paramType, curBB);
        new Store(param, paramPointer, curBB);
        currentSymTable->add(new Symbol(fParam->ident->content, paramType, nullptr, false, paramPointer));
    }
    Function *function = new Function(ident, params, retType);
    curLoop->setFunc(function);
    manager->addFunction(function);
    function->entry = entry;
    curFunc = function;
    entry->setFunction(curFunc, curLoop);
    visitBlock(def->body, false); // 分析函数体
    if (!curBB->isTerminated()) {
        // 如果没有 return 补上一条
        if (funcTypeTk == Token::VOID) { new Return(curBB); }
        else if (funcTypeTk == Token::INT)new Return(ConstantInt::CONST_0, curBB);
        else if (funcTypeTk == Token::FLOAT)new Return(ConstantFloat::CONST_0F, curBB);
        else {
            exit(16);
        }
    }
    currentSymTable = currentSymTable->parent;
    curBB = nullptr;
    curFunc = nullptr;
    isGlobal = true;
}

Type *Visitor::getBasicType(AST::FuncFParam *funcFParam) {
    if (funcFParam->bType->token_type == Token::INT) {
        return BasicType::I32_TYPE;
    } else {
        return BasicType::F32_TYPE;
    }
}

Type *Visitor::visitFuncFParam(AST::FuncFParam *funcFParam) {
    // 常数, 常数指针或者数组指针(注意降维)
    if (!funcFParam->array) {
        return getBasicType(funcFParam);
    } else {
        std::vector<int> lengths;
        for (AST::Exp *index: funcFParam->sizes) {
            int len = Evaluate::evalConstIntExp(index);
            lengths.push_back(len);
        }
        Type *paramType = getBasicType(funcFParam);
        for (int i = lengths.size() - 1; i >= 0; i--) {
            int len = lengths[i];
            paramType = new ArrayType(len, paramType);
        }
        return new PointerType(paramType);
    }
}

void Visitor::visitCompUnit(AST::CompUnit *unit) {
    // assert isGlobal();
    if (AST::Decl *p1 = dynamic_cast<AST::Decl *>(unit)) {
        visitDecl(p1); // 全局变量
    } else if (AST::FuncDef *p2 = dynamic_cast<AST::FuncDef *>(unit)) {
        visitFuncDef(p2);
    } else {
        exit(6);
    }
}