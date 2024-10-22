// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <algorithm>
#include <benchmark/benchmark.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <numeric>
#include <string>
#include <type_traits>
#include <vector>

using namespace std;

enum class AlgType { std_func, str_member_first, str_member_last };

template <AlgType Alg, class T, T Start = T{'!'}>
void bm(benchmark::State& state) {
    const size_t HSize = static_cast<size_t>(state.range(0));
    const size_t NSize = static_cast<size_t>(state.range(1));

    std::_Find_first_of_alg = static_cast<std::_Find_first_alg_t>(state.range(2));

    using container = conditional_t<Alg == AlgType::std_func, vector<T>, basic_string<T>>;

    constexpr T HaystackFiller{' '};
    static_assert(HaystackFiller != Start);

    container h(HSize, HaystackFiller);
    container n(NSize, T{0});

    for (auto _ : state) {
        benchmark::DoNotOptimize(h);
        benchmark::DoNotOptimize(n);
        if constexpr (Alg == AlgType::str_member_first) {
            benchmark::DoNotOptimize(h.find_first_of(n));
        } else if constexpr (Alg == AlgType::str_member_last) {
            benchmark::DoNotOptimize(h.find_last_of(n));
        } else {
            benchmark::DoNotOptimize(find_first_of(h.begin(), h.end(), n.begin(), n.end()));
        }
    }
}

void common_args(auto bm) {
    for (int arg1 : {3, 7, 15, 100, 3000}) {
        for (int arg2 : {3, 7, 15, 100, 3000}) {
            for (int arg3 : {std::_Vector_vector_table, std::_Vector_scalar_table, std::_Vector_no_table}) {
                bm->Args({arg1, arg2, arg3});
            }
        }
    }
}

BENCHMARK(bm<AlgType::str_member_first, char>)->Apply(common_args);
BENCHMARK(bm<AlgType::str_member_first, wchar_t>)->Apply(common_args);
BENCHMARK(bm<AlgType::str_member_first, char32_t>)->Apply(common_args);
BENCHMARK(bm<AlgType::str_member_first, unsigned long long>)->Apply(common_args);

BENCHMARK(bm<AlgType::str_member_last, char>)->Apply(common_args);
BENCHMARK(bm<AlgType::str_member_last, wchar_t>)->Apply(common_args);

BENCHMARK_MAIN();
