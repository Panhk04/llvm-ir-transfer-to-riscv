//
// Created by XuRuiyuan on 2023/8/1.
//

#ifndef FASTER_MEOW_FRONTENDINIT_H
#define FASTER_MEOW_FRONTENDINIT_H

#include "util/util.h"
#include "File.h"
#include "Lexer.h"

FILE *FileDealer::fpInput;
FILE *FileDealer::fpOutput;
FILE *FileDealer::fpArm;
char *FileDealer::fileStr;

bool Lexer::detect_float = false;

namespace CenterControl {
    bool _O2 = false;
    bool _GLOBAL_BSS = true;
    bool _FAST_REG_ALLOCATE = true;
    bool _IMM_TRY = false;
    bool STABLE = true;
    bool _ONLY_FRONTEND = true;  // Set to true since we only want frontend
    bool _cutLiveNessShortest = false;
    bool _FixStackWithPeepHole = true;
    bool _GPRMustToStack = true;
    bool _FPRMustToStack = true;
    bool _GPR_NOT_RESPILL = true;
    bool _FPR_NOT_RESPILL = true;
    bool _AGGRESSIVE_ADD_MOD_OPT = false;
    bool _OPEN_PARALLEL = false;  // Disabled for CACT
    bool AlreadyBackend = false;

    std::string _TAG = "1";
    bool _FUNC_PARALLEL = false;

    //O2-control
    bool _CLOSE_ALL_FLOAT_OPT = false;
    bool _OPEN_CONST_TRANS_FOLD = _CLOSE_ALL_FLOAT_OPT ? false : true;
    bool _OPEN_FLOAT_INSTR_COMB = _CLOSE_ALL_FLOAT_OPT ? false : true;
    bool _OPEN_FLOAT_LOOP_STRENGTH_REDUCTION = _CLOSE_ALL_FLOAT_OPT ? false : true;
    bool _OPEN_FTOI_ITOF_GVN = _CLOSE_ALL_FLOAT_OPT ? false : true;

    bool _STRONG_FUNC_INLINE = true;
    bool _STRONG_FUNC_GCM = false;

    double _HEURISTIC_BASE = 1.45;
    bool _PREC_MB_TOO_MUCH = false;
    std::string OUR_MEMSET_TAG = "__our_memset__";
    bool _GEP_CONST_BROADCAST = true;
    bool USE_ESTIMATE_BASE_SORT = true;
    bool ACROSS_BLOCK_MERGE_MI = false;

    bool _OPEN_LOOP_SWAP = true;
}

#include "ILinkNode.h"

int ILinkNode::id_num = 0;

#include "Value.h"

int Value::value_num = 0;
std::string Value::GLOBAL_PREFIX = "@";
std::string Value::LOCAL_PREFIX = "%";
std::string Value::GLOBAL_NAME_PREFIX = "g";
std::string Value::LOCAL_NAME_PREFIX = "v";
std::string Value::FPARAM_NAME_PREFIX = "f";
std::string Value::tagStrList[24] = {
        "value",
        "func",
        "bb",
        "bino",
        "ashr",
        "icmp",
        "fcmp",
        "fneg",
        "zext",
        "fptosi",
        "sitofp",
        "alloc",
        "load",
        "store",
        "gep",
        "bitcast",
        "call",
        "phi",
        "pcopy",
        "move",
        "branch",
        "jump",
        "ret",
};

#include "mir/Use.h"

int Use::use_num = 0;

#include "Type.h"

VoidType *VoidType::VOID_TYPE = new VoidType();

#include "Instr.h"

std::string INSTR::Alu::OpNameList[16] = {
        "add",
        "fadd",
        "sub",
        "fsub",
        "mul",
        "fmul",
        "sdiv",
        "fdiv",
        "srem",
        "frem",
        "and",
        "or",
        "xor",
        "shl",
        "lshr",
        "ashr",
};
int Instr::LOCAL_COUNT = 0;
int Instr::empty_instr_cnt = 0;

#include "Function.h"

int Function::Param::FPARAM_COUNT = 0;

#include "Constant.h"

int Constant::const_num = 0;

ConstantFloat *ConstantFloat::CONST_0F = new ConstantFloat(0.0f);
ConstantFloat *ConstantFloat::CONST_1F = new ConstantFloat(1.0f);
int ConstantFloat::float_const_cnt = 1;

ConstantInt *ConstantInt::CONST_0 = new ConstantInt(0);
ConstantInt *ConstantInt::CONST_1 = new ConstantInt(1);
std::map<int, ConstantInt *> ConstantInt::const_int_map;

#include "Manager.h"

std::string Manager::MAIN_FUNCTION = "main";
bool Manager::external = true;
std::map<std::string, Function *> *Manager::functions = new std::map<std::string, Function *>;
int Manager::outputLLVMCnt = 0;
int Manager::outputMIcnt = 0;

// CACT Runtime Functions
Function *ExternFunction::GET_INT = new Function(true, "get_int", new std::vector<Function::Param *>{},
                                                 BasicType::I32_TYPE);
Function *ExternFunction::GET_CHAR = new Function(true, "get_char", new std::vector<Function::Param *>{},
                                                BasicType::I32_TYPE);
Function *ExternFunction::GET_FLOAT = new Function(true, "get_float", new std::vector<Function::Param *>{},
                                                   BasicType::F32_TYPE);
Function *ExternFunction::PRINT_INT = new Function(true, "print_int",
                                                 new std::vector<Function::Param *>{
                                                         new Function::Param(BasicType::I32_TYPE, 0)},
                                                 VoidType::VOID_TYPE);
Function *ExternFunction::PRINT_CHAR = new Function(true, "print_char",
                                                new std::vector<Function::Param *>{
                                                        new Function::Param(BasicType::I32_TYPE, 0)},
                                                VoidType::VOID_TYPE);
Function *ExternFunction::PRINT_FLOAT = new Function(true, "print_float",
                                                   new std::vector<Function::Param *>{
                                                           new Function::Param(BasicType::F32_TYPE, 0)},
                                                   VoidType::VOID_TYPE);

Manager *Manager::MANAGER = new Manager();

#include "Initial.h"

std::string Token::list[100] = {
        "const",
        "int",
        "float",
        "break",
        "continue",
        "if",
        "else",
        "void",
        "while",
        "return",
        // ident
        "IDENT",
        // float const
        "HEX_FLOAT",
        "DEC_FLOAT",
        // int const
        "HEX_INT",
        "OCT_INT",
        "DEC_INT",
        // operator (double char)
        "&&",
        "||",
        "<=",
        ">=",
        "==",
        "!=",
        // operator (single char)
        "<",
        ">",
        "+", // regex
        "-",
        "*",
        "/",
        "%",
        "!",
        "=",
        ";",
        ",",
        "(",
        ")",
        "[",
        "]",
        "{",
        "}",
        "",
};

#include "Visitor.h"

Visitor *Visitor::VISITOR = new Visitor;
SymTable *Visitor::currentSymTable = new SymTable();
int Visitor::loopDepth = 0;
Loop *Visitor::curLoop = nullptr;



#endif //FASTER_MEOW_FRONTENDINIT_H
