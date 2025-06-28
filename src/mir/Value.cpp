//
// Created by Administrator on 2023/7/13.
//

#include "../../include/util/util.h"
#include "../../include/mir/Use.h"
#include "../../include/mir/Value.h"
#include "../../include/mir/Type.h"
#include "../../include/mir/Constant.h"
#include "../../include/mir/Instr.h"


Value::Value() {
    beginUse = new Use();
    endUse = new Use();
    beginUse->set_next(endUse);
    endUse->set_prev(beginUse);
}

Value::Value(Type *type) {
    beginUse = new Use();
    endUse = new Use();
    this->type = type;
    beginUse->set_next(endUse);
    endUse->set_prev(beginUse);
}

void Value::insertAtEnd(Use *use) {
    endUse->insert_before(use);
}

std::string Value::getName() const {
    return prefix + name;
}

Use *Value::getBeginUse() {
    return dynamic_cast<Use *>(beginUse->next);
}

Use *Value::getEndUse() {
    return dynamic_cast<Use *>(endUse->prev);
}

bool Value::onlyOneUser() {
    return dynamic_cast<Use *>(beginUse->next)
           == dynamic_cast<Use *>(endUse->prev);
}

bool Value::operator==(const Value &other) const {
    return id == other.id;
}

bool Value::operator!=(const Value &other) const {
    return id != other.id;
}

bool Value::operator<(const Value &other) const {
    return id < other.id;
}

std::string Value::getDescriptor() const {
    return std::string(*type) + " " + getName();;
}

std::ostream &operator<<(std::ostream &ostream, const Value *value) {
    ostream << value->type << " " << value->prefix << value->name;
    return ostream;
}

bool Value::isNoUse() {
    return beginUse->next->next == nullptr;
}

bool Value::isAlu() {
    return tag == ValueTag::bino;
}

bool Value::isStore() {
    return tag == ValueTag::store;
}

bool Value::isLoad() {
    return tag == ValueTag::load;
}

bool Value::isAlloc() {
    return tag == ValueTag::alloc;
}

bool Value::isGep() {
    return tag == ValueTag::gep;
}

bool Value::isCall() {
    return tag == ValueTag::call;
}

bool Value::isPhi() {
    return tag == ValueTag::phi;
}

bool Value::isIcmp() {
    return tag == ValueTag::icmp;
}

bool Value::isFcmp() {
    return tag == ValueTag::fcmp;
}

bool Value::isBranch() {
    return tag == ValueTag::branch;
}

bool Value::isJump() {
    return tag == ValueTag::jump;
}

bool Value::isBJ() {
    return tag == ValueTag::branch || tag == ValueTag::jump;
}

bool Value::isConstant() {
    return dynamic_cast<Constant *>(this) != nullptr;
}

bool Value::isConstantInt() {
    return dynamic_cast<ConstantInt *>(this) != nullptr;
}

bool Value::isConstantFloat() {
    return dynamic_cast<ConstantFloat *>(this) != nullptr;
}


bool Value::isReturn() {
    return tag == ValueTag::ret;
}

void Value::modifyAllUseThisToUseA(Value *A) {
    Use *use = (Use *) beginUse->next;
    while (use->next != nullptr) {
        Instr *user = (Instr *) use->user;
        user->modifyUse(this, A, use->idx);
        use = (Use *) use->next;
    }
}

// Value *Value::get_this() {
//     return __GET_V_SP__(Value);
// }
