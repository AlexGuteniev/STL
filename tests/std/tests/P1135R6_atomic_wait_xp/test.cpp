// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "test_atomic_wait.hpp"

int main() {
    __std_atomic_set_api_level(0x0502);
    test_atomic_wait();
}
