//
// Created by XuRuiyuan on 2023/7/18.
//

#ifndef FASTER_MEOW_TOKEN_H
#define FASTER_MEOW_TOKEN_H

#include <utility>

#include "string"
#include "vector"
#include "iostream"

class Token {
public:
    enum TokenType {
        CONST = 0,
        INT,
        FLOAT,
        BREAK,
        CONTINUE,
        IF,
        ELSE,
        VOID,
        WHILE,
        RETURN,
        // ident
        IDENT,
        // float const
        HEX_FLOAT,
        DEC_FLOAT,
        // DEC_FLOAT, // decimal
        // int const
        HEX_INT,
        OCT_INT,
        DEC_INT, // decimal
        // operator 
        LAND,
        LOR,
        LE,
        GE,
        EQ,
        NE,
        // operator 
        LT,
        GT,
        ADD, // regex
        SUB,
        MUL,
        DIV,
        MOD,
        NOT,
        ASSIGN,
        SEMI,
        COMMA,
        LPARENT,
        RPARENT,
        LBRACK,
        RBRACK,
        LBRACE,
        RBRACE,
        STR_CON,
    };
public:
    TokenType token_type;
    std::string content;

    bool is_alu_type() {
        return token_type >= ADD && token_type <= MOD;
    }

    bool is_cmp_type() {
        return token_type >= LE && token_type <= GT;
    }

    Token(TokenType type, std::string cont) : token_type(type), content(cont) {
    }

    bool is_of(TokenType type) {
        return type == token_type;
    }

    bool is_int_const() {
        return token_type <= DEC_INT && token_type >= HEX_INT;
    }

    bool is_float_const() {
        return token_type <= DEC_FLOAT && token_type >= HEX_FLOAT;
    }

    bool is_num_const() const {
        return token_type <= DEC_INT && token_type >= HEX_FLOAT;
    }

    static std::string list[100];

    friend std::ostream &operator<<(std::ostream &ostream, const Token &token);
};

class TokenList {
public:
    std::vector<Token *> tokens;
    int index = 0;

    void append(Token *token) {
        tokens.push_back(token);
    }

    bool has_next() {
        return index < tokens.size();
    }

    Token *get() {
        return tokens[index];
    }

    Token *ahead(int cnt) {
        return tokens[index + cnt];
    }

    Token *consume() {
        return tokens[index++];
    }

    Token *consume_expect(Token::TokenType type) {
        Token *token = tokens[index];
        if (token->token_type == type) {
            index++;
            return token;
        }
        exit(1);
    }

    TokenList() = default;

};



#endif //FASTER_MEOW_TOKEN_H
