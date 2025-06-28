//
// Created by XuRuiyuan on 2023/7/18.
//

#ifndef FASTER_MEOW_PARSER_H
#define FASTER_MEOW_PARSER_H

#include "../../include/frontend/Token.h"
#include "AST.h"
#include "map"
// #define ER 30
class Parser {
public:
    TokenList &tokenList;

    void out_tokens() {
        for (const Token *t: tokenList.tokens) {
            std::cout << t->content << " ";
        }
        std::cout << std::endl;
    }

    explicit Parser(TokenList &tokenList) : tokenList(tokenList) {}

    AST::Ast *parseAst() {
        auto ast = new AST::Ast();
        while (tokenList.has_next()) {
            if (tokenList.ahead(2)->is_of(Token::LPARENT)) {
                ast->units.push_back(parseFuncDef());
            } else {
                ast->units.push_back(parseDecl());
            }
        }
        return ast;
    }

    AST::Decl *parseDecl() {
        AST::Decl *decl = new AST::Decl();
        bool constant;
        if (tokenList.get()->is_of(Token::CONST)) {
            tokenList.consume();
            constant = true;
        } else { constant = false; }
        // todo
        Token *bType = tokenList.consume();
        decl->defs.push_back(parseDef(bType, constant));
        while (tokenList.get()->is_of(Token::COMMA)) {
            tokenList.consume();
            decl->defs.push_back(parseDef(bType, constant));
        }
        tokenList.consume_expect(Token::SEMI);
        decl->bType = bType;
        decl->constant = constant;
        return decl;
    }

    AST::Def *parseDef(Token *bType, bool constant) {
        AST::Def *def = new AST::Def();
        def->bType = bType->token_type;
        def->ident = tokenList.consume_expect(Token::IDENT);
        while (tokenList.get()->is_of(Token::LBRACK)) {
            tokenList.consume();
            def->indexes.push_back(parseAddExp());
            tokenList.consume_expect(Token::RBRACK);
        }
        if (constant) {
            tokenList.consume_expect(Token::ASSIGN);
            def->init = parseInitVal();
        } else {
            if (tokenList.has_next() && tokenList.get()->is_of(Token::ASSIGN)) {
                tokenList.consume();
                def->init = parseInitVal();
            }
        }
        return def;
    }

    AST::Init *parseInitVal() {
        if (tokenList.get()->is_of(Token::LBRACE)) {
            return dynamic_cast<AST::Init *>(parseInitArray());
        } else {
            return dynamic_cast<AST::Init *>(parseAddExp());
        }
    }

    AST::InitArray *parseInitArray() {
        // std::vector<AST::Init> init = new std::vector<>();
        AST::InitArray *initArray = new AST::InitArray;
        tokenList.consume_expect(Token::LBRACE);
        if (!tokenList.get()->is_of(Token::RBRACE)) {
            initArray->init.push_back(parseInitVal());
            while (tokenList.get()->is_of(Token::COMMA)) {
                tokenList.consume();
                initArray->init.push_back(parseInitVal());
            }
        }
        tokenList.consume_expect(Token::RBRACE);
        return initArray;
    }

    AST::FuncDef *parseFuncDef() {
        // todo
        auto *funcDef = new AST::FuncDef;
        funcDef->type = tokenList.consume();
        funcDef->ident = tokenList.consume_expect(Token::IDENT);
        // std::vector<AST::FuncFParam> fParams = new std::vector<>();
        tokenList.consume_expect(Token::LPARENT);
        if (!tokenList.get()->is_of(Token::RPARENT)) {
            funcDef->fParams = *parseFuncFParams();
        }
        tokenList.consume_expect(Token::RPARENT);
        funcDef->body = parseBlock();
        return funcDef;
    }

    std::vector<AST::FuncFParam *> *parseFuncFParams() {
        std::vector<AST::FuncFParam *> *fParams = new std::vector<AST::FuncFParam *>;
        // AST::FuncFParam *p = ;
        fParams->push_back(parseFuncFParam());
        while (tokenList.has_next() && tokenList.get()->is_of(Token::COMMA)) {
            tokenList.consume_expect(Token::COMMA);
            fParams->push_back(parseFuncFParam());
        }
        return fParams;
    }

    AST::FuncFParam *parseFuncFParam() {
        AST::FuncFParam *funcFParam = new AST::FuncFParam();
        funcFParam->bType = tokenList.consume();
        funcFParam->ident = tokenList.consume_expect(Token::IDENT);
        funcFParam->array = false;
        // std::vector<AST::Exp> sizes = new std::vector<>();
        if (tokenList.has_next() && tokenList.get()->is_of(Token::LBRACK)) {
            funcFParam->array = true;
            tokenList.consume_expect(Token::LBRACK);
            tokenList.consume_expect(Token::RBRACK);
            while (tokenList.has_next() && tokenList.get()->is_of(Token::LBRACK)) {
                tokenList.consume_expect(Token::LBRACK);
                funcFParam->sizes.push_back(parseAddExp());
                tokenList.consume_expect(Token::RBRACK);
            }
        }
        return funcFParam;
    }

    AST::Block *parseBlock() {
        AST::Block *block = new AST::Block;
        // std::vector<AST::BlockItem> items = new std::vector<>();
        tokenList.consume_expect(Token::LBRACE);
        while (!tokenList.get()->is_of(Token::RBRACE)) {
            block->items.push_back(parseBlockItem());
        }
        tokenList.consume_expect(Token::RBRACE);
        return block;
    }

    AST::BlockItem *parseBlockItem() {
        if (tokenList.get()->is_of(Token::FLOAT)
            || tokenList.get()->is_of(Token::INT)
            || tokenList.get()->is_of(Token::CONST)) {
            return parseDecl();
        } else {
            return parseStmt();
        }
    }

    AST::Stmt *parseStmt() {
        Token::TokenType temp = tokenList.get()->token_type;
        // AST::Exp *cond;
        switch (temp) {
            case Token::LBRACE: {
                return parseBlock();
            }
            case Token::IF: {
                AST::IfStmt *ifStmt = new AST::IfStmt;
                tokenList.consume();
                tokenList.consume_expect(Token::LPARENT);
                ifStmt->cond = parseCond();
                tokenList.consume_expect(Token::RPARENT);
                ifStmt->thenTarget = parseStmt();
                ifStmt->elseTarget = nullptr;
                if (tokenList.has_next() && tokenList.get()->is_of(Token::ELSE)) {
                    tokenList.consume_expect(Token::ELSE);
                    ifStmt->elseTarget = parseStmt();
                }
                return ifStmt;
            }
            case Token::WHILE: {
                auto whileStmt = new AST::WhileStmt;
                tokenList.consume();
                tokenList.consume_expect(Token::LPARENT);
                whileStmt->cond = parseCond();
                tokenList.consume_expect(Token::RPARENT);
                whileStmt->body = parseStmt();
                return whileStmt;
            }
            case Token::BREAK: {
                tokenList.consume_expect(Token::BREAK);
                tokenList.consume_expect(Token::SEMI);
                return new AST::Break();
            }
            case Token::CONTINUE: {
                tokenList.consume_expect(Token::CONTINUE);
                tokenList.consume_expect(Token::SEMI);
                return new AST::Continue();
            }
            case Token::RETURN: {
                AST::Return *ret = new AST::Return();
                tokenList.consume_expect(Token::RETURN);
                ret->value = nullptr;
                if (!tokenList.get()->is_of(Token::SEMI)) {
                    ret->value = parseAddExp();
                }
                tokenList.consume_expect(Token::SEMI);
                return ret;
            }
            case Token::IDENT: {
                // 先读出一整个 Exp 再判断是否只有一个 LVal (因为 LVal 可能是数组)
                AST::Exp *temp2 = parseAddExp();
                AST::LVal *left = extractLValFromExp(temp2);
                if (left == nullptr) {
                    AST::ExpStmt *expStmt = new AST::ExpStmt;
                    tokenList.consume_expect(Token::SEMI);
                    expStmt->exp = temp2;
                    return expStmt;
                } else {
                    // 只有一个 LVal，可能是 Exp; 也可能是 Assign
                    if (tokenList.get()->is_of(Token::ASSIGN)) {
                        AST::Assign *assign = new AST::Assign;
                        assign->left = left;
                        tokenList.consume_expect(Token::ASSIGN);
                        assign->right = parseAddExp();
                        tokenList.consume_expect(Token::SEMI);
                        return assign;
                    } else {
                        AST::ExpStmt *expStmt = new AST::ExpStmt;
                        expStmt->exp = temp2;
                        tokenList.consume_expect(Token::SEMI);
                        return expStmt;
                    }
                }
            }
            case Token::SEMI:
                tokenList.consume();
                return new AST::ExpStmt();
            default:
                AST::ExpStmt *expStmt = new AST::ExpStmt;
                expStmt->exp = parseAddExp();
                return expStmt;
        }
    }

    AST::LVal *parseLVal() {
        AST::LVal *lVal = new AST::LVal;
        lVal->ident = tokenList.consume_expect(Token::IDENT);
        // std::vector<AST::Exp> indexes = new std::vector<>();
        while (tokenList.has_next() && tokenList.get()->is_of(Token::LBRACK)) {
            tokenList.consume_expect(Token::LBRACK);
            lVal->indexes.push_back(parseAddExp());
            tokenList.consume_expect(Token::RBRACK);
        }
        return lVal;
    }

    AST::PrimaryExp *parsePrimary() {
        Token *temp = tokenList.get();
        if (temp->is_of(Token::LPARENT)) {
            tokenList.consume();
            AST::Exp *exp = parseAddExp();
            tokenList.consume_expect(Token::RPARENT);
            return exp;
        } else if (temp->is_num_const()) {
            Token *number = tokenList.consume();
            return new AST::Number(number);
        } else if (temp->is_of(Token::IDENT)
                   && tokenList.ahead(1)->is_of(Token::LPARENT)) {
            return parseCall();
        } else {
            return parseLVal();
        }
    }

    AST::Call *parseCall() {
        AST::Call *call = new AST::Call;
        call->ident = tokenList.consume_expect(Token::IDENT);
        // std::vector<AST::Exp> params = new std::vector<>();
        tokenList.consume_expect(Token::LPARENT);
        if (!tokenList.get()->is_of(Token::RPARENT)) {
            call->params.push_back(parseAddExp());
            while (tokenList.get()->is_of(Token::COMMA)) {
                tokenList.consume();
                call->params.push_back(parseAddExp());
            }
        }
        tokenList.consume_expect(Token::RPARENT);
        return call;
    }

    // 二元表达式的种类
    enum BinaryExpType {
        LOR,
        LAND,
        EQ,
        REL,
        ADD,
        MUL,
    };

    // 解析二元表达式的下一层表达式
    AST::Exp *parseSubBinaryExp(BinaryExpType expType) {

        if (expType == LOR) {
            return parseBinaryExp(BinaryExpType::LAND);
        } else if (expType == LAND) {
            return parseBinaryExp(BinaryExpType::EQ);
        } else if (expType == EQ) {
            return parseBinaryExp(BinaryExpType::REL);
        } else if (expType == REL) {
            return parseBinaryExp(BinaryExpType::ADD);
        } else if (expType == ADD) {
            return parseBinaryExp(BinaryExpType::MUL);
        } else if (expType == MUL) { return parseUnaryExp(); }
        else {
            exit(30);
        }
    }

    static bool contains(BinaryExpType binaryExpType, Token::TokenType tokenType) {
        if (binaryExpType == LOR) {
            return tokenType == Token::LOR;
        } else if (binaryExpType == LAND) {
            return tokenType == Token::LAND;
        } else if (binaryExpType == EQ) {
            return tokenType == Token::EQ || tokenType == Token::NE;
        } else if (binaryExpType == REL) {
            return tokenType == Token::GT || tokenType == Token::LT || tokenType == Token::GE || tokenType == Token::LE;
        } else if (binaryExpType == ADD) {
            return tokenType == Token::ADD || tokenType == Token::SUB;
        } else if (binaryExpType == MUL) {
            return tokenType == Token::MUL || tokenType == Token::DIV || tokenType == Token::MOD;
        } else {
            exit(31);
        }
    }

    // 解析二元表达式
    AST::BinaryExp *parseBinaryExp(BinaryExpType expType) {
        AST::BinaryExp *binaryExp = new AST::BinaryExp;
        binaryExp->first = parseSubBinaryExp(expType);
        while (tokenList.has_next() && contains(expType, tokenList.get()->token_type)) {
            Token *op = tokenList.consume(); // 取得当前层次的运算符
            binaryExp->operators.push_back(op);
            binaryExp->follows.push_back(parseSubBinaryExp(expType));
        }
        return binaryExp;
    }

    AST::UnaryExp *parseUnaryExp() {
        AST::UnaryExp *unaryExp = new AST::UnaryExp;
        // std::vector<Token> unaryOps = new std::vector<>();
        while (tokenList.get()->is_of(Token::ADD)
               || tokenList.get()->is_of(Token::SUB)
               || tokenList.get()->is_of(Token::NOT)) {
            unaryExp->unaryOps.push_back(tokenList.consume());
        }
        unaryExp->primaryExp = parsePrimary();
        return unaryExp;
    }

    AST::BinaryExp *parseAddExp() {
        return parseBinaryExp(BinaryExpType::ADD);
    }

    AST::BinaryExp *parseCond() {
        return parseBinaryExp(BinaryExpType::LOR);
    }

    // 从 Exp 中提取一个 LVal (如果不是仅有一个 LVal) 则返回 nullptr
    static AST::LVal *extractLValFromExp(AST::Exp *exp) {
        AST::Exp *cur = exp;
        while ((dynamic_cast<AST::BinaryExp *>(cur))) {
            // 如果是二元表达式，只能有 first 否则一定不是一个 LVal
            if (!(((AST::BinaryExp *) cur)->follows.empty())) {
                return nullptr;
            }
            cur = ((AST::BinaryExp *) cur)->first;
        }
        if (!(((AST::UnaryExp *) cur)->unaryOps.empty())) {
            return nullptr; // 不能有一元运算符
        }
        AST::PrimaryExp *primary = ((AST::UnaryExp *) cur)->primaryExp;
        return dynamic_cast<AST::LVal *>(primary);
    }
};

#endif //FASTER_MEOW_PARSER_H
