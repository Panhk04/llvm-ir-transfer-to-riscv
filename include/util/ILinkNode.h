//
// Created by Administrator on 2023/7/13.
//

#ifndef FASTER_MEOW_ILINKNODE_H
#define FASTER_MEOW_ILINKNODE_H

#include <memory>

class ILinkNode {
public:
    ILinkNode *prev{};
    ILinkNode *next{};
    static int id_num;
    int id = id_num++;

public:
    ILinkNode() = default;

    virtual ~ILinkNode() = default;

    bool has_prev() const;

    bool has_next() const;

    virtual void remove();

    void set_prev(ILinkNode *prev);

    void set_next(ILinkNode *next);

    // this -> node
    void insert_after(ILinkNode *node);

    // insert another node before this
    // -> node -> this->
    void insert_before(ILinkNode *node);

    // std::shared_ptr<ILinkNode> get_this();

    bool operator==(const ILinkNode& other) const;

    bool operator!=(const ILinkNode& other) const;

    bool operator<(const ILinkNode& other) const;

};

#endif //FASTER_MEOW_ILINKNODE_H
