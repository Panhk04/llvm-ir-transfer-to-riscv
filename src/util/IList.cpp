//
// Created by XuRuiyuan on 2023/7/15.
//
//
// Created by XuRuiyuan on 2023/7/15.
//

#ifndef FASTER_MEOW_ILIST_H
#define FASTER_MEOW_ILIST_H


#include <iterator>
#include "iostream"
#include "../../include/util/ILinkNode.h"
//
// namespace Coll {
//     namespace manually {
//         // template<ILinkNode FROM, ILinkNode TO>
//         template<typename T>
//         class IList {
//         public:
//             std::shared_ptr<T> head = std::shared_ptr<T>();
//             std::shared_ptr<T> tail = std::shared_ptr<T>();
//             template<T HEAD, T TAIL>
//             class iterator {
//                 std::shared_ptr<T> cur = HEAD;
//             public:
//                 explicit iterator()= default;
//
//                 iterator &operator++() {
//                     cur = cur->next;
//                     return *this;
//                 }
//
//                 iterator operator++(int) {
//                     iterator ret_val = *this;
//                     ++(*this);
//                     return ret_val;
//                 }
//
//                 bool operator==(iterator other) const { return cur == other.cur; }
//
//                 bool operator!=(iterator other) const { return !(*this == other); }
//
//                 long operator*() { return cur; }
//                 // // iterator traits
//                 // using difference_type = long;
//                 // using value_type = long;
//                 // using pointer = const long *;
//                 // using reference = const long &;
//                 // using iterator_category = std::forward_iterator_tag;
//             };
//
//             iterator begin() { return HEAD; }
//
//             iterator end() { TO->prev; }
//         };
//     } // namespace manually
//
//     void test_range() {
//         manually::Range<5, 13> r;
//         for (auto v: r) std::cout << v << ',';
//         std::cout << '\n';
//     }
//
// }


#endif //FASTER_MEOW_ILIST_H
