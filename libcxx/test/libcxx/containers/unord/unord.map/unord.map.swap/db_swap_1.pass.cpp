//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <unordered_map>

// template <class Key, class Value, class Hash = hash<Key>, class Pred = equal_to<Key>,
//           class Alloc = allocator<pair<const Key, Value>>>
// class unordered_map

// void swap(unordered_map& x, unordered_map& y);

// This test requires debug mode, which the library on macOS doesn't have.
// UNSUPPORTED: with_system_cxx_lib=macosx

#define _LIBCPP_DEBUG 1
#define _LIBCPP_ASSERT(x, m) ((x) ? (void)0 : std::exit(0))

#include <unordered_map>
#include <cassert>

#include "test_macros.h"

int main(int, char**)
{
    {
        typedef std::pair<int, int> P;
        P a1[] = {P(1, 1), P(3, 3), P(7, 7), P(9, 9), P(10, 10)};
        P a2[] = {P(0, 0), P(2, 2), P(4, 4), P(5, 5), P(6, 6), P(8, 8), P(11, 11)};
        std::unordered_map<int, int> c1(a1, a1+sizeof(a1)/sizeof(a1[0]));
        std::unordered_map<int, int> c2(a2, a2+sizeof(a2)/sizeof(a2[0]));
        std::unordered_map<int, int>::iterator i1 = c1.begin();
        std::unordered_map<int, int>::iterator i2 = c2.begin();
        swap(c1, c2);
        c1.erase(i2);
        c2.erase(i1);
        std::unordered_map<int, int>::iterator j = i1;
        c1.erase(i1);
        assert(false);
    }

    return 0;
}