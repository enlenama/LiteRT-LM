# Copyright 2025 The ODML Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

load("@io_bazel_rules_go//go:def.bzl", "go_binary")
load("@io_bazel_rules_go//go:def.bzl", "go_library")
load("@io_bazel_rules_go//go:def.bzl", "go_test")

# litert_lm:oss-only-begin
# This platform definition enables CGo builds on Windows for the OSS version
# by configuring the MinGW toolchain.
platform(
    name = "windows_amd64_mingw",
    constraint_values = [
        "@bazel_tools//tools/cpp:mingw",
        "@platforms//cpu:x86_64",
        "@platforms//os:windows",
    ],
)
# litert_lm:oss-only-end

package(
    
    
    default_visibility = [
        "//visibility:public",
    ],
)

go_library(
    name = "engine",
    srcs = ["engine.go"],
    cdeps = [
        "//c:engine",  # keep
        "//runtime/core:engine_impl",  # keep
    ],
    cgo = 1,
    importpath = "litert-lm/go_api/engine",
    visibility = ["//visibility:public"],
)

go_binary(
    name = "main",
    srcs = ["main.go"],
    cgo = 1,
    # The "short_logs" configuration is applied by default through the .bazelrc
    # file in the parent directory.
    tags = ["local"],
    visibility = ["//visibility:public"],
    deps = [
        ":engine",
    ],
)


# go_test(
#     name = "engine_test",
#     srcs = ["engine_test.go"],
#     cgo = 1,
#     data = [
#         "//runtime/testdata",
#     ],
#     embed = [":engine"],
# )
