//
// Created by XuRuiyuan on 2023/7/14.
//

#include "../../include/mir/Use.h"
#include <string>
#include <sstream>
#include <cassert>

#include "Value.h"
#include "Instr.h"

bool operator==(const Use &first, const Use &second) {
    return first.id == second.id;
}

bool operator!=(const Use &first, const Use &second) {
    return first.id != second.id;
}

bool operator<(const Use &first, const Use &second) {
    return first.id < second.id;
}

Use::operator std::string() const {
    std::stringstream ss;
    assert(this->used != nullptr && this->user != nullptr);
    ss << used->getDescriptor() << "@[" << std::string(*user) << "]";
    return ss.str();
}