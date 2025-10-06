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

load("@io_bazel_rules_go//go:def.bzl", "go_library")

package(
    
    
    default_visibility = [
        "//visibility:public",
    ],
)

go_library(
    name = "cmd",
    srcs = [
        "list.go",
        "list_models.go",
        "pull.go",
        "rm.go",
        "root.go",
        "run.go",
    ],
    importpath = "litert-lm/go_api/cli/cmd/cmd",
    deps = [
        "@com_github_spf13_cobra//:cobra",
        "//go_api:engine",
    ],
)
