//
// Created by Administrator on 2023/7/15.
//

#ifndef FASTER_MEOW_MANAGER_H
#define FASTER_MEOW_MANAGER_H


#include <string>
#include <map>
#include <unordered_map>
#include "vector"
#include "memory"
#include "frontend/Symbol.h"

class Function;

class GlobalValue;

class Initial;

class Manager {
public:

    static Manager *MANAGER;
    static std::string MAIN_FUNCTION;
    static bool external;
    std::unordered_map<GlobalValue *, Initial *> *globals = new std::unordered_map<GlobalValue *, Initial *>();
    static std::map<std::string, Function *> *functions;
    std::vector<Function *> externalFuncList;
    static int outputLLVMCnt;
    // public void outputLLVM() throws FileNotFoundException {
    //             outputLLVM("llvmOf-" + outputLLVMCnt++);
    //     }
    // public void outputLLVM(String llvmFilename) throws FileNotFoundException {
    //             outputLLVM(new FileOutputStream(llvmFilename + ".ll"));
    //     }
    static int outputMIcnt;

    // public void outputMI(String miFilename) throws FileNotFoundException {
    //             outputMI(new FileOutputStream(miFilename + ".txt"));
    //     }

    Manager();

    std::vector<Function *>* getFunctionList();

    void addGlobal(Symbol *symbol);

    void addFunction(Function *function);

    bool hasMainFunction();

    std::ostream &outputLLVM(std::ostream &stream);
    void outputLLVM(std::string filename);
    // void outputMI();
    // void outputMI(bool flag);
    // void outputMI(OutputStream out);
    // std::unordered_map<GlobalValue *, Initial *> getGlobals();

    // std::unordered_map<std::string, Function *> *getFunctions();

};


namespace ExternFunction {
    // CACT runtime functions
    extern Function *GET_INT;      // int get_int()
    extern Function *GET_CHAR;     // char get_char() 
    extern Function *GET_FLOAT;    // float get_float()
    extern Function *PRINT_INT;    // void print_int(int)
    extern Function *PRINT_CHAR;   // void print_char(char)
    extern Function *PRINT_FLOAT;  // void print_float(float)
};

#endif //FASTER_MEOW_MANAGER_H
