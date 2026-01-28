load("//:rust_cxx_bridge.bzl", "rust_cxx_bridge")
load("@rules_rust//rust:defs.bzl", "rust_library", "rust_test")

package(
    default_visibility = [
        ":default_visibility",
    ],
)

package_group(
    name = "default_visibility",
    packages = [
        "//runtime/components/...",
        "//runtime/components/tool_use/...",
    ],
)

rust_library(
    name = "minijinja_template",
    srcs = [
        "minijinja_template.rs",
    ],
    deps = [
        "@crate_index//:cxx",
        "@crate_index//:minijinja",
        "@crate_index//:serde",
        "@crate_index//:serde_json",
        "@crate_index//:chrono",
    ],
)

rust_cxx_bridge(
    name = "minijinja_template_cpp",
    src = "minijinja_template.rs",
    deps = [
        ":minijinja_template",
    ] + select({
        "@platforms//os:windows": [
            "//rust:alloc_defs",
            "//rust:global_allocator",
        ],
        "//conditions:default": [],
    }),
)

rust_test(
    name = "minijinja_template_test",
    crate = ":minijinja_template",
)
