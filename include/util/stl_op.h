//
// Created by start on 23-7-31.
//

#ifndef FASTER_COMPILER_STL_OP_H
#define FASTER_COMPILER_STL_OP_H

// contains all is pointer
template<typename T, typename _T>
int _index_of(T v, _T val) {
    return (std::find((v)->begin(), (v)->end(), (val)) - (v)->begin());
}

//#define _index_of(v, val) \
//        (std::find((v)->begin(), (v)->end(), (val)) - (v)->begin())

template<typename T, typename _T>
bool _contains_(T v, _T val) {
    return ((v)->find((val)) != (v)->end());
}

//#define _contains_(v, val) \
//        ((v)->find((val)) != (v)->end())


#define _put_all(src, dst) \
    for (auto it = (src)->begin(); it != (src)->end(); it++) {   \
        (*(dst))[it->first] = it->second;                     \
    }


#define _vector_add_all(src, dst) \
     for (auto it = src->begin(); it != src->end(); it++) {   \
        dst->push_back(*it);                     \
    }

#define _set_add_all(src, dst) \
     for (auto it = (src)->begin(); it != (src)->end(); it++) {   \
        (dst)->insert(*it);                     \
    }


#define _put_if_absent(map, key, value) \
    if (map->find(key) == map->end()) { \
        (*map)[key] = value;            \
    }


template<typename T, typename _T>
_T get_or_default(std::unordered_map<T, _T> *m, T key, _T d_value) {
    if (m->find(key) == m->end()) {
        return d_value;
    }
    return (*m)[key];
}

#define for_func_(functions) \
        for (Function* function: *(functions))


//遍历func中的bb
#define for_bb_(func) \
        for (BasicBlock* bb = (BasicBlock*) (func)->begin->next; bb->next != nullptr; bb = (BasicBlock*) bb->next)\

//遍历bb中的instr
#define for_instr_(bb) \
        for (Instr* instr = (Instr*) (bb)->begin->next; instr->next != nullptr; instr = (Instr*) instr->next)\
//遍历instr的use
#define for_use_(instr) \
        for (Use* use = (instr)->getBeginUse(); use->next != nullptr; use = (Use*) use->next)\

#define check_and_trans(type, instr) \
        if (dynamic_cast<type*>(instr) == nullptr) return false; \
        instr = (type*) (instr);

#define check_bb_instr_num(bb, num) \
        if (bb_instr_num(bb) != num) return false; \

#define init_check_trans(bb, index, type) \
        Instr* bb##_##index##_temp = get_instr_at(bb, index); \
        if (dynamic_cast<type*>(bb##_##index##_temp) == nullptr) return false; \
        type* bb##_##index = (type*) (bb##_##index##_temp);   \


#define init_check_trans_t1_t2(bb, index, _t1, _t2) \
        Instr* bb##_##index##_temp = get_instr_at(bb, index); \
        Instr* bb##_##index = nullptr;                         \
        if (dynamic_cast<_t1*>(bb##_##index##_temp) != nullptr)  { \
            bb##_##index = (_t1*) (bb##_##index##_temp);                                           \
        } else if (dynamic_cast<_t2*>(bb##_##index##_temp) != nullptr)  { \
            bb##_##index = (_t2*) (bb##_##index##_temp);                                           \
        } else {                                    \
            return false;                                            \
        }                                           \



#define init_check_trans_end(bb, index, type) \
        Instr* bb##_##end##_##index##_temp = get_instr_end_at(bb, index); \
        if (dynamic_cast<type*>(bb##_##end##_##index##_temp) == nullptr) return false; \
        type* bb##_##end##_##index = (type*) (bb##_##end##_##index##_temp);   \

#define remove_bb_and_instr(bb) \
        bb->remove(); \
        for_instr_(bb) { \
            instr->remove();\
        } \

#define check_use_at(instr, value, index) \
        if (instr->useValueList[index] != value) return false;

#define check_use_const_at(instr, _const, index) \
        if (!is_type(ConstantInt, instr->useValueList[index])) return false; \
        if (((ConstantInt*) instr->useValueList[index])->get_const_val() != _const) return false; \


#define check_ret_const(instr, _const) \
        if (dynamic_cast<INSTR::Return*>(instr) == nullptr) return false; \
        if (dynamic_cast<ConstantInt*>(instr->useValueList[0]) == nullptr) return false; \
        if (((ConstantInt*) (instr->useValueList[0]))->get_const_val() != _const) return false;

#define check_ret_val(instr, val) \
        if (dynamic_cast<INSTR::Return*>(instr) == nullptr) return false; \
        if ((instr->useValueList[0]) != val) return false; \

#define check_store_val_to_ptr(instr, val, ptr) \
        if (instr->useValueList[0] != val) return false; \
        if (instr->useValueList[1] != ptr) return false; \


#define check_icmp_val_op_const(instr, val, _op, _const) \
        if (instr->op != _op) return false; \
        if (instr->useValueList[0] != val) return false; \
        if (dynamic_cast<ConstantInt*>(instr->useValueList[1]) == nullptr) return false; \
        if (((ConstantInt*) (instr->useValueList[1]))->get_const_val() != _const) return false;


#define check_icmp_v1_op_v2(instr, v1, _op, v2) \
        if (instr->op != _op) return false; \
        if (instr->useValueList[0] != v1) return false; \
        if (instr->useValueList[1] != v2) return false; \

#define check_phi(phi, vec) \
        if (phi->useValueList->size() != vec.size()) return false; \
        for (int i = 0; i < vec.size(); i++) {                     \
            if (dynamic_cast<ConstantInt*>(phi->useValueList[i]) != nullptr && dynamic_cast<ConstantInt*>(vec[i])) { \
                if (((ConstantInt*) phi->useValueList[i])->get_const_val() != ((ConstantInt*) vec[i])->get_const_val()) return false;            \
            }\
            else if (phi->useValueList[i] != vec[i]) return false;                    \
        }
#define check_gep_equal_index_diff_array(load_1, load_2) \
        if (load_1->useValueList.size() != 3) return false; \
        if (load_2->useValueList.size() != 3) return false; \
        check_use_const_at(load_1, 0, 1);                 \
        check_use_const_at(load_2, 0, 1); \
        if (load_1->useValueList[0] == load_2->useValueList[0]) return false; \
        if (load_1->useValueList[2] != load_2->useValueList[2]) return false; \



//ALU check
#define check_alu_v1_op_v2(instr, v1, _op, v2) \
        if (instr->op != _op) return false; \
        if (instr->useValueList[0] != v1) return false; \
        if (instr->useValueList[1] != v2) return false; \



#define check_alu_v1_op_const(instr, v1, _op, _const) \
        if (instr->op != _op) return false; \
        if (instr->useValueList[0] != v1) return false; \
        if (dynamic_cast<ConstantInt*>(instr->useValueList[1]) == nullptr) return false; \
        if (((ConstantInt*) instr->useValueList[1])->get_const_val() != _const) return false; \

#define check_alu_v1_op_not_special_const(instr, v1, _op) \
        if (instr->op != _op) return false; \
        if (instr->useValueList[0] != v1) return false; \
        if (dynamic_cast<ConstantInt*>(instr->useValueList[1]) == nullptr) return false; \


#define get_const_v(val) \
        ((ConstantInt*) val)->get_const_val()\

#define check_val_mod_const(instr, val) \
        if (((INSTR::Alu*) instr)->op != INSTR::Alu::Op::REM) return false; \
        if (instr->useValueList[0] != val) return false; \
        if (dynamic_cast<ConstantInt*>(instr->useValueList[1]) == nullptr) return false; \
        if (mod_set.find(((ConstantInt*) instr->useValueList[1])->get_const_val()) == mod_set.end()) return false; \

#define check_val_mod_2(instr, val) \
        if (((INSTR::Alu*) instr)->op != INSTR::Alu::Op::REM) return false; \
        if (instr->useValueList[0] != val) return false; \
        if (dynamic_cast<ConstantInt*>(instr->useValueList[1]) == nullptr) return false; \
        if (((ConstantInt*) instr->useValueList[1])->get_const_val() != 2) return false; \




#define check_val_div_const(instr, val, _const) \
        if (instr->op != INSTR::Alu::Op::DIV) return false; \
        if (instr->useValueList[0] != val) return false;    \
        if (dynamic_cast<ConstantInt*>(instr->useValueList[1]) == nullptr) return false; \
        if (((ConstantInt*) (instr->useValueList[1]))->get_const_val() != _const) return false;



#define check_call_func_p1_p2(instr, func, p1, p2) \
        if (instr->useValueList[0] != func) return false; \
        if (instr->useValueList[1] != p1) return false; \
        if (instr->useValueList[2] != p2) return false; \


#define check_v1_add_v2(instr, v1, v2) \
        if (instr->op != INSTR::Alu::Op::ADD) return false; \
        if (instr->useValueList[0] != v1) return false; \
        if (instr->useValueList[1] != v2) return false; \

#define check_const_shl_val(instr, _const, val) \
        if (instr->op != INSTR::Alu::Op::SHL) return false; \
        if (instr->useValueList[1] != val) return false;    \
        if (dynamic_cast<ConstantInt*>(instr->useValueList[0]) == nullptr) return false; \
        if (((ConstantInt*) (instr->useValueList[0]))->get_const_val() != _const) return false;


#define check_square_len(v) \
        if (!v->type->is_pointer_type()) return false; \
        if (!((PointerType*) v->type)->inner_type->is_array_type()) return false; \
        if (((ArrayType*) ((PointerType*) v->type)->inner_type)->dims.size() != 2) return false; \
        if (((ArrayType*) ((PointerType*) v->type)->inner_type)->dims[0] != 1024) return false; \
        if (((ArrayType*) ((PointerType*) v->type)->inner_type)->dims[1] != 1024) return false; \


#define check_gep_a_i_j(_instr, _a, _i, _j) \
        check_use_at(_instr, _a, 0); \
        check_use_at(_instr, _i, 1); \
        check_use_at(_instr, _j, 2); \

#define instr_use_num(instr) \
        (instr->useValueList.size())


#define first_bb_(func) \
        ((BasicBlock*) (func)->begin->next) \

#define first_instr_(bb) \
        ((Instr*) (bb)->begin->next) \


#define last_bb_(func) \
        ((BasicBlock*) (func)->end->prev) \

#define last_instr_(bb) \
        ((Instr*) (bb)->end->prev) \

#define is_type(T, v) (dynamic_cast<T*>(v) != nullptr)

template<typename T, typename _T>
_T *_first(T *t) {
    return (_T *) t->begin->next;
}

template<typename T, typename _T>
_T *_last(T *t) {
    return (_T *) t->end->prev;
}


template<typename ... Args>
bool _type_in_types(Value *v) {
//    const int len = sizeof...(Args);
    std::vector<bool> a = {(dynamic_cast<Args *>(v) != nullptr)...};
    bool ret = false;
    std::for_each(a.begin(), a.end(), [&ret](bool x) { ret |= x; });
    return ret;
}

//Value* get_const_value() {
//
//}
//
//Value* get_none_const_value() {
//
//}

#endif //FASTER_COMPILER_STL_OP_H
