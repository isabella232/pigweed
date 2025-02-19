# Copyright 2019 The Pigweed Authors
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
    "@com_github_bazelbuild_buildtools//buildifier:def.bzl",
    "buildifier",
    "buildifier_test",
)

licenses(["notice"])  # Apache License 2.0

exports_files(
    ["tsconfig.json"],
    visibility = ["//:__subpackages__"],
)

# Fix all Bazel relevant files.
buildifier(
    name = "buildifier",
    # Ignore gn and CIPD outputs in formatting.
    # NOTE: These globs are not Bazel native and are passed directly
    # through to the buildifier binary.
    # TODO: Remove these globs when
    # https://github.com/bazelbuild/buildtools/issues/623 is addressed.
    exclude_patterns = [
        "./.environment/**/*",
        "./.presubmit/**/*",
        "./.out/**/*",
    ],
)

# Test to ensure all Bazel build files are properly formatted.
buildifier_test(
    name = "buildifier_test",
    srcs = glob(
        [
            "**/*.bazel",
            "**/*.bzl",
            "**/*.oss",
            "**/*.sky",
            "**/BUILD",
        ],
        # Node modules do not play nice with buildifier. Exclude these
        # generated Bazel files from format testing.
        exclude = ["**/node_modules/**/*"],
    ) + ["WORKSPACE"],
    diff_command = "diff -u",
    mode = "diff",
    verbose = True,
)
