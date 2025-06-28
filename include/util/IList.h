//
// Created by XuRuiyuan on 2023/7/15.
//

#ifndef FASTER_MEOW_ILIST_H
#define FASTER_MEOW_ILIST_H


#include <iterator>
#include "iostream"
#include <memory>

// bool operator==(const std::shared_ptr<int> &a, const std::shared_ptr<int> &b) {
//     // printf(" == \t%d\t%d\n", *a, *b);
//     return *a == *b;
// }
//
class Int {
public:
    int i;
public:
    Int(int i) : i(i) {}

    friend bool operator==(const std::shared_ptr<Int> &a, const std::shared_ptr<Int> &b) {
        printf(" == \t%d\t%d\n", a->i, b->i);
        return a->i == b->i;
    }

    friend int operator+(int a, const std::shared_ptr<Int> &b) {
        return a + (b->i);
    }

    friend std::ostream& operator<<(std::ostream& ostream, const Int& ii) {
        ostream << ii.i;
        return ostream;
    }
};

namespace Coll {
    template<typename T>
    class iterator {
        std::shared_ptr<T> cur;
    public:
        explicit iterator(std::shared_ptr<T> i) : cur(i) {};

        explicit iterator() = default;

        iterator &operator++() {
            cur = cur->next;
            // printf("%d\n", *cur);
            // cur = std::make_shared<T>(*cur + 1);
            return *this;
        }

        iterator operator++(int) {
            iterator ret_val = *this;
            ++(*this);
            return ret_val;
        }

        bool operator==(const iterator &other) const {
            // printf("comp if == bet:\t%d\t%d\n", *(this->cur), *(other.cur));
            return cur == other.cur;
        }

        bool operator!=(const iterator &other) const {
            // printf("comp if != bet:\t%d\t%d\n", *(this->cur) , *(other.cur));
            // printf("comp res %d\n", cur == other.cur);
            return !(cur == other.cur);
        }

        T operator*() {
            // return cur->i;
            return *cur;
        }

    };

    // template<ILinkNode FROM, ILinkNode TO>
    template<typename T>
    class IList {
    public:
        std::shared_ptr<T> head = std::make_shared<T>();
        std::shared_ptr<T> tail = std::make_shared<T>();
        long size = 0;
    public:
        IList() {}

        IList(std::shared_ptr<T> head, std::shared_ptr<T> tail) :
                head(head), tail(tail), size(0) {}

        //iterator<T> begin() { return iterator(head->next); }

        //iterator<T> end() { return iterator(tail); }

    };

    // void test_range() {
    //     std::shared_ptr<Int> x = std::make_shared<Int>(2);
    //     std::cout << 2 + x << std::endl;
    //     // std::shared_ptr<Int> y = std::make_shared<Int>(2);
    //     // if (x == y) {
    //     //
    //     // }
    //     // IList<int> r{};
    //     // // manually::IList<int> r(std::make_shared<int>(1), std::make_shared<int>(5));
    //     // for (auto v: r) {
    //     //     std::cout << v << ',';
    //     //     for (int i = 0; i < 100000; i++) {}
    //     // }
    //     std::cout << '\n';
    // }

}


#endif //FASTER_MEOW_ILIST_H
