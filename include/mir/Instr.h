//
// Created by XuRuiyuan on 2023/7/14.
//

#ifndef FASTER_MEOW_INSTR_H
#define FASTER_MEOW_INSTR_H

#include "Value.h"
// #include "BasicBlock.h"
#include <vector>
#include <set>
#include <map>
// class Value;
class Use;
class Type;
class BasicBlock;
class Function;

class Instr : public Value{
public:
    static int LOCAL_COUNT;
    static int empty_instr_cnt;
    BasicBlock *bb, *earliestBB, *latestBB;
    bool arrayInit = false;
    std::vector<Use*> useList;
    std::vector<Value*> useValueList;
    int condCount;
    bool inLoopCond = false;


public:
    Instr();
    Instr(Type* type, BasicBlock* bb, int bit);
    Instr(Type* type, BasicBlock* curBB);
    Instr(Type* type, BasicBlock* curBB, bool Lipsum);
    Instr(Type* type, Instr* instr);

    void setUse(Value* value, int idx);
    void modifyUse(Value* old, Value* now, int index);
    void modifyUse(Value* now, int index);
    bool isEnd();

    ~Instr() override = default;

    void init();
    void removeUserUse();
    void remove() override;
    void delFromNowBB();
    void setInLoopCond();
    void setNotInLoopCond();
    bool isInLoopCond();
    bool isCond();
    int getCondCount();
    void setCondCount(int condCount);
    void setArrayInit(bool arrayInit);
    bool isArrayInit();
    bool isTerminator();
    bool canComb();
    bool canCombFloat();
    bool check();

    std::string usesToString() const;
    std::string useValuesToString() const;

    virtual Instr* cloneToBB(BasicBlock* bb);
    void fix();

    virtual explicit operator std::string() const override {
        return "!inst!";
    }

    friend bool operator==(const Instr &first, const Instr &second);

    friend bool operator!=(const Instr &first, const Instr &second);

    friend bool operator<(const Instr &first, const Instr &second);
};

namespace INSTR {
    class Alu: public Instr {
    public:
        static std::string OpNameList[16];
        enum class Op{
            ADD = 0,
            FADD,
            SUB,
            FSUB,
            MUL,
            FMUL,
            DIV,
            FDIV,
            REM,
            FREM,
            AND,
            OR,
            XOR,
            SHL,
            LSHR,
            ASHR,
        };
    public:
        Op op;
    public:
        Alu(Type* type, Op op, Value* op1, Value* op2, BasicBlock* basicBlock);
        Alu(Type* type, Op op, Value* op1, Value* op2, Instr* insertBefore);

        Value* getRVal1() const ;
        Value* getRVal2() const ;

        bool isAdd() const;
        bool isSub() const;
        bool isFAdd() const;
        bool isFSub() const;
        bool isMul() const;
        bool isFMul() const;
        bool hasOneConst() const;
        bool hasTwoConst() const;
        Instr* cloneToBB(BasicBlock* bb) override;
        virtual explicit operator std::string() const override;
    };

    class Icmp: public Instr {
    public:
        enum class Op {
            EQ = 0,
            NE,
            SGT,
            SGE,
            SLT,
            SLE
        };
        Op op;
    public:
        Icmp(Op op, Value* op1, Value* op2, BasicBlock* curBB);
        Icmp(Op op, Value* op1, Value* op2, Instr* insertBefore);

        Value* getRVal1() const;
        Value* getRVal2() const;
        Instr* cloneToBB(BasicBlock* bb) override;

        virtual explicit operator std::string() const override;
    };

    class Fcmp: public Instr {
    public:
        enum class Op {
            OEQ = 0,
            ONE,
            OGT,
            OGE,
            OLT,
            OLE
        };
        Op op;
    public:
        Fcmp(Op op, Value* op1, Value* op2, BasicBlock* curBB);
        Fcmp(Op op, Value* op1, Value* op2, Instr* insertBefore);

        Value* getRVal1() const;
        Value* getRVal2() const;
        Instr* cloneToBB(BasicBlock* bb) override;
        virtual explicit operator std::string() const override;
    };

    class Zext: public Instr {
    public:
        Zext(Value* src, BasicBlock* parentBB);
        Zext(Value* src, Instr* insertBefore);

        Value* getRVal1() const;
        Instr* cloneToBB(BasicBlock* bb) override;
        virtual explicit operator std::string() const override;
    };

    class FPtosi: public Instr {
    public:
        FPtosi(Value* src, BasicBlock* parentBB);
        FPtosi(Value* src, Instr* insertBefore);

        Value* getRVal1() const;
        Instr* cloneToBB(BasicBlock* bb) override;

        virtual explicit operator std::string() const override;
    };

    class SItofp: public Instr {
    public:
        SItofp(Value* src, BasicBlock* parentBB);
        SItofp(Value* src, Instr* insertBefore);

        Value* getRVal1() const;
        Instr* cloneToBB(BasicBlock* bb) override;

        virtual explicit operator std::string() const override;
    };

    class Alloc: public Instr {
    public:
        Type* contentType;
        std::set<Instr*>* loads = new std::set<Instr*>();
    public:
        Alloc(Type* type, BasicBlock* parentBB);
        Alloc(Type* type, BasicBlock* parentBB, bool Lipsum);

        void clearLoads();
        void addLoad(Instr* instr);
        bool isArrayAlloc();

        Instr* cloneToBB(BasicBlock* bb) override;
        virtual explicit operator std::string() const override;
    };

    class Load: public Instr {
    public:
        Value* alloc;
        Instr* useStore;
    public:
        void clear();
        Load(Value* pointer, BasicBlock* parentBB);
        Load(Value* pointer, Instr* insertBefore);

        Value * getPointer() const;
        Instr* cloneToBB(BasicBlock* bb) override;
        virtual explicit operator std::string() const override;
    };

    class Store: public Instr {
    public:
        Value* alloc;
        std::set<Instr*>* users = new std::set<Instr*>();
    public:
        void addUser(Instr* user);
        void clear();

        Store(Value* value, Value* address, BasicBlock *parent);
        Store(Value* value, Value* address, Instr *insertBefore);

        Value * getValue() const;
        Value * getPointer() const;
        Instr* cloneToBB(BasicBlock* bb) override;
        virtual explicit operator std::string() const override;
    };

    class GetElementPtr: public Instr {
    public:
        GetElementPtr(Type* pointeeType, Value* ptr, std::vector<Value*>* idxList, BasicBlock* bb);

        Type* getPointeeType();
        Value * getPtr() const;
        int getOffsetCount();

        std::vector<Value *> * getIdxList() const;
        Value* getIdxValueOf(int i);

        void modifyPtr(Value* ptr);
        void modifyIndexs(std::vector<Value*>* indexs);

        Instr* cloneToBB(BasicBlock* bb) override;
        virtual explicit operator std::string() const override;
    };

    class BitCast: public Instr {
    public:
        BitCast(Value* srcValue, Type* dstType, BasicBlock* parent);

        Value * getSrcValue() const;
        Type* getDstType();
        Instr* cloneToBB(BasicBlock* bb) override;
        virtual explicit operator std::string() const override;
    };

    class Call: public Instr {
    public:
        // std::vector<Value *>* paramList = new std::vector<Value *>();

        Call(Function* func, std::vector<Value*>* paramList, BasicBlock* parent);
        Call(Function* func, std::vector<Value*>* paramList, Instr* insertBefore);

        Function * getFunc() const;
        std::vector<Value *>* getParamList() const;
        Instr* cloneToBB(BasicBlock* bb) override;

        virtual explicit operator std::string() const override;
    };

    class Phi: public Instr {
    public:
        bool isLCSSA = false;
    public:
        Phi(Type* type, std::vector<Value*>* values, BasicBlock* parent);
        Phi(Type* type, std::vector<Value*>* values, BasicBlock* parent, bool isLCSSA);

        std::vector<Value*>* getOptionalValues();
        void addOptionalValue(Value* value);
        Instr* cloneToBB(BasicBlock* bb) override;
        void simple(std::vector<BasicBlock*>* oldPres,
                    std::vector<BasicBlock*>* newPres);
        virtual explicit operator std::string() const override;
    };

    class PCopy: public Instr {
    public:
        std::vector<Value*> *LHS, *RHS;
    public:
        PCopy(std::vector<Value*> *LHS,
              std::vector<Value*> *RHS,
              BasicBlock* parent);
        void addToPC(Value* tag, Value* src);
        std::vector<Value*>* getLHS();
        std::vector<Value*>* getRHS();
        virtual explicit operator std::string() const override;
    };

    class Move: public Instr {
    public:
        Value *src, *dst;
    public:
        Move(Type* type, Value* dst, Value* src, BasicBlock* parent);

        Value* getSrc();
        Value* getDst();
        virtual explicit operator std::string() const override;
    };

    class Branch: public Instr {
    public:
        BasicBlock *thenTarget, *elseTarget;
    public:
        Branch(Value* cond, BasicBlock* thenTarge, BasicBlock* elseTarget, BasicBlock* parent);
        Value * getCond() const;
        BasicBlock * getThenTarget() const;
        BasicBlock * getElseTarget() const;
        void setThenTarget(BasicBlock* thenTarget);
        void setElseTarget(BasicBlock* elseTarget);
        Instr* cloneToBB(BasicBlock* bb) override;
        virtual explicit operator std::string() const override;
    };

    class Jump: public Instr {
    public:
        Jump(BasicBlock* target, BasicBlock* parent);
        BasicBlock * getTarget() const;
        Instr* cloneToBB(BasicBlock* bb) override;
        virtual explicit operator std::string() const override;
    };

    class Return: public Instr {
    public:
        explicit Return(BasicBlock* parent);
        Return(Value* retValue, BasicBlock* parent);

        bool hasValue();
        Value * getRetValue() const;
        Instr* cloneToBB(BasicBlock* bb) override;
        virtual explicit operator std::string() const override;
    };


    extern std::string get_alu_op_name(Alu::Op op);
    extern std::string get_icmp_op_name(Icmp::Op op);
    extern std::string get_fcmp_op_name(Fcmp::Op op);

}


#endif //FASTER_MEOW_INSTR_H
