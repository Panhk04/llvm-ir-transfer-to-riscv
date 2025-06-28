//
// Created by Administrator on 2023/7/13.
//

#ifndef FASTER_MEOW_VISITOR_H
#define FASTER_MEOW_VISITOR_H

#include "../mir/Type.h"
#include <memory>
#include "stack"
#include "../mir/Value.h"
#include "Parser.h"
#include "../util/Manager.h"
#include "../mir/BasicBlock.h"
#include "Symbol.h"
#include "mir/Instr.h"
#include "mir/Constant.h"
#include "mir/Function.h"
#include "mir/Loop.h"
#include "frontend/Evaluate.h"
#include "mir/Instr.h"
// using namespace INSTR;
// #define __ONLY_PARSE_OUTSIDE_DIM
#define _initAll false

/**
 * 遍历语法树, 生成 IR 代码
 */
class Visitor {
public:
    static Visitor *VISITOR;
    Manager *manager = Manager::MANAGER; // 最终生成的 IR
    static SymTable *currentSymTable; // 当前符号表, 初始时是全局符号表
    // 用于记录loop的单cond中的累计第几个, 不可直接public获取, 很危险
    // 在循环相关的所有visitLOrExp和visitLAndExp之前++可保证递增且唯一
    // 上面这个想法是错的, visitLOrExp内进第一个visitLAndExp之前加加会导致第一个andCond有两批编号
    // 应该在每一次切换基本块时++
    int curLoopCondCount = 0;

    // 实时更新表示Instr是否在循环中
    bool inLoop = false;

    // 实时更新表示Instr是否在Cond中
    bool inCond = false;

    int getCondCount() {
        return inCond ? curLoopCondCount : -1;
    }

    bool isInLoopCond() {
        return inLoop && inCond;
    }

    Function *curFunc = nullptr; // 当前正在分析的函数
    BasicBlock *curBB = nullptr; // 当前所在的基本块
    bool isGlobal = true;
    static Loop *curLoop;

    static int loopDepth;

    // bool isGlobal() {
    //     return curFunc == nullptr && curBB == nullptr && !currentSymTable->has_parent();
    // }

    std::stack<BasicBlock *> loopHeads; // for continue;
    std::stack<BasicBlock *> loopFollows; // for break;

    Visitor() = default;

    Value *trimTo(Value *value, BasicType *targetType) const;

    bool isAlu(Token *token) {
        return token->is_alu_type();
    }

    bool isCmp(Token *token) {
        return token->is_cmp_type();
    }

    bool isLogical(Token *token) {
        return token->token_type == Token::LAND || token->token_type == Token::LOR;
    }

    INSTR::Alu::Op aluOpHelper(Token *token);

    INSTR::Alu::Op aluOpHelper(Token *token, bool needFloat);

    INSTR::Icmp::Op icmpOpHelper(Token *token);

    INSTR::Fcmp::Op fcmpOpHelper(Token *token);

    void visitAst(AST::Ast *ast);

    bool hasFloatType(Value *v1, Value *v2) {
        return v1->type->is_float_type() || v2->type->is_float_type();
    }

    bool hasIntType(Value *v1, Value *v2) {
        return v1->type->is_int32_type() || v2->type->is_int32_type();
    }

    Value *visitBinaryExp(AST::BinaryExp *exp);

    Value *visitUnaryExp(AST::UnaryExp *exp);

    Value *visitExp(AST::Exp *exp);

    Value *visitNumber(AST::Number *number);

    Value *visitPrimaryExp(AST::PrimaryExp *exp);

// if left: return address, else return value (generate load instruction)
    Value *visitLVal(AST::LVal *lVal, bool needPointer);

// returns the return value if parentFunc call, nullptr if parentFunc is void
    Value *visitCall(AST::Call *call);

    void visitAssign(AST::Assign *assign);

    void visitExpStmt(AST::ExpStmt *expStmt);

    void dealCondCount() {
        if (inCond) {
            curLoopCondCount++;
        }
    }

/***
 * 出来的时候保证curBB为最后一个条件所在的块，不用Jump
 * @param exp
 * @param falseBlock
 * @return 保证返回Int1Type
 * @
 */
    Value *visitCondLAnd(AST::Exp *exp, BasicBlock *falseBlock);

/***
 * 出来的时候保证curBB为最后一个条件所在的块，不用Jump
 * @param exp
 * @param trueBlock
 * @param falseBlock
 * @return 保证返回Int1Type
 * @
 */
    Value *visitCondLOr(AST::Exp *exp, BasicBlock *trueBlock, BasicBlock *falseBlock);

    void visitIfStmt(AST::IfStmt *ifStmt);

    void visitWhileStmt(AST::WhileStmt *whileStmt);

    void visitBreak();

    void visitContinue();

    void visitReturn(AST::Return *ret);

    void visitStmt(AST::Stmt *stmt);

    void visitBlockItem(AST::BlockItem *item);

// sym: 是否需要新开一层符号表
    void visitBlock(AST::Block *block, bool sym);

    void visitDecl(AST::Decl *decl);

// static  std::vector<Value> const0Pair = new std::vector<>(Collections.nCopies(2, ConstantInt::CONST_0));

    void initZeroHelper(Value *pointer, Type *pointeeType);

    void initLocalVarHelper(Value *pointer, Initial *init);

    void visitDef(AST::Def *def, bool constant);

    ArrayInit *visitInitArray(AST::InitArray *initial, ArrayType *type, bool constant, bool eval);

    ValueInit *visitInitVal(BasicType *basicType, AST::Exp *exp);


    ExpInit *visitInitExp(BasicType *basicType, AST::Exp *exp);

// 由于编译器采用 Load-Store 形式，变量符号全部对应指针，所以在函数入口一开始先把形参全存到栈上 hhh
    void visitFuncDef(AST::FuncDef *def);

    Type *getBasicType(AST::FuncFParam *funcFParam);

    Type *visitFuncFParam(AST::FuncFParam *funcFParam);

    void visitCompUnit(AST::CompUnit *unit);

};


#endif //FASTER_MEOW_VISITOR_H
