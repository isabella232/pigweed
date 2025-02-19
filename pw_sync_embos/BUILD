# Copyright 2021 The Pigweed Authors
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

load(
    "//pw_build:pigweed.bzl",
    "pw_cc_library",
)

package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache License 2.0

pw_cc_library(
    name = "binary_semaphore_headers",
    hdrs = [
        "public/pw_sync_embos/binary_semaphore_inline.h",
        "public/pw_sync_embos/binary_semaphore_native.h",
        "public_overrides/pw_sync_backend/binary_semaphore_inline.h",
        "public_overrides/pw_sync_backend/binary_semaphore_native.h",
    ],
    includes = [
        "public",
        "public_overrides",
    ],
    target_compatible_with = [
        "//pw_build/constraints/rtos:embos",
    ],
    deps = [
        # TODO(pwbug/317): This should depend on embOS but our third parties
        # currently do not have Bazel support.
        "//pw_chrono:system_clock",
        "//pw_chrono_embos:system_clock_headers",
    ],
)

pw_cc_library(
    name = "binary_semaphore",
    srcs = [
        "binary_semaphore.cc",
    ],
    target_compatible_with = [
        "//pw_build/constraints/rtos:embos",
    ],
    deps = [
        ":binary_semaphore_headers",
        "//pw_interrupt:context",
        "//pw_sync:binary_semaphore_facade",
    ],
)

pw_cc_library(
    name = "counting_semaphore_headers",
    hdrs = [
        "public/pw_sync_embos/counting_semaphore_inline.h",
        "public/pw_sync_embos/counting_semaphore_native.h",
        "public_overrides/pw_sync_backend/counting_semaphore_inline.h",
        "public_overrides/pw_sync_backend/counting_semaphore_native.h",
    ],
    includes = [
        "public",
        "public_overrides",
    ],
    target_compatible_with = [
        "//pw_build/constraints/rtos:embos",
    ],
    deps = [
        # TODO(pwbug/317): This should depend on embOS but our third parties
        # currently do not have Bazel support.
        "//pw_chrono:system_clock",
        "//pw_chrono_embos:system_clock_headers",
    ],
)

pw_cc_library(
    name = "counting_semaphore",
    srcs = [
        "counting_semaphore.cc",
    ],
    target_compatible_with = [
        "//pw_build/constraints/rtos:embos",
    ],
    deps = [
        ":counting_semaphore_headers",
        "//pw_interrupt:context",
        "//pw_sync:counting_semaphore_facade",
    ],
)

pw_cc_library(
    name = "mutex_headers",
    hdrs = [
        "public/pw_sync_embos/mutex_inline.h",
        "public/pw_sync_embos/mutex_native.h",
        "public_overrides/pw_sync_backend/mutex_inline.h",
        "public_overrides/pw_sync_backend/mutex_native.h",
    ],
    includes = [
        "public",
        "public_overrides",
    ],
    target_compatible_with = [
        "//pw_build/constraints/rtos:embos",
    ],
    deps = [
        # TODO(pwbug/317): This should depend on embOS but our third parties
        # currently do not have Bazel support.
        "//pw_sync:mutex_facade",
    ],
)

pw_cc_library(
    name = "mutex",
    target_compatible_with = [
        "//pw_build/constraints/rtos:embos",
    ],
    deps = [
        ":mutex_headers",
        "//pw_sync:mutex_facade",
    ],
)

pw_cc_library(
    name = "timed_mutex_headers",
    hdrs = [
        "public/pw_sync_embos/timed_mutex_inline.h",
        "public_overrides/pw_sync_backend/timed_mutex_inline.h",
    ],
    includes = [
        "public",
        "public_overrides",
    ],
    target_compatible_with = [
        "//pw_build/constraints/rtos:embos",
    ],
    deps = [
        # TODO(pwbug/317): This should depend on embOS but our third parties
        # currently do not have Bazel support.
        "//pw_chrono:system_clock",
        "//pw_sync:timed_mutex_facade",
    ],
)

pw_cc_library(
    name = "timed_mutex",
    srcs = [
        "timed_mutex.cc",
    ],
    target_compatible_with = [
        "//pw_build/constraints/rtos:embos",
    ],
    deps = [
        ":timed_mutex_headers",
        "//pw_chrono_embos:system_clock_headers",
        "//pw_interrupt:context",
        "//pw_sync:timed_mutex_facade",
    ],
)

pw_cc_library(
    name = "interrupt_spin_lock_headers",
    hdrs = [
        "public/pw_sync_embos/interrupt_spin_lock_inline.h",
        "public/pw_sync_embos/interrupt_spin_lock_native.h",
        "public_overrides/pw_sync_backend/interrupt_spin_lock_inline.h",
        "public_overrides/pw_sync_backend/interrupt_spin_lock_native.h",
    ],
    includes = [
        "public",
        "public_overrides",
    ],
    target_compatible_with = [
        "//pw_build/constraints/rtos:embos",
    ],
    # TODO(pwbug/317): This should depend on embOS but our third parties
    # currently do not have Bazel support.
)

pw_cc_library(
    name = "interrupt_spin_lock",
    srcs = [
        "interrupt_spin_lock.cc",
    ],
    target_compatible_with = [
        "//pw_build/constraints/rtos:embos",
    ],
    deps = [
        ":interrupt_spin_lock_headers",
        "//pw_sync:interrupt_spin_lock_facade",
    ],
)
