// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <algorithm>
#include <benchmark/benchmark.h>
#include <numeric>
#include <vector>

#include "skewed_allocator.hpp"

using namespace std;

template <class T>
void bm_next(benchmark::State& state) {
    const auto size = static_cast<size_t>(state.range(0));
    vector<T, not_highly_aligned_allocator<T>> v(size);
    iota(v.begin(), v.end(), T{0});

    for (auto _ : state) {
        benchmark::DoNotOptimize(v);
        bool r = next_permutation(v.begin(), v.end());
        benchmark::DoNotOptimize(r);
    }
}

void common_args(auto bm) {
    bm->Arg(3)->Arg(7)->Arg(21)->Arg(56)->Arg(120);
}

BENCHMARK(bm_next<std::uint8_t>)->Apply(common_args);
BENCHMARK(bm_next<std::uint16_t>)->Apply(common_args);
BENCHMARK(bm_next<std::uint32_t>)->Apply(common_args);
BENCHMARK(bm_next<std::uint64_t>)->Apply(common_args);
BENCHMARK(bm_next<float>)->Apply(common_args);
BENCHMARK(bm_next<double>)->Apply(common_args);


BENCHMARK_MAIN();
