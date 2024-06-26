// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

#include <range_algorithm_support.hpp>

using namespace std;
using P = pair<int, int>;

// Validate dangling story
static_assert(same_as<decltype(ranges::sort(borrowed<false>{})), ranges::dangling>);
static_assert(same_as<decltype(ranges::sort(borrowed<true>{})), int*>);

struct instantiator {
    static constexpr array input = {P{-1200257975, 0}, P{-1260655766, 1}, P{-1298559576, 2}, P{-1459960308, 3},
        P{-2095681771, 4}, P{-441494788, 5}, P{-47163201, 6}, P{-912489821, 7}, P{1429106719, 8}, P{1668617627, 9}};

    template <ranges::random_access_range R>
    static constexpr void call() {
        using ranges::sort, ranges::is_sorted, ranges::iterator_t, ranges::less;

        { // Validate range overload
            auto buff = input;
            const R range{buff};
            const same_as<iterator_t<R>> auto result = sort(range, less{}, get_first);
            assert(result == range.end());
            assert(is_sorted(range, less{}, get_first));
        }

        { // Validate iterator overload
            auto buff = input;
            const R range{buff};
            const same_as<iterator_t<R>> auto result = sort(range.begin(), range.end(), less{}, get_first);
            assert(result == range.end());
            assert(is_sorted(range.begin(), range.end(), less{}, get_first));
        }

        { // Validate empty range
            const R range{span<P, 0>{}};
            const same_as<iterator_t<R>> auto result = sort(range, less{}, get_first);
            assert(result == range.end());
            assert(is_sorted(range, less{}, get_first));
        }
    }
};

constexpr void test_devcom_1559808() {
    // Regression test for DevCom-1559808, a bad interaction between constexpr vector and the use of structured bindings
    // in the implementation of ranges::sort.

    vector<int> vec(33, 42); // NB: 33 > std::_ISORT_MAX
    ranges::sort(vec);
    assert(vec.back() == 42);
}

int main() {
    static_assert((test_random<instantiator, P>(), true));
    test_random<instantiator, P>();

    static_assert((test_devcom_1559808(), true));
    test_devcom_1559808();
}
