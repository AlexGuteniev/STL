// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// support for <execution>

#include <internal_shared.h>
#include <thread>
#include <xatomic_wait.h>

namespace {
    unsigned char _Atomic_load_uchar(const volatile unsigned char* _Ptr) noexcept {
        // atomic load of unsigned char, copied from <atomic> except ARM and ARM64 bits
        unsigned char _Value;
#if defined(_M_IX86) || defined(_M_X64) || defined(_M_ARM) || defined(_M_ARM64)
        _Value = __iso_volatile_load8(reinterpret_cast<const volatile char*>(_Ptr));
        _ReadWriteBarrier();
#else
#error Unsupported architecture
#endif
        return _Value;
    }
} // unnamed namespace

extern "C" {

// TRANSITION, ABI
_NODISCARD unsigned int __stdcall __std_parallel_algorithms_hw_threads() noexcept {
    return _STD thread::hardware_concurrency();
}

_NODISCARD PTP_WORK __stdcall __std_create_threadpool_work(
    PTP_WORK_CALLBACK _Callback, void* _Context, PTP_CALLBACK_ENVIRON _Callback_environ) noexcept {
    return CreateThreadpoolWork(_Callback, _Context, _Callback_environ);
}

void __stdcall __std_submit_threadpool_work(PTP_WORK _Work) noexcept {
    SubmitThreadpoolWork(_Work);
}

void __stdcall __std_bulk_submit_threadpool_work(PTP_WORK _Work, const size_t _Submissions) noexcept {
    for (size_t _Idx = 0; _Idx < _Submissions; ++_Idx) {
        SubmitThreadpoolWork(_Work);
    }
}

void __stdcall __std_close_threadpool_work(PTP_WORK _Work) noexcept {
    CloseThreadpoolWork(_Work);
}

void __stdcall __std_wait_for_threadpool_work_callbacks(PTP_WORK _Work, BOOL _Cancel) noexcept {
    WaitForThreadpoolWorkCallbacks(_Work, _Cancel);
}

void __stdcall __std_execution_wait_on_uchar(const volatile unsigned char* _Address, unsigned char _Compare) noexcept {
    auto _Address_adjusted_cv = const_cast<const unsigned char*>(_Address);
    for (;;) {
        if (_Atomic_load_uchar(_Address) != _Compare) {
            return;
        }
        _Atomic_wait_result _Result =
            __std_atomic_wait_direct(_Address_adjusted_cv, &_Compare, 1, _Atomic_wait_no_deadline);
#if _STL_WIN32_WINNT < _WIN32_WINNT_WIN8
        if (_Result == _Atomic_wait_fallback) {
            for (;;) {
                if (_Atomic_load_uchar(_Address) != _Compare) {
                    __std_atomic_wait_fallback_uninit(_Address_adjusted_cv);
                    return;
                }
                __std_atomic_wait_fallback(_Address_adjusted_cv, _Atomic_wait_no_deadline);
            }
        }
#else // ^^^ _STL_WIN32_WINNT < _WIN32_WINNT_WIN8 / _STL_WIN32_WINNT >= _WIN32_WINNT_WIN8 vvv
        (void) _Result;
#endif // ^^^ _STL_WIN32_WINNT >= _WIN32_WINNT_WIN8 ^^^
    }
}

void __stdcall __std_execution_wake_by_address_all(const volatile void* _Address) noexcept {
    __std_atomic_notify_all_direct(const_cast<const void*>(_Address));
}

} // extern "C"
