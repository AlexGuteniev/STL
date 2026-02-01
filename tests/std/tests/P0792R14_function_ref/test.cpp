// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <cassert>
#include <functional>

using namespace std;

int fn(const char* a, int b) {
    return *a - '0' + b;
}

int __stdcall fn_cc(const char* a, short b) {
    return *a - '0' + b + 1;
}

int fn_nx(int a, int b) noexcept {
    return a + b;
}

int __stdcall fn_cc_nx(int a, int b) noexcept {
    return a - b;
}

struct global_object {
    unsigned m = 0x55;

    unsigned operator()(const unsigned i) const {
        return m ^ i;
    }

    unsigned fn(const unsigned a, const unsigned b) const {
        return m ^ a ^ b;
    }

} constexpr glob;

#ifdef __cpp_noexcept_function_type
struct global_object_nx {
    unsigned m = 0x55;

    unsigned operator()(const unsigned i) const noexcept {
        return m ^ i;
    }

    unsigned fn(const unsigned a, const unsigned b) const noexcept {
        return m ^ a ^ b;
    }

} constexpr glob_nx;
#endif // __cpp_noexcept_function_type

static_assert(sizeof(function_ref<void()>) == sizeof(void*) + sizeof(void (*)()));

void constructors() {
    static_assert(!is_default_constructible_v<function_ref<void()>>);

    // 1. From function pointer
    {
        function_ref<int(const char*, short) const> fn1 = ::fn;
        assert(fn1("1", 2) == 3);

        function_ref<int(const char*, short)> fn2 = ::fn_cc;
        assert(fn2("1", 2) == 4);

        function_ref fn3 = ::fn;
        assert(fn1("3", 2) == 5);

#ifdef __cpp_noexcept_function_type
        function_ref<int(int, int) noexcept> fn4 = ::fn_nx;
        assert(fn4(4, 3) == 7);

        function_ref<int(int, int) const noexcept> fn5 = ::fn_cc_nx;
        assert(fn5(3, 4) == -1);
#endif // __cpp_noexcept_function_type
    }
    // 2. From object
    {
        int i = 1;
        int j = 2;
        int k = 3;

        auto l1 = [i, &j, &k] { k = i + j + k; };

        function_ref<void()> fn1 = l1;
        assert(k == 3);
        fn1();
        assert(k == 6);

#ifdef __cpp_noexcept_function_type
        auto l2 = [i](int j) noexcept { return i + j; };

        function_ref<int(int) noexcept> fn2 = l2;
        assert(l2(4) == 5);
#endif // __cpp_noexcept_function_type
    }
    // 3. From contant
    {
        function_ref<int(const char*, short)> fn1 = constant_arg<::fn>;
        assert(fn1("2", 5) == 7);

        function_ref<unsigned(unsigned)> fn2 = constant_arg<glob>;
        assert(fn2(0x33) == 0x66);

        // [func.wrap.ref.deduct]/2 through 4
        function_ref fn3 = constant_arg<::fn>;
        assert(fn3("3", 3) == 6);

#ifdef __cpp_noexcept_function_type
        function_ref<unsigned(unsigned) noexcept> fn4 = constant_arg<glob_nx>;
        assert(fn4(0x66) == 0x33);
#endif // __cpp_noexcept_function_type
    }
    // 4. From consant and obj
    {
        function_ref<int(short)> fn1(constant_arg<::fn>, "6");
        assert(fn1(3) == 9);

        unsigned i = 0xAA;
        function_ref<unsigned() const> fn2(constant_arg<glob>, i);
        assert(fn2() == 0xFF);

        function_ref<unsigned(unsigned, unsigned)> fn3(constant_arg<&global_object::fn>, glob);
        assert(fn3(0x22, 0x44) == 0x33);

        function_ref fn4(constant_arg<&global_object::fn>, glob);
        assert(fn4(0x22, 0x88) == 0xFF);

        static_assert(&global_object::m);

        function_ref fn5(constant_arg<&global_object::m>, glob);
        assert(fn5() == 0x55);

        function_ref fn6(constant_arg<::fn>, "3");
        assert(fn6(5) == 8);

#ifdef __cpp_noexcept_function_type
        int k = 2;
        function_ref<int(int)> fn7(constant_arg<::fn_nx>, k);
        assert(fn7(2) == 4);

        unsigned t = 0xEE;
        function_ref<unsigned() const> fn8(constant_arg<glob_nx>, t);
        assert(fn8() == 0xBB);

        function_ref<unsigned(unsigned, unsigned)> fn9(constant_arg<&global_object_nx::fn>, glob_nx);
        assert(fn9(0xEE, 0xFF) == 0x44);
#endif // __cpp_noexcept_function_type
    }
    // 5. From consant and ptr
    {
        function_ref<int(short)> fn1(constant_arg<::fn>, static_cast<const char*>("3"));
        assert(fn1(7) == 10);

        function_ref<unsigned(unsigned, unsigned)> fn2(constant_arg<&global_object::fn>, &glob);
        assert(fn2(0x44, 0x88) == 0x99);
    }
}

void copy_and_assign() {
    static_assert(is_assignable_v<function_ref<void()>, function_ref<void()>>);

    static_assert(!is_assignable_v<function_ref<void()>, nullptr_t>);
    static_assert(!is_assignable_v<function_ref<void()>, move_only_function<void()>>);
    static_assert(!is_assignable_v<function_ref<void()>, function_ref<void() const>>);

    auto a_plus_b = [](int a, int b) noexcept { return a + b; };
    move_only_function<int(int, int) const> fn{a_plus_b};

    auto placeholder = [](int, int) -> int { abort(); };

    function_ref<int(int, int) const> ref         = fn;
    function_ref<int(int, int) const> ref_copy    = ref;
    function_ref<int(int, int) const> ref_assign  = placeholder;
    function_ref<int(int, int)> ref_copy_dif_sign = ref;

    ref_assign = ref;

    assert(ref(2, 3) == 5);
    assert(ref_copy(2, 3) == 5);
    assert(ref_assign(2, 3) == 5);
    assert(ref_copy_dif_sign(2, 3) == 5);

    auto a_minus_b = [](int a, int b) { return a - b; };
    ref            = function_ref<int(int, int) const>(a_minus_b);

    assert(ref(2, 3) == -1);
    assert(ref_copy(2, 3) == 5);
    assert(ref_assign(2, 3) == 5);

    // Implementing LWG-4264 speculatively; otherwise -1 expected
    assert(ref_copy_dif_sign(2, 3) == 5);

#ifdef __cpp_noexcept_function_type
    auto a_by_b = [](int a, int b) noexcept { return a * b; };

    function_ref<int(int, int) const noexcept> ref_nx(a_by_b);
    function_ref<int(int, int)> ref_nx_copy(ref_nx);
    ref_nx = function_ref<int(int, int) const noexcept>(a_plus_b);

    // Implementing LWG-4264 speculatively; otherwise 5 expected
    assert(ref_nx_copy(2, 3) == 6);
#endif // __cpp_noexcept_function_type
}

int main() {
    constructors();
    copy_and_assign();
}
