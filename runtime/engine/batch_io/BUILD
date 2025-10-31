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

# [Google-internal load of `cc_library`]
# [Google-internal load of `cc_test`]

package(
    default_hdrs_check = "strict",
    default_visibility = [
        "//visibility:public",
    ],
)

licenses(["notice"])

cc_library(
    name = "litert_lm_batch_io",
    hdrs = ["litert_lm_batch_io.h"],
    deps = [
        ":batch_io_record_converter",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "//runtime/engine:io_types",
    ],
)

cc_library(
    name = "batch_io_record_converter",
    hdrs = ["batch_io_record_converter.h"],
    deps = [
        "@com_google_absl//absl/status:statusor",
        "//runtime/engine:io_types",
    ],
)

cc_library(
    name = "batch_io_utils",
    srcs = ["batch_io_utils.cc"],
    hdrs = ["batch_io_utils.h"],
    deps = [
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings:string_view",
        "//runtime/engine:io_types",
        "//runtime/util:litert_status_util",
    ],
)

# ==============================================================================
# Tests
# ==============================================================================

cc_library(
    name = "test_utils",
    testonly = True,
    srcs = ["test_utils.cc"],
    hdrs = ["test_utils.h"],
    deps = [
        ":litert_lm_batch_io",
        "//runtime/engine:io_types",
    ],
)

cc_test(
    name = "litert_lm_batch_io_test",
    srcs = ["litert_lm_batch_io_test.cc"],
    deps = [
        ":batch_io_utils",
        ":litert_lm_batch_io",
        ":test_utils",
        "@com_google_googletest//:gtest_main",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "//runtime/engine:io_types",
        "//runtime/util:litert_status_util",
        "//runtime/util:test_utils",
    ],
)

cc_test(
    name = "batch_io_utils_test",
    srcs = ["batch_io_utils_test.cc"],
    deps = [
        ":batch_io_utils",
        "@com_google_googletest//:gtest_main",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "//runtime/engine:io_types",
        "//runtime/util:test_utils",
    ],
)
