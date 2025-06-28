#include <iostream>
#include "../include/util/IList.h"
#include "frontend/Lexer.h"
#include "frontend/Parser.h"
#include "../include/frontend/Visitor.h"
#include <fstream>
#include "sstream"
#include "iostream"
#include "Init.h"
#include "FrontendInit.h"
#include <chrono>
#include <string>
#include <cstring>
#include <cstdlib>

/* Argument Parse begin */

std::string input_file, ir_file;

void parse_args(int argc, char *argv[]) {
    // Usage: ./compiler -emit-ir <ir_file> <input_file>
    
    for (int i = 1; i < argc; i++) {
        if (i + 1 < argc && strcmp(argv[i], "-emit-ir") == 0) {
            ir_file.assign(argv[i + 1]);
            i += 1;
            continue;
        }
        input_file.assign(argv[i]);
    }
    if (input_file.empty()) {
        std::cerr << "error: need input file." << std::endl;
        exit(1);
    }
    if (ir_file.empty()) {
        std::cerr << "error: need output ir file." << std::endl;
        exit(1);
    }
}

/* Argument Parse end */

int main(int argc, char *argv[]) {
    parse_args(argc, argv);
    
    Manager::external = false;
    FileDealer::inputDealer(input_file.c_str());
    Lexer lexer = Lexer();
    lexer.lex();
    Parser parser = Parser(lexer.tokenList);
    std::cerr << "Parser & Visitor begin" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    AST::Ast *ast = parser.parseAst();
    Visitor visitor = Visitor();
    visitor.visitAst(ast);
    auto end = std::chrono::high_resolution_clock::now();
    std::cerr << "Parser & Visitor end, Use Time: " << std::chrono::duration<double>(end - start).count() << "s"
              << std::endl;

    // Output LLVM IR directly
    Manager::MANAGER->outputLLVM(ir_file);
    
    std::cerr << "CACT compilation finished successfully." << std::endl;
    return 0;
}
