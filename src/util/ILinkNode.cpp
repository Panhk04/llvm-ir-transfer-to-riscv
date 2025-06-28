#include "../../include/util/ILinkNode.h"
#include "../../include/util/util.h"


bool ILinkNode::has_prev() const {
    return prev != nullptr;
}

bool ILinkNode::has_next() const {
    return next != nullptr;
}

void ILinkNode::remove(){
    if (prev != nullptr) {
        prev->next = next;
    }
    if (next != nullptr) {
        next->prev = prev;
    }
}

void ILinkNode::insert_after(ILinkNode *node) {
    node->prev = this;
    node->next = this->next;
    if (next != nullptr) {
        next->prev = node;
    }
    this->next = node;
}

// node -> this
void ILinkNode::insert_before(ILinkNode *node) {
    node->next = this;
    node->prev = this->prev;
    if (prev != nullptr) {
        prev->next = node;
    }
    this->prev = node;
}

void ILinkNode::set_prev(ILinkNode *node) {
    this->prev = node;
}

void ILinkNode::set_next(ILinkNode *node) {
    this->next = node;
}

bool ILinkNode::operator==(const ILinkNode &other) const {
    return id == other.id;
}

bool ILinkNode::operator!=(const ILinkNode &other) const {
    return id != other.id;
}

bool ILinkNode::operator<(const ILinkNode &other) const {
    return id < other.id;
}
