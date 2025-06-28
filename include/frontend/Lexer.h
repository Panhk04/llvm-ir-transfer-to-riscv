//
// Created by XuRuiyuan on 2023/7/18.
//

#ifndef FASTER_MEOW_LEXER_H
#define FASTER_MEOW_LEXER_H

#include "string"
#include "Token.h"
#include <regex>
#include "../util/File.h"
#include "iostream"

class Lexer {
public:
    Lexer() = default;

    TokenList tokenList = TokenList();

    void keywordTokenDeal(const std::string &str) {
        if (str == "const") {
            Token *tk = new Token(Token::TokenType::CONST, str);
            tokenList.append(tk);
        } else if (str == "int") {
            Token *tk = new Token(Token::TokenType::INT, str);
            tokenList.append(tk);
        } else if (str == "float") {
            detect_float = true;
            Token *tk = new Token(Token::TokenType::FLOAT, str);
            tokenList.append(tk);
        } else if (str == "break") {
            Token *tk = new Token(Token::TokenType::BREAK, str);
            tokenList.append(tk);
        } else if (str == "continue") {
            Token *tk = new Token(Token::TokenType::CONTINUE, str);
            tokenList.append(tk);
        } else if (str == "if") {
            Token *tk = new Token(Token::TokenType::IF, str);
            tokenList.append(tk);
        } else if (str == "else") {
            Token *tk = new Token(Token::TokenType::ELSE, str);
            tokenList.append(tk);
        } else if (str == "while") {
            Token *tk = new Token(Token::TokenType::WHILE, str);
            tokenList.append(tk);
        } else if (str == "return") {
            Token *tk = new Token(Token::TokenType::RETURN, str);
            tokenList.append(tk);
        } else if (str == "void") {
            Token *tk = new Token(Token::TokenType::VOID, str);
            tokenList.append(tk);
        } else {
            Token *tk = new Token(Token::TokenType::IDENT, str);
            tokenList.append(tk);
        }
    }

    bool dealAsicii(int c) {
        bool flag = (c == 32 || c == 33 || (c <= 126 && c >= 40));
        if (c == 34) return false;
        if (c == (int) '%') return true;
        return flag;
    }

    bool isNewline(char c) {
        if (c == '\n') {
            return true;
        } else return c == '\r';
    }

    bool isDigital(char c) {
        return (c <= '9' && c >= '0') || c == '.';
    }

    bool isLetter(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    }

    int index = 0;
    char last_char = 0;
    std::set<char> special_set;

    int my_get_c() {
        // if(index == 15) {
        //     int j = 1;
        // }
        // printf("%d:%c\n",index, FileDealer::fileStr[index]);
        return (int) FileDealer::fileStr[index++];
    }

    void init() {
        special_set.insert('x');
        special_set.insert('X');
        special_set.insert('p');
        special_set.insert('P');
        special_set.insert('e');
        special_set.insert('E');
    }

    static bool detect_float;

    void lex() {
        int constChar[128] = {0};
        int floatChar[128] = {0};
        for (int c = '0'; c <= '9'; c++) {
            constChar[c] = 1;
        }
        constChar['x'] = 1;
        constChar['X'] = 1;
        constChar['p'] = 1;
        constChar['P'] = 1;
        constChar['e'] = 1;
        constChar['E'] = 1;
        constChar['.'] = 1;
        for (int c = 'a'; c <= 'f'; c++) {
            constChar[c] = 1;
        }
        for (int c = 'A'; c <= 'F'; c++) {
            constChar[c] = 1;
        }
        floatChar['x'] = 1;
        floatChar['X'] = 1;
        floatChar['p'] = 1;
        floatChar['P'] = 1;
        floatChar['e'] = 1;
        floatChar['E'] = 1;
        Token::TokenType NUM_CON_LIST[] = {Token::TokenType::HEX_INT,
                                           Token::TokenType::DEC_INT,
                                           Token::TokenType::OCT_INT,
                                           Token::TokenType::HEX_FLOAT,
                                           Token::TokenType::DEC_FLOAT};
        std::regex r0("0(x|X)[0-9A-Fa-f]+");
        std::regex r1("0|([1-9][0-9]*)");
        std::regex r2("0[0-7]+");
        std::regex r3("(0(x|X)[0-9A-Fa-f]*\\.[0-9A-Fa-f]*((p|P|e|E)(\\+|\\-)?[0-9A-Fa-f]*)?)|(0(x|X)[0-9A-Fa-f]*[\\.]?[0-9A-Fa-f]*(p|P|e|E)((\\+|\\-)?[0-9A-Fa-f]*)?)");
        std::regex r4("([0-9]*\\.[0-9]*((p|P|e|E)(\\+|\\-)?[0-9]+)?)|([0-9]*[\\.]?[0-9]*(p|P|e|E)((\\+|\\-)?[0-9]+)?)");
        std::regex regex_list[] = {
                r0, r1, r2, r3, r4
        };
        int c = my_get_c();
        while (c != 0) {
            std::string stringBuilder;
            last_char = (char) c;
            if (last_char == '/') {
                c = my_get_c();
                if (c == -1) {
                    break;
                }
                if (c == (int) '/') {
                    c = my_get_c();
                    while (c != 0) {
                        if (isNewline((char) c)) {
                            c = my_get_c();
                            break;
                        }
                        c = my_get_c();
                    }
                } else if (c == (int) '*') {
                    c = my_get_c();
                    while (c != 0) {
                        if (c == (int) '*') {
                            c = my_get_c();
                            if (c == (int) '/') {
                                c = my_get_c();
                                break;
                            } else {
                                continue;
                            }
                        } else if (c == (int) '\n') {
                        }
                        c = my_get_c();
                    }
                } else {
                    Token *tk = new Token(Token::TokenType::DIV, "/");
                    tokenList.append(tk);
                }
                if (c == -1) {
                    break;
                }
                last_char = (char) c;
            }
            while (last_char == ' ' || isNewline(last_char) || last_char == '\t') {
                c = my_get_c();
                if (c == -1) {
                    break;
                }
                last_char = (char) c;
            }
            if (c == -1) {
                break;
            }
            if (isLetter(last_char)) {
                // String str = "";
                stringBuilder += last_char;
                c = my_get_c();
                if (c == -1) {
                    keywordTokenDeal(stringBuilder);
                    break;
                }
                last_char = (char) c;
                while (isLetter(last_char) || isDigital(last_char)) {
                    stringBuilder += last_char;
                    c = my_get_c();
                    if (c == -1) {
                        break;
                    }
                    last_char = (char) c;
                }
                keywordTokenDeal(stringBuilder);
                if (c == -1) {
                    break;
                }
            } else if (isDigital(last_char)) {
                std::string numStr;
                bool flag = false;
                do {
                    numStr += last_char;
                    c = my_get_c();
                    if (c == -1) {
                        break;
                    }
                    last_char = (char) c;
                    if (flag) {
                        if (last_char == '+' || last_char == '-') {
                            c = my_get_c();
                            if (c == -1) {
                                break;
                            }
                            numStr += last_char;
                            last_char = (char) c;
                        }
                    }
                    flag = floatChar[c];
                } while (constChar[c]);
                bool success = false;
                for (int i = 0; i < 5; i++) {
                    Token::TokenType type = NUM_CON_LIST[i];
                    if (std::regex_match(numStr, regex_list[i])) {
                        if (type == Token::TokenType::HEX_FLOAT || type == Token::TokenType::DEC_FLOAT) {
                            detect_float = true;
                        }
                        // String token = matcher.group(0);
                        Token *tk = new Token(type, numStr);
                        tokenList.append(tk);
                        success = true;
                        break;
                    }
                }
                if (!success) {
                    exit(2);
                }
                if (c == -1) {
                    break;
                }
            } else if (last_char == '\"') {
                stringBuilder + ('\"');
                c = my_get_c();
                if (c == -1) {
                    break;
                }
                last_char = (char) c;
                if (last_char == '\"') {
                    exit(3);
                    // lexerErrorAdd(lexer'a');
                    Token *tk = new Token(Token::TokenType::STR_CON, "");
                    tokenList.append(tk);
                    c = my_get_c();
                    if (c == -1) {
                        break;
                    }
                    last_char = (char) c;
                    continue;
                }
                while (true) {
                    dealAsicii(last_char);
                    if (last_char == '\\') {
                        stringBuilder + "\\";
                    } else {
                        stringBuilder += last_char;
                    }
                    c = my_get_c();
                    if (c == -1) {
                        break;
                    }
                    last_char = (char) c;
                    if (last_char == '\"') {
                        break;
                    }
                }
                if (c == -1) {
                    break;
                }
                stringBuilder + '\"';
                Token *tk = new Token(Token::TokenType::STR_CON, stringBuilder);
                tokenList.append(tk);
                c = my_get_c();
                if (c == -1) {
                    break;
                }
                last_char = (char) c;
            } else if (last_char == '=') {
                c = my_get_c();
                if (c == -1) {
                    Token *tk = new Token(Token::TokenType::ASSIGN, "=");
                    tokenList.append(tk);
                    break;
                }
                last_char = (char) c;
                if (last_char == '=') {
                    Token *tk = new Token(Token::TokenType::EQ, "==");
                    tokenList.append(tk);
                    c = my_get_c();
                    if (c == -1) {
                        break;
                    }
                    last_char = (char) c;
                } else {
                    Token *tk = new Token(Token::TokenType::ASSIGN, "=");
                    tokenList.append(tk);
                }
            } else if (last_char == '!') {
                c = my_get_c();
                if (c == -1) {
                    Token *tk = new Token(Token::TokenType::NOT, "!");
                    tokenList.append(tk);
                    break;
                }
                last_char = (char) c;
                if (last_char == '=') {
                    Token *tk = new Token(Token::TokenType::NE, "!=");
                    tokenList.append(tk);
                    c = my_get_c();
                    if (c == -1) {
                        break;
                    }
                    last_char = (char) c;
                } else {
                    Token *tk = new Token(Token::TokenType::NOT, "!");
                    tokenList.append(tk);
                }
            } else if (last_char == '>') {
                c = my_get_c();
                if (c == -1) {
                    Token *tk = new Token(Token::TokenType::GT, ">");
                    tokenList.append(tk);
                    break;
                }
                last_char = (char) c;
                if (last_char == '=') {
                    Token *tk = new Token(Token::TokenType::GE, ">=");
                    tokenList.append(tk);
                    c = my_get_c();
                    if (c == -1) {
                        break;
                    }
                    last_char = (char) c;
                } else {
                    Token *tk = new Token(Token::TokenType::GT, ">");
                    tokenList.append(tk);
                }
            } else if (last_char == '<') {
                c = my_get_c();
                if (c == -1) {
                    Token *tk = new Token(Token::TokenType::LT, "<");
                    tokenList.append(tk);
                    break;
                }
                last_char = (char) c;
                if (last_char == '=') {
                    Token *tk = new Token(Token::TokenType::LE, "<=");
                    tokenList.append(tk);
                    c = my_get_c();
                    if (c == -1) {
                        break;
                    }
                    last_char = (char) c;
                } else {
                    Token *tk = new Token(Token::TokenType::LT, "<");
                    tokenList.append(tk);
                }
            } else if (last_char == '&') {
                c = my_get_c();
                if (c == -1) {
                    break;
                }
                last_char = (char) c;
                if (last_char == '&') {
                    Token *tk = new Token(Token::TokenType::LAND, "&&");
                    tokenList.append(tk);
                    c = my_get_c();
                    if (c == -1) {
                        break;
                    }
                    last_char = (char) c;
                }
            } else if (last_char == '|') {
                c = my_get_c();
                if (c == -1) {
                    break;
                }
                last_char = (char) c;
                if (last_char == '|') {
                    Token *tk = new Token(Token::TokenType::LOR, "||");
                    tokenList.append(tk);
                    c = my_get_c();
                    if (c == -1) {
                        break;
                    }
                    last_char = (char) c;
                }
            } else {
                if (last_char == '+') {
                    Token *tk = new Token(Token::TokenType::ADD, "+");
                    tokenList.append(tk);
                    c = my_get_c();
                } else if (last_char == '-') {
                    Token *tk = new Token(Token::TokenType::SUB, "-");
                    tokenList.append(tk);
                    c = my_get_c();
                } else if (last_char == '*') {
                    Token *tk = new Token(Token::TokenType::MUL, "*");
                    tokenList.append(tk);
                    c = my_get_c();
                } else if (last_char == '%') {
                    Token *tk = new Token(Token::TokenType::MOD, "%");
                    tokenList.append(tk);
                    c = my_get_c();
                } else if (last_char == ';') {
                    Token *tk = new Token(Token::TokenType::SEMI, ";");
                    tokenList.append(tk);
                    c = my_get_c();
                } else if (last_char == ',') {
                    Token *tk = new Token(Token::TokenType::COMMA, ",");
                    tokenList.append(tk);
                    c = my_get_c();
                } else if (last_char == '(') {
                    Token *tk = new Token(Token::TokenType::LPARENT, "(");
                    tokenList.append(tk);
                    c = my_get_c();
                } else if (last_char == ')') {
                    Token *tk = new Token(Token::TokenType::RPARENT, ")");
                    tokenList.append(tk);
                    c = my_get_c();
                } else if (last_char == '[') {
                    Token *tk = new Token(Token::TokenType::LBRACK, "[");
                    tokenList.append(tk);
                    c = my_get_c();
                } else if (last_char == ']') {
                    Token *tk = new Token(Token::TokenType::RBRACK, "]");
                    tokenList.append(tk);
                    c = my_get_c();
                } else if (last_char == '{') {
                    Token *tk = new Token(Token::TokenType::LBRACE, "{");
                    tokenList.append(tk);
                    c = my_get_c();
                } else if (last_char == '}') {
                    Token *tk = new Token(Token::TokenType::RBRACE, "}");
                    tokenList.append(tk);
                    c = my_get_c();
                } else if (c == -1) {
                    break;
                }
                last_char = (char) c;
            }

        }
    }
};


#endif //FASTER_MEOW_LEXER_H
