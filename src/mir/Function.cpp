//
// Created by XuRuiyuan on 2023/7/15.
//

#include <utility>
#include <cassert>
#include <iostream>
#include <fstream>

#include "../../include/util/util.h"
#include "../../include/mir/Function.h"
#include "../../include/util/Manager.h"
#include "../../include/mir/BasicBlock.h"
#include "sstream"
#include "CloneInfoMap.h"

Function::Function(std::string string, std::vector<Param *> *vector, Type *pType) {
    begin = new BasicBlock();
    end = new BasicBlock();
    begin->next = end;
    end->prev = begin;
    this->name = std::move(string);
    this->params = vector;
    for (Function::Param *param: (*params)) {
        param->parentFunc = this;
    }
    retType = pType;
    this->type = pType;
}

Function::Function(bool isExternal, std::string string, std::vector<Param *> *vector, Type *pType) {
    begin = new BasicBlock();
    end = new BasicBlock();
    begin->next = end;
    end->prev = begin;
    this->name = std::move(string);
    this->params = vector;
    for (Function::Param *param: (*params)) {
        param->parentFunc = this;
    }
    retType = pType;
    this->type = pType;
    this->isExternal = isExternal;
}

// Function *Function::get_this() {
//     return __GET_V_SP__(Function);
// }

Function::Param::Param(Type *type, int idx)
        : idx(idx) {
    this->type = type;
    if (!Manager::external) {
        this->prefix = LOCAL_PREFIX;
        this->name = FPARAM_NAME_PREFIX + std::to_string(FPARAM_COUNT++);
    }
}

Function::Param::operator std::string() const {
    return std::string(*type) + " " + getName();
}

// std::vector<Function::Param *> Function::Param::wrapParam(int size, Type *types[]) {
//     std::vector<Param *> params_temp;
//     for (int i = 0; i < size; i++) {
//         params_temp.push_back(new Param(types[i], i));
//     }
//     return params_temp;
// }

//bool Function::Param::hasBody() {
//    return entry != nullptr;
//}

bool Function::hasBody() {
    return entry != nullptr;
}

bool Function::hasRet() {
    return !retType->is_void_type();
}

void Function::insertAtBegin(BasicBlock *bb) {
    bb->set_prev(begin);
    bb->set_next(begin->next);
    begin->next->set_prev(bb);
    begin->set_next(bb);
}

void Function::insertAtEnd(BasicBlock *in) {
    in->set_prev(end->prev);
    in->set_next(end);
    end->prev->set_next(in);
    end->set_prev(in);
}

BasicBlock *Function::getBeginBB() const {
    return (BasicBlock *) begin->next;
}

BasicBlock *Function::getEndBB() const {
    return (BasicBlock *) end->prev;
}

bool Function::isEmpty() {
    return begin->next->next == nullptr;
}

void Function::addLoopHead(BasicBlock *bb) {
    loopHeads->insert(bb);
}

void Function::inlineToFunc(Function *tagFunc, BasicBlock *retBB, INSTR::Call *call, Loop *loop) {
    INSTR::Phi *retPhi = nullptr;
    if (dynamic_cast<INSTR::Phi*>(retBB->getEndInstr()) != nullptr) {
        retPhi = (INSTR::Phi*) retBB->getBeginInstr();
    }
    for (BasicBlock* bb = getBeginBB(); bb->next != nullptr; bb = (BasicBlock*) bb->next) {
//        if (bb->label == "b31") {
//            printf("copy b31");
//        }
        bb->cloneToFunc(tagFunc, loop);
    }
    std::vector<Value*>* callParams = call->getParamList();
    std::vector<Function::Param*>* funcParams = this->params;
    for (int i = 0; i < callParams->size(); ++i) {
        CloneInfoMap::addValueReflect((*funcParams)[i], (*callParams)[i]);
    }
    for (BasicBlock* bb = getBeginBB(); bb->next != nullptr; bb = (BasicBlock*) bb->next) {
//        bb->cloneToFunc(tagFunc, loop);
        BasicBlock *need_fix_bb = (BasicBlock *) CloneInfoMap::getReflectedValue(bb);
        if (bb != getBeginBB()) {
            std::vector<BasicBlock *> *pres = new std::vector<BasicBlock *>();
            for (auto pre: *bb->precBBs) {
                pres->push_back((BasicBlock *) CloneInfoMap::getReflectedValue(pre));
            }
            need_fix_bb->setPrecBBs(pres);
        }

        if (dynamic_cast<INSTR::Return *>(bb->getEndInstr()) == nullptr) {
            std::vector<BasicBlock *> *succs = new std::vector<BasicBlock *>();
            for (auto succ: *bb->succBBs) {
                succs->push_back((BasicBlock *) CloneInfoMap::getReflectedValue(succ));
            }
            need_fix_bb->setSuccBBs(succs);
        }

        std::vector<BasicBlock *> *retSucc = new std::vector<BasicBlock *>();
        retSucc->push_back(retBB);

        for (auto instr = need_fix_bb->getBeginInstr(); instr->next != nullptr; instr = (Instr *) instr->next) {
            instr->fix();
            if (dynamic_cast<INSTR::Return *>(instr) != nullptr && ((INSTR::Return *) instr)->hasValue()) {
                auto jump_to_ret_bb = new INSTR::Jump(retBB, need_fix_bb);
                instr->insert_before(jump_to_ret_bb);
                retBB->addPre(need_fix_bb);
                need_fix_bb->modifySucs(retSucc);
                assert(retPhi != nullptr);
                retPhi->addOptionalValue(((INSTR::Return *) instr)->getRetValue());
                instr->remove();
            } else if (dynamic_cast<INSTR::Return *>(instr) != nullptr) {
                auto jump_to_ret_bb = new INSTR::Jump(retBB, need_fix_bb);
                instr->insert_before(jump_to_ret_bb);
                retBB->addPre(need_fix_bb);
                need_fix_bb->modifySucs(retSucc);
                instr->remove();
            }
        }
    }
}

//TODO: purefunc
bool Function::isCanGVN() {
    return isPure;
}

std::string Function::getDeclare() {
    std::stringstream ss;
    for (int i = 0; i < params->size(); ++i) {
        ss << std::string(*(*params)[i]->type);
        if (i != params->size() - 1) {
            ss << ", ";
        }
    }
    std::string paramList = ss.str();
    assert(retType->is_void_type() || retType->is_basic_type());
    return "declare " + std::string(*retType) + " @" + getName() + "(" + paramList + ")";

}

std::ostream &operator<<(std::ostream &stream, const Function *f) {
    std::stringstream ss;
    for (int i = 0; i < f->params->size(); ++i) {
        ss << std::string(*(*f->params)[i]);
        if (i != f->params->size() - 1) {
            ss << ", ";
        }
    }
    std::string paramList = ss.str();
    std::string str;
    str.append("define dso_local ").append(std::string(*f->retType)).append(" @").append(f->getName()).append(
            "(").append(paramList).append(") {\n");
    // print branch probs and freqs for heuristic
    auto bb2str = [] (BasicBlock *bb) {
        return std::string(bb->label + "_" + std::to_string(bb->loop->getLoopDepth()));
    };
    if (!f->branch_probs.empty()) {
        str.append("; branch probs: \n");
        for (auto br : f->branch_probs) {
            str.append(";   ").append(bb2str(br.first.first)).append("-->|").append(std::to_string(br.second)).append("|").append(bb2str(br.first.second)).append("\n");
        }
    }
    if (!f->branch_freqs.empty()) {
        str.append("; branch freqs: \n");
        for (auto br : f->branch_freqs) {
            str.append(";   ").append(bb2str(br.first.first)).append("-->|").append(std::to_string(br.second)).append("|").append(bb2str(br.first.second)).append("\n");
        }
    }
    for (BasicBlock *bb = (BasicBlock *) f->begin->next; bb->next != nullptr; bb = (BasicBlock *) bb->next) {
        str.append(std::string(*bb)).append(":\n");
        for (Instr *inst = (Instr *) bb->begin->next; inst->next != nullptr; inst = (Instr *) inst->next) {
            // Instr inst = (Instr) instNode;
            str.append("\t").append(std::string(*inst)).append("\n");
        }
    }
    str.append("}\n");
    stream << str;
    return stream;
}

std::string Function::getName() const {
    return (isTimeFunc ? "_sysy_" : "") + name;
}

void Function::print_dominator_tree() const {
    std::ofstream fout("dom-" + this->getName() + ".md");
    fout << "## `" << this->getName() << "`\n" << std::endl;
    fout << "```mermaid" << std::endl;
    fout << "graph TD;" << std::endl;
    std::map<BasicBlock *, std::string> bb2tag;
    int tag_count = 0;
    FOR_ITER_BB(bb, this) {
        bb2tag[bb] = "b" + std::to_string(++tag_count);
        fout << bb2tag[bb] << "(\"" << bb->label << ": " << bb->domTreeDeep << "\");" << std::endl;
    }
    FOR_ITER_BB(bb, this) {
        BasicBlock *idom = bb->iDominator;
        if (idom != nullptr) {
            fout << bb2tag[idom] << "-->" << bb2tag[bb] << ";" << std::endl;
        }
    }
    fout << "```\n\n" << std::endl;
    fout.close();
}

void Function::print_reversed_dominator_tree() const {
    std::ofstream fout("rdom-" + this->getName() + ".md");
    fout << "## `" << this->getName() << "`\n" << std::endl;
    fout << "```mermaid" << std::endl;
    fout << "graph TD;" << std::endl;
    std::map<BasicBlock *, std::string> bb2tag;
    int tag_count = 0;
    FOR_ITER_BB(bb, this) {
        bb2tag[bb] = "b" + std::to_string(++tag_count);
        fout << bb2tag[bb] << "(\"" << bb->label << "\");" << std::endl;
    }
    bb2tag[vExit] = "b_vExit";
    fout << bb2tag[vExit] << "(\"" << vExit->label << "\");" << std::endl;
    FOR_ITER_BB(bb, this) {
        BasicBlock *irdom = bb->iRDominator;
        if (irdom != nullptr) {
            fout << bb2tag[irdom] << "-->" << bb2tag[bb] << ";" << std::endl;
        }
    }
    fout << "```\n\n" << std::endl;
    fout.close();
}