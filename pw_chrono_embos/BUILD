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
    name = "system_clock_headers",
    hdrs = [
        "public/pw_chrono_embos/config.h",
        "public/pw_chrono_embos/system_clock_config.h",
        "public/pw_chrono_embos/system_clock_constants.h",
        "public_overrides/pw_chrono_backend/system_clock_config.h",
    ],
    includes = [
        "public",
        "public_overrides",
    ],
    target_compatible_with = [
        "//pw_build/constraints/rtos:embos",
    ],
    deps = [
        "//pw_chrono:epoch",
    ],
)

pw_cc_library(
    name = "system_clock",
    srcs = [
        "system_clock.cc",
    ],
    target_compatible_with = [
        "//pw_build/constraints/rtos:embos",
    ],
    deps = [
        ":system_clock_headers",
        "//pw_chrono:system_clock_facade",
    ],
)
