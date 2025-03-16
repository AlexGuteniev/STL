// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <algorithm>
#include <benchmark/benchmark.h>
#include <cstddef>
#include <cstdint>
#include <random>
#include <type_traits>
#include <vector>

#include "skewed_allocator.hpp"

using namespace std;

// NB: This particular algorithm has std and ranges implementations with different perf characteristics!

enum class AlgType { Std, Rng };

enum class PartternType {
    TwoZones,
    RareSignleMatches,
    DenseSmallSequences,
};

template <class T, AlgType Alg, PartternType Parttern>
void bm(benchmark::State& state) {
    const auto size  = static_cast<size_t>(state.range(0));
    const auto n = static_cast<size_t>(state.range(1));

    constexpr T no_match{'-'};
    constexpr T match{'*'};

    vector<T, not_highly_aligned_allocator<T>> v(size, no_match);

    if constexpr (Parttern == PartternType::TwoZones) {
        fill(v.begin() + v.size() / 2, v.end(), match);
    } else if constexpr (Parttern == PartternType::RareSignleMatches) {
        if (size != 0 && n != 0) {
            mt19937 gen{275423};

            uniform_int_distribution<size_t> pos_dis(0, size - 1);

            const size_t single_match_amount = size / n;

            for (size_t i = 0; i != single_match_amount; ++i) {
                v[pos_dis(gen)] = match;
            }
        }
    } else if constexpr (Parttern == PartternType::DenseSmallSequences) {
        if (size != 0 && n != 0) {
            mt19937 gen{7687239};

            uniform_int_distribution<size_t> len_dis(0, n - 1);

            size_t cur_len = len_dis(gen);

            for (size_t i = 0; i != size; ++i) {
                if (cur_len != 0) {
                    v[i] = match;
                    --cur_len;
                } else {
                    cur_len = len_dis(gen);
                }
            }
        }
    }

    for (auto _ : state) {
        if constexpr (Alg == AlgType::Std) {
            benchmark::DoNotOptimize(search_n(v.begin(), v.end(), n, match));
        } else if constexpr (Alg == AlgType::Rng) {
            benchmark::DoNotOptimize(ranges::search_n(v, n, match));
        }
    }
}

void common_args_large_counts(auto bm) {
    bm->ArgPair(3000, 200)->ArgPair(3000, 40)->ArgPair(3000, 20)->ArgPair(3000, 10)->ArgPair(3000, 5);
}

void common_args(auto bm) {
    common_args_large_counts(bm);
    bm->ArgPair(3000, 2)->ArgPair(3000, 1);
}


BENCHMARK(bm<uint8_t, AlgType::Std, PartternType::TwoZones>)->Apply(common_args);
BENCHMARK(bm<uint8_t, AlgType::Rng, PartternType::TwoZones>)->Apply(common_args);
BENCHMARK(bm<uint8_t, AlgType::Std, PartternType::RareSignleMatches>)->Apply(common_args_large_counts);
BENCHMARK(bm<uint8_t, AlgType::Rng, PartternType::RareSignleMatches>)->Apply(common_args_large_counts);
BENCHMARK(bm<uint8_t, AlgType::Std, PartternType::DenseSmallSequences>)->Apply(common_args);
BENCHMARK(bm<uint8_t, AlgType::Rng, PartternType::DenseSmallSequences>)->Apply(common_args);

BENCHMARK(bm<uint16_t, AlgType::Std, PartternType::TwoZones>)->Apply(common_args);
BENCHMARK(bm<uint16_t, AlgType::Rng, PartternType::TwoZones>)->Apply(common_args);
BENCHMARK(bm<uint16_t, AlgType::Std, PartternType::RareSignleMatches>)->Apply(common_args_large_counts);
BENCHMARK(bm<uint16_t, AlgType::Rng, PartternType::RareSignleMatches>)->Apply(common_args_large_counts);
BENCHMARK(bm<uint16_t, AlgType::Std, PartternType::DenseSmallSequences>)->Apply(common_args);
BENCHMARK(bm<uint16_t, AlgType::Rng, PartternType::DenseSmallSequences>)->Apply(common_args);

BENCHMARK(bm<uint32_t, AlgType::Std, PartternType::TwoZones>)->Apply(common_args);
BENCHMARK(bm<uint32_t, AlgType::Rng, PartternType::TwoZones>)->Apply(common_args);
BENCHMARK(bm<uint32_t, AlgType::Std, PartternType::RareSignleMatches>)->Apply(common_args_large_counts);
BENCHMARK(bm<uint32_t, AlgType::Rng, PartternType::RareSignleMatches>)->Apply(common_args_large_counts);
BENCHMARK(bm<uint32_t, AlgType::Std, PartternType::DenseSmallSequences>)->Apply(common_args);
BENCHMARK(bm<uint32_t, AlgType::Rng, PartternType::DenseSmallSequences>)->Apply(common_args);

BENCHMARK(bm<uint64_t, AlgType::Std, PartternType::TwoZones>)->Apply(common_args);
BENCHMARK(bm<uint64_t, AlgType::Rng, PartternType::TwoZones>)->Apply(common_args);
BENCHMARK(bm<uint64_t, AlgType::Std, PartternType::RareSignleMatches>)->Apply(common_args_large_counts);
BENCHMARK(bm<uint64_t, AlgType::Rng, PartternType::RareSignleMatches>)->Apply(common_args_large_counts);
BENCHMARK(bm<uint64_t, AlgType::Std, PartternType::DenseSmallSequences>)->Apply(common_args);
BENCHMARK(bm<uint64_t, AlgType::Rng, PartternType::DenseSmallSequences>)->Apply(common_args);


BENCHMARK_MAIN();
