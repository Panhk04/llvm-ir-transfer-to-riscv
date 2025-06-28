//
// Created by XuRuiyuan on 2023/7/15.
//

#ifndef FASTER_MEOW_FUNCTION_H
#define FASTER_MEOW_FUNCTION_H


#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include "Value.h"
#include "Instr.h"
#include "Type.h"

class BasicBlock;
class Loop;
class Function : public Value {
public:
    std::string name;
    bool is_caller = true;
    bool is_deleted = false;
    Type* retType = nullptr;
    bool isPure = false;
    BasicBlock *entry = nullptr;
    BasicBlock *vExit = nullptr; // only for reversed dom tree

public:
    class Param : public Value{
    public:
        // Param *sp;
        static int FPARAM_COUNT;
        int idx;
        Function *parentFunc;
        std::set<Instr *>* loads = new std::set<Instr*>();
    public:
        explicit Param(Type *type, int idx);
        ~Param() override = default;
        virtual explicit operator std::string() const override;
        // static std::vector<Param *> wrapParam(int size, Type * types[]);
//        bool hasBody();
    };


    std::vector<Param *>* params;
    BasicBlock *begin;
    BasicBlock *end;
    std::unordered_map<BasicBlock*, std::vector<BasicBlock*>*>* preMap;
    std::unordered_map<BasicBlock*, std::vector<BasicBlock*>*>* sucMap;
    std::set<BasicBlock*>* BBs = new std::set<BasicBlock*>();
    std::set<BasicBlock*>* loopHeads = new std::set<BasicBlock*>();
    bool isExternal = false;
public:
    explicit Function(std::string string, std::vector<Param *>* vector, Type *pType);
    explicit Function(bool isExternal, std::string string, std::vector<Param *>* vector, Type *pType);

    ~Function() override = default;

    bool hasBody();
    bool hasRet();

    void insertAtBegin(BasicBlock *bb);
    void insertAtEnd(BasicBlock* in);
    BasicBlock* getBeginBB() const;
    BasicBlock* getEndBB() const;
    bool isEmpty();
    void addLoopHead(BasicBlock* bb);
    void inlineToFunc(Function*  tagFUnc,
                      BasicBlock* retBB,
                      INSTR::Call* call,
                      Loop* loop);
    // Function *get_this();
    bool isTimeFunc = false;
    bool getDeleted() const {return is_deleted;}
    bool isCanGVN();

    std::string getDeclare();
    std::string getName() const override;

    friend std::ostream &operator<<(std::ostream &stream, const Function *f);

    std::map<std::pair<BasicBlock*, BasicBlock*>, double> branch_probs;
    std::map<std::pair<BasicBlock*, BasicBlock*>, double> branch_freqs;
    std::vector<BasicBlock *> newBBs;
    std::vector<BasicBlock *> reversedPostOrder;

    void print_dominator_tree() const;
    void print_reversed_dominator_tree() const;
};



#endif //FASTER_MEOW_FUNCTION_H
