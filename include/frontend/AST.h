//
// Created by Administrator on 2023/7/13.
//

#ifndef FASTER_MEOW_AST_H
#define FASTER_MEOW_AST_H

#include "vector"
#include "Token.h"

#define __SS__(T) public: T() = default; ~T() override = default;
namespace AST {
    // class Decl;

    class Def;

    class CompUnit;

    class Init {
    public:
        Init() = default;

        virtual ~Init() = default;
    };

    class InitArray;

    class FuncDef;


    class Block;

    class BlockItem {
    public:
        virtual ~BlockItem() = default;
    };

    class Stmt : public BlockItem {
    public:
        Stmt() = default;

        ~Stmt() override = default;
    };
    //
    // class Assign;
    //
    // class ExpStmt;
    //
    // class IfStmt;
    //
    // class WhileStmt;
    //
    // class Continue;
    //
    // class Return;

    class PrimaryExp {
    public:
        PrimaryExp() = default;

        virtual ~PrimaryExp() = default;
    };

    class Exp : public Init, public PrimaryExp {
    public:
        Exp() = default;

        ~Exp() override = default;
    };

    // class BinaryExp;
    //
    // class UnaryExp;
    //
    //
    // class LVal;
    //
    // class Number;
    //
    // class Call;

    class Ast {
    public:
        std::vector<CompUnit *> units;

        Ast() = default;

        virtual ~Ast() = default;
    };

    class CompUnit {
    public:
        CompUnit() = default;

        virtual ~CompUnit() = default;
    };

    class Decl : public CompUnit, public BlockItem {
    public:
        bool constant;
        Token *bType;
        std::vector<Def *> defs;

        Decl() = default;
        // Decl(bool constant, Token *bType) : constant(constant), bType(bType) {}

        ~Decl() override = default;
    };

    class Def {
    public:
        Token::TokenType bType;
        Token *ident;
        std::vector<Exp *> indexes;
        Init *init;

        Def() = default;

        ~Def() = default;
    };

    class InitArray : public Init {
    public:
        std::vector<Init *> init;
        int nowIdx = 0;

        ~InitArray() override = default;

        Init *getNowInit() {
            return this->init[nowIdx];
        }
    };

    class FuncFParam;

    class FuncDef : public CompUnit {
    public:
        Token *type;
        Token *ident;
        std::vector<FuncFParam *> fParams;
        Block *body;

        FuncDef() = default;

        ~FuncDef() override = default;
    };

    class FuncFParam {
    public:
        Token *bType;
        Token *ident;
        bool array;
        std::vector<Exp *> sizes;
    };

    class Block : public Stmt {
    public:
        Block() = default;

        ~Block() override = default;

        std::vector<BlockItem *> items;
    };

    class LVal;

    class Assign : public Stmt {
    public:
        LVal *left;
        Exp *right;
    };

    class ExpStmt : public Stmt {
    public:
        Exp *exp;

        ExpStmt() = default;

        ~ExpStmt() override = default;
    };

    class IfStmt : public Stmt {
    public:
        Exp *cond;
        Stmt *thenTarget;
        Stmt *elseTarget;

        IfStmt() = default;

        ~IfStmt() override = default;
    };

    class WhileStmt : public Stmt {
    public:
        Exp *cond;
        Stmt *body;

        WhileStmt() = default;

        ~WhileStmt() override = default;
    };

    class Break : public Stmt {
    __SS__(Break)
    };

    class Continue : public Stmt {
    __SS__(Continue)
    };

    class Return : public Stmt {
    __SS__(Return)

        Exp *value;
    };

    class BinaryExp : public Exp {
    __SS__(BinaryExp)

        Exp *first;
        std::vector<Token *> operators;
        std::vector<Exp *> follows;
    };

    class UnaryExp : public Exp {
    __SS__(UnaryExp)

        std::vector<Token *> unaryOps;
        PrimaryExp *primaryExp;
    };

    class LVal : public PrimaryExp {
    __SS__(LVal)

        Token *ident;
        std::vector<Exp *> indexes;
    };

    class Number : public PrimaryExp {
    public:
        Number(Token *number) {
            this->number = number;

            if (number->is_int_const()) {
                isIntConst = true;

                if (number->token_type == Token::HEX_INT)
                    intConstVal = std::stoi(number->content.substr(2), nullptr, 16);
                else if (number->token_type == Token::OCT_INT)
                    intConstVal = std::stoi(number->content.substr(1), nullptr, 8);
                else if (number->token_type == Token::DEC_INT)
                    intConstVal = std::stoi(number->content);
                // intConstVal = std::stoi(number->content);
                floatConstVal = (float) intConstVal;
            } else if (number->is_float_const()) {
                isFloatConst = true;
                // todo : may bug !!!
                floatConstVal = std::stof(number->content);
                intConstVal = (int) floatConstVal;
            } else {
                exit(5);
            }
        }

        ~Number() override = default;

        Token *number;
        bool isIntConst = false;
        bool isFloatConst = false;
        int intConstVal = 0;
        float floatConstVal = 0.0f;

        friend std::ostream &operator<<(std::ostream &stream, const Number *number1) {

            if (number1->isIntConst) {
                stream << "int " << number1->intConstVal;
            } else if (number1->isFloatConst) {
                stream << "float" << number1->floatConstVal;
            } else {
                stream << "???" << number1->number;
            }
            return stream;
        }
    };

    class Call : public PrimaryExp {
    __SS__(Call)

        Token *ident;
        std::vector<Exp *> params;
        int lineno = 0;
    };
}

#endif //FASTER_MEOW_AST_H
