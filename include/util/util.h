//
// Created by Administrator on 2023/7/15.
//

#ifndef FASTER_MEOW_UTIL_H
#define FASTER_MEOW_UTIL_H

#include "memory"
#include "string"
#include <cassert>
#include <variant>
#include <optional>

#define __sp__ std::shared_ptr
#define __cst_sp__(T) std::shared_ptr<T>&
#define MAKE_SHARED_PTR(type, ...) std::make_shared<type>(__VA_ARGS__)
// #define __GET_V_SP__(T) std::enable_shared_from_this<T>::shared_from_this();
// #define __ASSIGN_V_SP__(T) this->sp=__GET_V_SP__(T)
#define ASSERT

#define __IS__(t, T) dynamic_cast<T *>(t) != nullptr
#define MITAG(tag) MachineInst::Tag::tag
#define CAST_IF(ptr, t, T) auto *ptr = dynamic_cast<T *>(t)
#define __IS_NOT__(t, T) dynamic_cast<T *>(t) == nullptr
#define SP_ALIGN 8
#define FOR_ITER_MI(mi, mb) for(MachineInst*mi=dynamic_cast<MachineInst*>(mb->beginMI->next);mi->next!=nullptr;mi=dynamic_cast<MachineInst*>(mi->next))
#define FOR_ITER_REVERSE_MI(mi, mb) for(MachineInst*mi=dynamic_cast<MachineInst*>(mb->endMI->prev);mi->prev!=nullptr;mi=dynamic_cast<MachineInst*>(mi->prev))
#define FOR_ITER_MB(mb, mf) for(McBlock*mb=(McBlock*)mf->beginMB->next;mb->next!=nullptr;mb=(McBlock *)mb->next)
#define FOR_ITER_BB(bb, func) for(BasicBlock*bb=(BasicBlock*)func->begin->next;bb->next!=nullptr;bb=(BasicBlock *)bb->next)
#define FOR_ITER_MF(mf, mp) for(McFunction*mf=(McFunction*)mp->beginMF->next;mf->next!=nullptr;mf=(McFunction *)mf->next)
#define FOR_ITER_REVERSE_MB(mb, mf) for(McBlock*mb=(McBlock*)mf->endMB->prev;mb->prev!=nullptr;mb=(McBlock *)mb->prev)
namespace CenterControl {
    extern bool _O2;
    extern bool _GLOBAL_BSS;
    extern bool _FAST_REG_ALLOCATE;
    extern bool _IMM_TRY;
    extern bool STABLE;
    extern bool _ONLY_FRONTEND;
    extern bool _cutLiveNessShortest;
    extern bool _FixStackWithPeepHole;
    extern bool _GPRMustToStack;
    extern bool _FPRMustToStack;
    extern bool _GPR_NOT_RESPILL;
    extern bool _FPR_NOT_RESPILL;
    extern bool _AGGRESSIVE_ADD_MOD_OPT;
    extern bool _OPEN_PARALLEL;
    // extern bool _OPEN_PARALLEL_BACKEND;
    extern bool AlreadyBackend;

    extern std::string _TAG;
    extern bool _FUNC_PARALLEL;
    // extern bool _initAll;


    //O2-control
    extern bool _CLOSE_ALL_FLOAT_OPT;
    extern bool _OPEN_CONST_TRANS_FOLD;
    extern bool _OPEN_FLOAT_INSTR_COMB;
    extern bool _OPEN_FLOAT_LOOP_STRENGTH_REDUCTION;
    extern bool _OPEN_FTOI_ITOF_GVN;


    extern bool _STRONG_FUNC_INLINE;
    extern bool _STRONG_FUNC_GCM;

    extern double _HEURISTIC_BASE;
    extern bool _PREC_MB_TOO_MUCH;
    extern std::string OUR_MEMSET_TAG;
    extern bool _GEP_CONST_BROADCAST;
    extern bool USE_ESTIMATE_BASE_SORT;
    extern bool ACROSS_BLOCK_MERGE_MI;

    extern bool _OPEN_LOOP_SWAP;

}
// #define _BACKEND_COMMENT_OUTPUT
// #define LIVENESS_OUT
// #define _OPEN_PARALLEL_BACKEND
namespace ENUM {
    enum STACK_FIX {
        NO_NEED,
        VAR_STACK,
        ONLY_PARAM,
        INT_TOTAL_STACK,
        FLOAT_TOTAL_STACK
    };
}
#define GPR_NUM 17
#define FPR_NUM 32
#define MIN(a, b) (a<b?a:b)
#define ABS(num) ((num)<0?-(num):(num))
#define NUM_AND_BIT(num, bit) ((1u<<(bit))&num)
#define NUM_HAS_BIT(num, bit) ((1u<<(bit))&num)!=0u
#define NUM_NO_BIT(num, bit) ((1u<<(bit))&num)==0u
#define NUM_SET_BIT(num, bit) num|=(1u<<(bit))
using IFV = std::variant<int, float>;
union IFU {
    int i = 0;
    float f;
};

enum class DataType {
    I1 = 0,
    I32 = 1,
    F32 = 2,
};

struct BKConstVal {
    DataType dataType;
    union {
        int iv;
        float fv;
    };

    BKConstVal() {}

    BKConstVal(int v) : dataType(DataType::I32) { iv = v; }

    BKConstVal(float v) : dataType(DataType::F32) { fv = v; }

    bool isIntZero() const {
        return dataType == DataType::I32 && iv == 0;
    }

    bool isIntOne() const {
        return dataType == DataType::F32 && iv == 1;
    }

    BKConstVal getNeg() const {
        BKConstVal newBKCst = (*this);
        if (dataType == DataType::I32) {
            newBKCst.iv = -iv;
            return newBKCst;
        } else if (dataType == DataType::F32) {
            newBKCst.fv = -fv;
            return newBKCst;
        }
        exit(54);
    }

    bool operator==(const BKConstVal &other) const {
        if (dataType != other.dataType) {
            return false;
        } else if (dataType == DataType::F32) {
            return fv == other.fv;
        } else if (dataType == DataType::I32) {
            return iv == other.iv;
        } else {
            exit(55);
        }
    }

    bool operator!=(const BKConstVal &other) const {
        if (dataType != other.dataType) {
            return true;
        } else if (dataType == DataType::F32) {
            return fv != other.fv;
        } else if (dataType == DataType::I32) {
            return iv != other.iv;
        } else {
            exit(56);
        }
    }

    virtual explicit operator std::string() const {
        if (dataType == DataType::F32) {
            return std::to_string(fv);
        } else if (dataType == DataType::I32) {
            return std::to_string(iv);
        } else {
            exit(57);
        }
    }
};
namespace std {
    template <> class hash<BKConstVal> {
    public:
        size_t operator()(const BKConstVal &r) const { return r.iv; }
    };
}
#endif //FASTER_MEOW_UTIL_H
