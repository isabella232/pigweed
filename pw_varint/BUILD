# Copyright 2020 The Pigweed Authors
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
    "pw_cc_test",
)

package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache License 2.0

pw_cc_library(
    name = "pw_varint",
    srcs = [
        "varint.cc",
    ],
    hdrs = [
        "public/pw_varint/varint.h",
    ],
    includes = ["public"],
    deps = [
        "//pw_polyfill",
        "//pw_preprocessor",
        "//pw_span",
    ],
)

pw_cc_test(
    name = "varint_test",
    srcs = [
        "varint_test.cc",
        "varint_test_c.c",
    ],
    deps = [
        ":pw_varint",
        "//pw_unit_test",
    ],
)
