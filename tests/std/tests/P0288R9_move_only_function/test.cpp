// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <cassert>
#include <cstdlib>
#include <functional>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

using namespace std;

constexpr auto large_function_size = 100;

struct pass_this_by_ref {
    int v;

    pass_this_by_ref(int v_) : v(v_) {}

    pass_this_by_ref(const pass_this_by_ref&) {
        abort();
    }
};

struct counter {
    static int inst;
    static int copies;
    static int moves;

    counter() {
        ++inst;
    }

    counter(const counter&) {
        ++inst;
        ++copies;
    }

    counter(counter&&) noexcept {
        ++inst;
        ++moves;
    }

    ~counter() {
        --inst;
    }
};

int counter::inst   = 0;
int counter::copies = 0;
int counter::moves  = 0;

struct small_callable : counter {
    int operator()(int a, pass_this_by_ref& b) {
        assert(a == 23);
        assert(b.v == 63);
        return 38;
    }

    void* operator new(size_t)  = delete;
    void operator delete(void*) = delete;

    small_callable() = default;

    small_callable(const small_callable&) = default;

    small_callable(small_callable&&) noexcept = default;
};

struct large_callable : counter {
    char data[large_function_size] = {};

    int operator()(int a, pass_this_by_ref& b) {
        assert(a == 23);
        assert(b.v == 63);
        return 39;
    }

    void* operator new(size_t)  = delete;
    void operator delete(void*) = delete;

    large_callable() = default;

    large_callable(const large_callable&) = default;

    large_callable(large_callable&&) noexcept = default;
};

struct odd_cc_callable : counter {
    int __fastcall operator()(int a, pass_this_by_ref& b) {
        assert(a == 23);
        assert(b.v == 63);
        return 40;
    }

    odd_cc_callable() = default;

    odd_cc_callable(const odd_cc_callable&)     = default;
    odd_cc_callable(odd_cc_callable&&) noexcept = default;
};

struct large_implicit_ptr_callable : counter {
    char data[large_function_size] = {};

    using pfn = int (*)(int a, pass_this_by_ref& b);

    operator pfn() {
        return [](int a, pass_this_by_ref& b) {
            assert(a == 23);
            assert(b.v == 63);
            return 41;
        };
    }

    large_implicit_ptr_callable() = default;

    large_implicit_ptr_callable(const large_implicit_ptr_callable&)     = default;
    large_implicit_ptr_callable(large_implicit_ptr_callable&&) noexcept = default;
};

int __fastcall plain_callable(int a, pass_this_by_ref& b) {
    assert(a == 23);
    assert(b.v == 63);
    return 42;
}

template <class Function, bool Copyable, class F, class... Args>
void test_construct_impl(int expect, Args... args) {
    {
        pass_this_by_ref x{63};

        Function constructed_directly(F{args...});

        assert(constructed_directly(23, x) == expect);

        assert(constructed_directly);
        assert(constructed_directly != nullptr);

        Function move_constructed = move(constructed_directly);

        assert(move_constructed(23, x) == expect);

        if constexpr (is_class_v<F>) {
            assert(counter::copies == 0);
        }

        F v{args...};
        Function constructed_lvalue(v);
        if constexpr (is_class_v<F>) {
            assert(counter::copies == 1);
            counter::copies = 0;
        }

        if constexpr (is_class_v<F>) {
            counter::copies = 0;
            counter::moves  = 0;
        }
        Function constructed_in_place(in_place_type<F>, args...);
        assert(constructed_in_place(23, x) == expect);
        if constexpr (is_class_v<F>) {
            assert(counter::copies == 0);
            assert(counter::moves == 0);
        }

        if constexpr (Copyable) {
            Function copy(move_constructed);
            assert(copy(23, x) == expect);
            assert(move_constructed(23, x) == expect);
            if constexpr (is_class_v<F>) {
                assert(counter::copies == 1);
                assert(counter::moves == 0);
                counter::copies = 0;
            }
        } else {
            static_assert(!is_copy_constructible_v<Function>);
        }
    }

    if constexpr (is_class_v<F>) {
        assert(counter::inst == 0);
    }
}

template <class Function>
void test_move_assign() {
    pass_this_by_ref x{63};

    {
        Function f1{small_callable{}};
        Function f2{large_callable{}};
        f2 = move(f1);
        assert(f2(23, x) == 38);
        f1 = large_callable{};
        assert(f1(23, x) == 39);
    }

    {
        Function f1{large_callable{}};
        Function f2{small_callable{}};
        f2 = move(f1);
        assert(f2(23, x) == 39);
        f1 = small_callable{};
        assert(f1(23, x) == 38);
    }

    {
        Function f1{small_callable{}};
        Function f2{odd_cc_callable{}};
        f2 = move(f1);
        assert(f2(23, x) == 38);
        f1 = odd_cc_callable{};
        assert(f1(23, x) == 40);
    }

    {
        Function f1{large_callable{}};
        Function f2{large_implicit_ptr_callable{}};
        f2 = move(f1);
        assert(f2(23, x) == 39);
        f1 = large_implicit_ptr_callable{};
        assert(f1(23, x) == 41);
    }

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"
#endif // __clang__
#pragma warning(push)
#pragma warning(disable : 26800) // use a moved-from object
    {
        Function f1{small_callable{}};
        Function f2{large_callable{}};
        f1 = move(f1); // deliberate self-move as a test case
        assert(f1(23, x) == 38);
        f2 = move(f2); // deliberate self-move as a test case
        assert(f2(23, x) == 39);
    }
#pragma warning(pop)
#ifdef __clang__
#pragma clang diagnostic pop
#endif // __clang__
}

template <class Function>
void test_copy_assign() {
    pass_this_by_ref x{63};

    {
        Function f1{small_callable{}};
        Function f2{large_callable{}};
        f2 = f1;
        assert(f2(23, x) == 38);
        f1 = large_callable{};
        assert(f1(23, x) == 39);
    }

    {
        Function f1{large_callable{}};
        Function f2{small_callable{}};
        f2 = f1;
        assert(f2(23, x) == 39);
        f1 = small_callable{};
        assert(f1(23, x) == 38);
    }

    {
        Function f1{small_callable{}};
        Function f2{odd_cc_callable{}};
        f2 = f1;
        assert(f2(23, x) == 38);
        f1 = odd_cc_callable{};
        assert(f1(23, x) == 40);
    }

    {
        Function f1{large_callable{}};
        Function f2{large_implicit_ptr_callable{}};
        f2 = f1;
        assert(f2(23, x) == 39);
        f1 = large_implicit_ptr_callable{};
        assert(f1(23, x) == 41);
    }

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif // __clang__
    {
        Function f1{small_callable{}};
        Function f2{large_callable{}};
        f1 = f1; // deliberate self-assign as a test case
        assert(f1(23, x) == 38);
        f2 = f2; // deliberate self-assign as a test case
        assert(f2(23, x) == 39);
    }
#ifdef __clang__
#pragma clang diagnostic pop
#endif // __clang__
}

template <class Function, class OtherFunction, bool Copyable, bool OtherCopyable>
void test_null_assign() {
    {
        Function f1{small_callable{}};
        Function f2{large_callable{}};
        Function f3{small_callable{}};
        Function f4{large_callable{}};
        Function f5{nullptr};
        Function f6{Function{}};
        assert(f1);
        assert(f2);
        assert(f3);
        assert(f4);
        assert(!f5);
        assert(!f6);

        if constexpr (!Copyable || OtherCopyable) {
            Function f7{OtherFunction{}};
            assert(!f7);
        }

        f1 = nullptr;
        f2 = nullptr;
        f3 = Function{nullptr};
        f4 = Function{nullptr};
        assert(!f1);
        assert(!f2);
        assert(!f3);
        assert(!f4);
    }
}

template <class Function>
void test_swap() {
    pass_this_by_ref x{63};

    {
        Function f1{small_callable{}};
        Function f2{large_callable{}};
        swap(f1, f2);
        assert(f2(23, x) == 38);
        assert(f1(23, x) == 39);
    }

    {
        Function f1{small_callable{}};
        Function f2{odd_cc_callable{}};
        f1.swap(f2);
        assert(f2(23, x) == 38);
        assert(f1(23, x) == 40);
    }

    {
        Function f1{large_callable{}};
        Function f2{large_implicit_ptr_callable{}};
        f2.swap(f1);
        assert(f2(23, x) == 39);
        assert(f1(23, x) == 41);
    }

    {
        Function f1{small_callable{}};
        Function f2{large_callable{}};
        swap(f1, f1);
        f2.swap(f2);
        assert(f1(23, x) == 38);
        assert(f2(23, x) == 39);
    }
}

template <class Function>
void test_empty() {
    Function no_callable;
    assert(!no_callable);
    assert(no_callable == nullptr);
    assert(nullptr == no_callable);

    Function no_callable_moved = move(no_callable);
#pragma warning(push)
#pragma warning(disable : 26800) // use a moved-from object
    assert(!no_callable);
    assert(no_callable == nullptr);
#pragma warning(pop)
    assert(!no_callable_moved);
    assert(no_callable_moved == nullptr);
}

template <template <class...> class Function>
void test_ptr() {
    struct s_t {
        int f(int p) {
            return p + 2;
        }

        int j = 6;

        static int g(int z) {
            return z - 3;
        }
    };

    Function<int(s_t*, int)> mem_fun_ptr(&s_t::f);
    Function<int(s_t*)> mem_ptr(&s_t::j);
    Function<int(int)> fun_ptr(&s_t::g);

    s_t s;
    assert(mem_fun_ptr);
    assert(mem_fun_ptr(&s, 3) == 5);
    assert(mem_ptr);
    assert(mem_ptr(&s) == 6);
    assert(fun_ptr);
    assert(fun_ptr(34) == 31);

    Function<int(s_t*, int)> mem_fun_ptr_n(static_cast<decltype(&s_t::f)>(nullptr));
    Function<int(s_t*)> mem_ptr_n(static_cast<decltype(&s_t::j)>(nullptr));
    Function<int(int)> fun_ptr_n(static_cast<decltype(&s_t::g)>(nullptr));

    assert(!mem_fun_ptr_n);
    assert(!mem_ptr_n);
    assert(!fun_ptr_n);
}

template <template <class...> class Function>
void test_inner() {
    Function<short(long, long)> f1(nullptr);
    Function<int(int, int)> f2 = move(f1);
    assert(!f2);
#pragma warning(push)
#pragma warning(disable : 26800) // use a moved-from object
    f2 = move(f1);
    assert(!f1);
#pragma warning(pop)
}

struct noncopyablle_base {
    noncopyablle_base()                                    = default;
    noncopyablle_base(const noncopyablle_base&)            = delete;
    noncopyablle_base& operator=(const noncopyablle_base&) = delete;
};

struct copyable_base {};

template <template <class...> class Function, bool Copyable>
void test_inplace_list() {
    struct in_place_list_constructible : conditional_t<Copyable, copyable_base, noncopyablle_base> {
        in_place_list_constructible(initializer_list<int> li) {
            int x = 0;
            for (int i : li) {
                ++x;
                assert(x == i);
            }
        }

        in_place_list_constructible(initializer_list<int> li, const char*) {
            int x = 0;
            for (int i : li) {
                --x;
                assert(x == i);
            }
        }

        int operator()(int i) {
            return i - 1;
        }
    };

    Function<int(int)> f1(in_place_type<in_place_list_constructible>, {1, 2, 3, 4, 5});
    assert(f1(5) == 4);

    Function<int(int)> f2(in_place_type<in_place_list_constructible>, {-1, -2, -3, -4, -5}, "fox");
    assert(f2(8) == 7);
}


template <bool Nx>
struct test_noexcept_t {
    int operator()() noexcept(Nx) {
        return 888;
    }
};

template <template <class...> class Function>
void test_noexcept() {
    using f_x  = Function<int()>;
    using f_nx = Function<int() noexcept>;

    static_assert(!noexcept(declval<f_x>()()));
#ifdef __cpp_noexcept_function_type
    static_assert(noexcept(declval<f_nx>()()));
#else // ^^^ defined(__cpp_noexcept_function_type) / !defined(__cpp_noexcept_function_type) vvv
    static_assert(!noexcept(declval<f_nx>()()));
#endif // ^^^ !defined(__cpp_noexcept_function_type) ^^^

    static_assert(is_constructible_v<f_x, test_noexcept_t<false>>);
    assert(f_x(test_noexcept_t<false>{})() == 888);

    static_assert(is_constructible_v<f_x, test_noexcept_t<true>>);
    assert(f_x(test_noexcept_t<true>{})() == 888);

#ifdef __cpp_noexcept_function_type
    static_assert(!is_constructible_v<f_nx, test_noexcept_t<false>>);
#else // ^^^ defined(__cpp_noexcept_function_type) / !defined(__cpp_noexcept_function_type) vvv
    static_assert(is_constructible_v<f_nx, test_noexcept_t<false>>);
    assert(f_nx(test_noexcept_t<false>{})() == 888);
#endif // ^^^ !defined(__cpp_noexcept_function_type) ^^^

    static_assert(is_constructible_v<f_nx, test_noexcept_t<true>>);
    assert(f_nx(test_noexcept_t<true>{})() == 888);
}

template <bool>
struct test_const_t {
    int operator()() {
        return 456;
    }
};

template <>
struct test_const_t<true> {
    int operator()() const {
        return 456;
    }
};

template <template <class...> class Function>
void test_const() {
    using f_c  = Function<int() const>;
    using f_nc = Function<int()>;

    static_assert(is_constructible_v<f_nc, test_const_t<false>>);
    f_nc f1(test_const_t<false>{});
    assert(f1() == 456);

    static_assert(is_constructible_v<f_nc, test_const_t<true>>);
    f_nc f2(test_const_t<true>{});
    assert(f2() == 456);

    static_assert(!is_constructible_v<f_c, test_const_t<false>>);

    static_assert(is_constructible_v<f_c, test_const_t<true>>);
    f_c f3(test_const_t<true>{});
    assert(f3() == 456);
    const f_c f4(test_const_t<true>{});
    assert(f4() == 456);
}

template <template <class...> class Function>
void test_qual() {
    Function<int(int)> f1([](auto i) { return i + 1; });
    assert(f1(1) == 2);
    Function<int(int) &> f2([](auto i) { return i + 1; });
    assert(f2(2) == 3);
    Function<int(int) &&> f3([](auto i) { return i + 1; });
    assert(move(f3)(3) == 4);

    Function<int(int) const> f1c([](auto i) { return i + 1; });
    assert(f1c(4) == 5);
    Function<int(int) const&> f2c([](auto i) { return i + 1; });
    assert(f2c(5) == 6);
    Function<int(int) const&&> f3c([](auto i) { return i + 1; });
    assert(move(f3c)(6) == 7);

    Function<int(int) noexcept> f1_nx([](auto i) noexcept { return i + 1; });
    assert(f1_nx(1) == 2);
    Function<int(int) & noexcept> f2_nx([](auto i) noexcept { return i + 1; });
    assert(f2_nx(2) == 3);
    Function<int(int) && noexcept> f3_nx([](auto i) noexcept { return i + 1; });
    assert(move(f3_nx)(3) == 4);

    Function<int(int) const noexcept> f1c_nx([](auto i) noexcept { return i + 1; });
    assert(f1c_nx(4) == 5);
    Function<int(int) const & noexcept> f2c_nx([](auto i) noexcept { return i + 1; });
    assert(f2c_nx(5) == 6);
    Function<int(int) const && noexcept> f3c_nx([](auto i) noexcept { return i + 1; });
    assert(move(f3c_nx)(6) == 7);
}

template <template <class...> class Function>
void test_result() {
    static_assert(is_same_v<typename Function<void()>::result_type, void>);
    static_assert(is_same_v<typename Function<short(long&) &>::result_type, short>);
    static_assert(is_same_v<typename Function<int(char*) &&>::result_type, int>);
    static_assert(is_same_v<typename Function<void() const>::result_type, void>);
    static_assert(is_same_v<typename Function<short(long&) const&>::result_type, short>);
    static_assert(is_same_v<typename Function<int(char*) const&&>::result_type, int>);

#ifdef __cpp_noexcept_function_type
    static_assert(is_same_v<typename Function<void() noexcept>::result_type, void>);
    static_assert(is_same_v<typename Function<short(long&) & noexcept>::result_type, short>);
    static_assert(is_same_v<typename Function<int(char*) && noexcept>::result_type, int>);
    static_assert(is_same_v<typename Function<void() const noexcept>::result_type, void>);
    static_assert(is_same_v<typename Function<short(long&) const & noexcept>::result_type, short>);
    static_assert(is_same_v<typename Function<int(char*) const && noexcept>::result_type, int>);
#endif // ^^^ defined(__cpp_noexcept_function_type) ^^^
}

bool fail_allocations = false;

#pragma warning(push)
#pragma warning(disable : 28251) // Inconsistent annotation for 'new': this instance has no annotations.
void* operator new(size_t size) {
#pragma warning(pop)
    if (fail_allocations) {
        throw bad_alloc{};
    }
    void* result = size > 0 ? malloc(size) : malloc(1);
    if (!result) {
        throw bad_alloc{};
    }
    return result;
}

void operator delete(void* p) noexcept {
    free(p);
}

template <template <class...> class Function, bool Copyable>
void test_except() {
    struct throwing {
        throwing() = default;
        throwing(const throwing&)
        {
            throw runtime_error{"boo"};
        }

        void operator()() {}
    };

    static_assert(!is_nothrow_move_constructible_v<throwing>);

    struct not_throwing {
        not_throwing() = default;
        not_throwing(const not_throwing&) {}
        void operator()() {}
    };

    static_assert(!is_nothrow_move_constructible_v<not_throwing>); // avoid small function optimization

    try {
        Function<void()> f{throwing{}};
        assert(false); // unreachable
    } catch (const runtime_error&) {
    }

    try {
        fail_allocations = true;
        Function<void()> f{not_throwing{}};
        assert(false); // unreachable
    } catch (const bad_alloc&) {
        fail_allocations = false;
    }
}

template <template <class...> class Function, template <class...> class OtherFunction, bool Copyable,
    bool OtherCopyable>
void test() {
    using test_function_t = Function<int(int, pass_this_by_ref&)>;
    using test_other_function_t = OtherFunction<int(int, pass_this_by_ref&)>;

     test_construct_impl<test_function_t, Copyable, small_callable>(38);
     test_construct_impl<test_function_t, Copyable, large_callable>(39);
     test_construct_impl<test_function_t, Copyable, odd_cc_callable>(40);
     test_construct_impl<test_function_t, Copyable, large_implicit_ptr_callable>(41);
     test_construct_impl<test_function_t, Copyable, decltype(&plain_callable)>(42, plain_callable);

     test_move_assign<test_function_t>();
     if constexpr (Copyable) {
         test_copy_assign<test_function_t>();
     }
     test_null_assign<test_function_t, test_other_function_t, Copyable, OtherCopyable>();
     test_swap<test_function_t>();
     test_empty<test_function_t>();

     test_ptr<Function>();
     test_inner<Function>();
     test_inplace_list<Function, Copyable>();
     test_noexcept<Function>();
     test_const<Function>();
     test_qual<Function>();
     test_result<Function>();
     test_except<Function, Copyable>();
}

int main() {
#if _HAS_CXX26
    test<move_only_function, copyable_function, false, true>();
    test<copyable_function, move_only_function, true, false>();
#else // ^^^ _HAS_CXX26 / !_HAS_CXX26 vvv
    test<move_only_function, move_only_function, false, false>();
#endif // ^^^ !_HAS_CXX26 ^^^
}
