//
// Created by Administrator on 2023/7/15.
//

#include "../../include/util/Manager.h"
#include "../../include/mir/Function.h"
#include "../../include/mir/GlobalVal.h"
#include "../../include/mir/Type.h"
#include "../../include/frontend/Initial.h"

#include <fstream>

Manager::Manager() {
    external = true;
    // CACT runtime functions
    addFunction(ExternFunction::GET_INT);
    addFunction(ExternFunction::GET_CHAR);
    addFunction(ExternFunction::GET_FLOAT);
    addFunction(ExternFunction::PRINT_INT);
    addFunction(ExternFunction::PRINT_CHAR);
    addFunction(ExternFunction::PRINT_FLOAT);
    external = false;
}

std::vector<Function *> *Manager::getFunctionList() {
    std::vector<Function *> *list = new std::vector<Function *>;
    for (const auto &it: *functions) {
        Function *function = it.second;
        if (function->is_deleted) continue;
        if (function->hasBody()) {
            list->push_back(function);
        }
    }
    return list;
}

void Manager::addGlobal(Symbol *symbol) {
    (*globals)[(GlobalValue *) symbol->value] = symbol->initial;
}

void Manager::addFunction(Function *function) {
    (*functions)[function->name] = function;
    externalFuncList.push_back(function);
}

bool Manager::hasMainFunction() {
    return functions->find(MAIN_FUNCTION) != functions->end();
}

// std::unordered_map<GlobalValue *, Initial *> Manager::getGlobals() {
//     return this->globals;
// }

// std::unordered_map<std::string, Function *> *Manager::getFunctions() {
//     std::unordered_map<std::string, Function *> *ret = new std::unordered_map<std::string, Function *>();
//     for (const auto &it: functions) {
//         std::string str = it.first;
//         if (!it.second->is_deleted) {
//             (*ret)[str] = it.second;
//         }
//     }
//     return ret;
// }

std::ostream &Manager::outputLLVM(std::ostream &stream) {
    for (const auto &it: *globals) {
        stream << it.first->getName() + " = dso_local global " + std::string(*it.second) << "\n";
    }
    for (const auto &it: *functions) {
        Function *f = it.second;
        if (f->is_deleted) continue;
        if (!f->hasBody()) {
            stream << f->getDeclare() << "\n";
        }
    }
    for (const auto &it: *functions) {
        Function *f = it.second;
        if (f->is_deleted) continue;
        if (f->getDeleted()) {
            continue;
        }
        if (f->hasBody()) {
            stream << f << "\n";
        }
    }
    return stream;
}

void Manager::outputLLVM(std::string filename) {
    std::ofstream fout(filename);
    this->outputLLVM(fout);
    fout.close();
}