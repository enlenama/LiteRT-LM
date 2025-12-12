# Copyright 2026 Google LLC.
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

if(CMAKE_VERSION VERSION_LESS "3.25")
    message(FATAL_ERROR 
        "CMake 3.25+ is required because we use the 'block()' command for variable isolation.\n"
        "Your current version is: ${CMAKE_VERSION}.\n"
        "Please upgrade CMake to proceed."
    )
endif()
cmake_minimum_required(VERSION 3.25 FATAL_ERROR)
project(LiteRT-LM LANGUAGES C CXX)


# --- Global Paths
set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

set(LITERTLM_SCRIPTS_DIR        "${PROJECT_ROOT}/cmake/scripts")
set(LITERTLM_MODULES_DIR        "${PROJECT_ROOT}/cmake/modules")
list(APPEND CMAKE_MODULE_PATH   "${LITERTLM_MODULES_DIR}")

set(EXTERNAL_PROJECT_BINARY_DIR "${CMAKE_BINARY_DIR}/external")
set(THIRD_PARTY_DIR             "${CMAKE_BINARY_DIR}/third_party")

set(GENERATED_SRC_DIR           "${PROJECT_ROOT}/cmake/generated/src")

set(LITERTLM_INCLUDE_PATHS      
  "${GENERATED_SRC_DIR}"
  "${CMAKE_BINARY_DIR}"
)


# --- Environment
if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_POLICY_VERSION_MINIMUM 3.20 CACHE STRING "Required CMake policy version." FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive" CACHE STRING "Required CXX flags" FORCE)
add_link_options("-Wl,-z,muldefs")


# --- Optimization
option(ENABLE_CCACHE "Enables the use of ccache" OFF)
if(${ENABLE_CCACHE} STREQUAL "ON")
  find_program(CCACHE_PROGRAM ccache)
  if(CCACHE_PROGRAM)
      message(STATUS "🚀 Using CCache to speed up builds")
      set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
      set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  else()
      message(WARNING "⚠️ CCache not found. Install it (`sudo apt install ccache`) for faster builds!")
  endif()
endif()


# --- Utilities
include(macros)
include(utils)


# --- Dependencies
include(external_project)
include(fetch_content)

# LITERTLM_INCLUDE_PATHS must be updated before collect_dependencies is called
list(APPEND LITERTLM_INCLUDE_PATHS
  ${OPENCL_HDR_DIR}
  ${ABSL_INCLUDE_DIR}
  ${FLATBUFFERS_INCLUDE_DIR}
  ${GTEST_INCLUDE_DIR}
  ${PROTO_INCLUDE_DIR}
  ${RE2_INCLUDE_DIR}
  ${SENTENCE_INCLUDE_DIR}
  ${TOKENIZER_INCLUDE_DIR}
  ${TFLITE_INCLUDE_DIR}
  ${TFLITE_SRC_DIR}
  ${RUY_INCLUDE_DIR}
  ${TENSORFLOW_SOURCE_DIR}
  ${LITERT_INCLUDE_PATHS}
  ${FETCHCONTENT_MODULE_SRC_DIRS}
  ${FETCHCONTENT_MODULE_INCLUDE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/antlr_generated
)

include(collect_dependencies)


# --- LiteRT-LM Targets
include(setup_generated_src_files)
generate_src_files(ALL_GENERATED_FILES ${ALL_SOURCE_FILES} ${ALL_HEADER_FILES})


if(TARGET LITERTLM_DEPS)
    message(STATUS "✅ DEBUG: LITERTLM_DEPS exists!")
else()
    message(FATAL_ERROR "❌ DEBUG: LITERTLM_DEPS is MISSING!")
endif()


add_compile_definitions(
    absl_nullable=
    absl_nonnull=
)


# # set(CMAKE_EXE_LINKER_FLAGS "${ABSL_LINK_FLAGS}")
# # set(CMAKE_SHARED_LINKER_FLAGS "${ABSL_LINK_FLAGS}")

set(CMAKE_EXE_LINKER_FLAGS
    " -L${ABSL_LIB_DIR} \
    -L${PROTO_INSTALL_PREFIX}/lib \
    -L${TFLITE_LIB_DIR} \
    -L${TFLITE_BUILD_DIR} \
    -L${EXTERNAL_PROJECT_BINARY_DIR}/tflite_external-build/ruy"
)
set(CMAKE_SHARED_LINKER_FLAGS
    " -L${ABSL_LIB_DIR} \
    -L${PROTO_INSTALL_PREFIX}/lib \
    -L${TFLITE_LIB_DIR} \
    -L${TFLITE_BUILD_DIR} \
    -L${EXTERNAL_PROJECT_BINARY_DIR}/tflite_external-build/ruy \
    -L${LITERT_INCLUDE_PATHS}"
)
set(CMAKE_CXX_STANDARD_LIBRARIES
                "-Wl,--start-group \
                -labsl_log_internal_proto \
                -labsl_log_internal_message \
                -labsl_log_internal_structured_proto \
                -labsl_log_internal_check_op \
                -labsl_log_severity \
                -labsl_cord \
                -labsl_cord_internal \
                -labsl_cordz_handle \
                -labsl_cordz_info \
                -labsl_cordz_functions \
                -labsl_cordz_sample_token \
                -labsl_crc_cord_state \
                -labsl_crc32c \
                -labsl_crc_internal\
                -labsl_crc_cpu_detect \
                -labsl_exponential_biased \
                -labsl_symbolize \
                -labsl_stacktrace \
                -labsl_tracing_internal \
                -labsl_debugging_internal \
                -labsl_examine_stack \
                -labsl_demangle_internal \
                -labsl_demangle_rust \
                -labsl_decode_rust_punycode \
                -labsl_log_internal_globals \
                -labsl_log_globals \
                -labsl_log_sink \
                -labsl_log_internal_log_sink_set \
                -labsl_log_internal_format \
                -labsl_log_internal_conditions \
                -labsl_log_internal_nullguard \
                -labsl_log_internal_fnmatch \
                -labsl_status \
                -labsl_statusor \
                -labsl_raw_logging_internal \
                -labsl_base \
                -labsl_spinlock_wait \
                -labsl_malloc_internal \
                -labsl_failure_signal_handler \
                -labsl_throw_delegate \
                -labsl_int128 \
                -labsl_strings \
                -labsl_strings_internal \
                -labsl_string_view \
                -labsl_hashtable_profiler \
                -labsl_strerror \
                -labsl_poison \
                -labsl_bad_any_cast_impl \
                -labsl_bad_optional_access \
                -labsl_bad_variant_access \
                -labsl_synchronization \
                -labsl_periodic_sampler \
                -labsl_scoped_set_env \
                -labsl_kernel_timeout_internal \
                -labsl_time \
                -labsl_time_zone \
                -labsl_hash \
                -labsl_city \
                -labsl_leak_check \
                -labsl_raw_hash_set \
                -labsl_utf8_for_code_point \
                -labsl_hashtablez_sampler \
                -labsl_flags_parse \
                -labsl_flags_usage \
                -labsl_flags_usage_internal \
                -labsl_flags_marshalling \
                -labsl_flags_internal \
                -labsl_flags_reflection \
                -labsl_flags_config \
                -labsl_flags_commandlineflag \
                -labsl_flags_commandlineflag_internal \
                -labsl_flags_private_handle_accessor \
                -labsl_flags_program_name \
                -labsl_die_if_null \
                -lprotobuf \
                -lprotobuf-lite \
                -ltensorflow-lite \
                -lXNNPACK \
                -lxnnpack-microkernels-prod \
                -lxnnpack-delegate \
                -lcpuinfo \
                -lpthreadpool \

                -leight_bit_int_gemm \
                -lfft2d_fftsg \
                -lfft2d_fftsg2d \
                -lfarmhash \
                -lflatbuffers \

                -lruy_allocator \
                -lruy_apply_multiplier \
                -lruy_block_map \
                -lruy_blocking_counter \
                -lruy_context \
                -lruy_context_get_ctx \
                -lruy_cpuinfo \
                -lruy_ctx \
                -lruy_denormal \
                -lruy_frontend \
                -lruy_have_built_path_for_avx \
                -lruy_have_built_path_for_avx2_fma \
                -lruy_have_built_path_for_avx512 \
                -lruy_kernel_arm \
                -lruy_kernel_avx \
                -lruy_kernel_avx2_fma \
                -lruy_kernel_avx512 \
                -lruy_pack_arm \
                -lruy_pack_avx \
                -lruy_pack_avx2_fma \
                -lruy_pack_avx512 \
                -lruy_prepacked_cache \
                -lruy_prepare_packed_matrices \
                -lruy_profiler_instrumentation \
                -lruy_profiler_profiler \
                -lruy_system_aligned_alloc \
                -lruy_thread_pool \
                -lruy_trmul \
                -lruy_tune \
                -lruy_wait \
                -llitert_cc_api \
                -llitert_cc_options \

                -llitert_c_api \
                -llitert_c_options \
                -llitert_logging \

                -llitert_core \
                -llitert_core_model \
                -llitert_compiler_plugin \
                -llitert_runtime \

                -Wl,--end-group \
                -lpthread"
)

set(TENSORFLOW_MISSING_SRCS
    "${TENSORFLOW_SOURCE_DIR}/tensorflow/compiler/mlir/lite/allocation.cc"
    "${TENSORFLOW_SOURCE_DIR}/tensorflow/compiler/mlir/lite/mmap_allocation.cc"
    "${TENSORFLOW_SOURCE_DIR}/tensorflow/compiler/mlir/lite/mmap_allocation_disabled.cc"
    "${TENSORFLOW_SOURCE_DIR}/tensorflow/compiler/mlir/lite/core/api/error_reporter.cc"
    "${TENSORFLOW_SOURCE_DIR}/tensorflow/compiler/mlir/lite/core/model_builder_base.cc"

)
set_source_files_properties(${TENSORFLOW_MISSING_SRCS} PROPERTIES GENERATED TRUE)


add_subdirectory(c)
add_subdirectory(schema)
add_subdirectory(runtime)