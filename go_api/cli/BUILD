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
load("@io_bazel_rules_go//go:def.bzl", "go_test")

package(
    
    
    default_visibility = [
        "//visibility:public",
    ],
)

go_binary(
    name = "lit",
    srcs = ["main.go"],
    cgo = 1,
    tags = ["local"],
    visibility = ["//visibility:public"],
    deps = [
        "//go_api/cli/cmd",
    ],
)


# go_test(
#     name = "lit_test",
#     srcs = ["main_test.go"],
#     cgo = 1,
#     embed = [":lit"],
# )
