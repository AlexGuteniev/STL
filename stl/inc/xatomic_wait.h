// xatomic.h internal header

// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#pragma once
#ifndef _XATOMIC_WAIT_H
#define _XATOMIC_WAIT_H
#include <yvals.h>
#if _STL_COMPILER_PREPROCESSOR

#include <xatomic.h>

#pragma pack(push, _CRT_PACKING)
#pragma warning(push, _STL_WARNING_LEVEL)
#pragma warning(disable : _STL_DISABLED_WARNINGS)
_STL_DISABLE_CLANG_WARNINGS

enum _Atomic_spin_phase : unsigned long {
    _Atomic_wait_phase_mask            = 0x0000'00FF,
    _Atomic_spin_value_mask            = ~_Atomic_wait_phase_mask,
    _Atomic_spin_value_step            = _Atomic_wait_phase_mask + 1,
    _Atomic_wait_phase_init_spin_count = 0x0000'0000,
    _Atomic_wait_phase_spin            = 0x0000'0008,
    _Atomic_wait_phase_wait_locked     = 0x0000'0001,
    _Atomic_wait_phase_wait_none       = 0x0000'0002,
    _Atomic_wait_phase_wait_counter    = 0x0000'0004,
    _Atomic_wait_phase_yield           = 0x0000'0010,
    _Atomic_wait_phase_sleep           = 0x0000'0020,
    _Atomic_unwait_needed              = _Atomic_wait_phase_wait_locked,
};

struct _Atomic_wait_context_t {
    static constexpr unsigned long long _No_deadline = 0xFFFF'FFFF'FFFF'FFFF;
    unsigned long _Wait_phase_and_spin_count         = _Atomic_wait_phase_init_spin_count;
    unsigned long long _Deadline; // _No_deadline or GetTickCount64 plus duration
    unsigned long long _Counter; // For indirect waits - value of internal variable to wait against
};

_EXTERN_C
bool __stdcall __std_atomic_wait_direct(
    const void* _Storage, const void* _Comparand, const size_t _Size, _Atomic_wait_context_t& _Wait_context) noexcept;
void __stdcall __std_atomic_notify_one_direct(const void* _Storage) noexcept;
void __stdcall __std_atomic_notify_all_direct(const void* _Storage) noexcept;
void __stdcall __std_atomic_unwait_direct(const void* _Storage, _Atomic_wait_context_t& _Wait_context) noexcept;

bool __stdcall __std_atomic_wait_indirect(const void* _Storage, _Atomic_wait_context_t& _Wait_context) noexcept;
void __stdcall __std_atomic_notify_one_indirect(const void* _Storage) noexcept;
void __stdcall __std_atomic_notify_all_indirect(const void* _Storage) noexcept;
void __stdcall __std_atomic_unwait_indirect(const void* _Storage, _Atomic_wait_context_t& _Wait_context) noexcept;

_NODISCARD unsigned long __stdcall __std_atomic_get_spin_count(bool _Is_direct) noexcept;
_NODISCARD unsigned long long __cdecl __std_atomic_wait_get_current_time() noexcept;
_END_EXTERN_C


_NODISCARD inline bool _Atomic_wait_spin(unsigned long& _Wait_phase_and_spin_count, const bool _Is_direct) noexcept {
#ifdef _ATOMIC_WAIT_ON_ADDRESS_STATICALLY_AVAILABLE
    if (_Is_direct) {
        // WaitOnAddress spins by itself, but this is only helpful for direct waits,
        // since for indirect waits this will work only if notified.
        return false;
    }
#endif // _ATOMIC_WAIT_ON_ADDRESS_STATICALLY_AVAILABLE
    switch (_Wait_phase_and_spin_count & _Atomic_wait_phase_mask) {
    case _Atomic_wait_phase_init_spin_count:
        _Wait_phase_and_spin_count = _Atomic_wait_phase_spin | __std_atomic_get_spin_count(_Is_direct);
#if _HAS_CXX17
        [[fallthrough]];
#endif

    case _Atomic_wait_phase_spin:
        if ((_Wait_phase_and_spin_count & _Atomic_spin_value_mask) != 0) {
            _Wait_phase_and_spin_count -= _Atomic_spin_value_step;
            _YIELD_PROCESSOR();
            return true;
        }
        _Wait_phase_and_spin_count = _Atomic_wait_phase_wait_none;
        break;
    }
    return false;
}

_NODISCARD inline unsigned long long _Atomic_wait_get_deadline(const unsigned long _Timeout) noexcept {
    if (_Timeout == 0xFFFF'FFFF) {
        return _Atomic_wait_context_t::_No_deadline;
    } else {
        return __std_atomic_wait_get_current_time() + _Timeout;
    }
}

_STL_RESTORE_CLANG_WARNINGS
#pragma warning(pop)
#pragma pack(pop)
#endif // _STL_COMPILER_PREPROCESSOR
#endif // _XATOMIC_WAIT_H
