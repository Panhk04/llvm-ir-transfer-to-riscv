//
// Created by XuRuiyuan on 2023/7/18.
//
#include "../../include/frontend/Token.h"
#include "iostream"

std::ostream &operator<<(std::ostream &ostream, const Token &token) {
    ostream << "<" << Token::list[token.token_type] << " " << token.content << ">";
    return ostream;
}
