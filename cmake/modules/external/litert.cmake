include(ExternalProject)

# =========================================================
#  1. Path & Environment Setup
# =========================================================
set(LITERT_EXT_PREFIX    ${EXTERNAL_PROJECT_BINARY_DIR}/litert)

set(LITERT_BUILD_DIR     ${LITERT_EXT_PREFIX}/src/litert_external-build)
set(LITERT_SOURCE_DIR    ${LITERT_EXT_PREFIX}/src/litert_external)
set(LITERT_INCLUDE_PATHS
  ${LITERT_SOURCE_DIR}
  ${LITERT_BUILD_DIR}/include
  ${LITERT_BUILD_DIR}/c/include
  ${LITERT_BUILD_DIR}/cc/include
  ${LITERT_BUILD_DIR}/compiler/include
  ${LITERT_BUILD_DIR}/core/include
  ${LITERT_BUILD_DIR}/runtime/include
)

# =========================================================
#  2. Construct Compiler Flags (The "dirty work")
# =========================================================
# Start with standard flags + permissive mode
set(LITERT_CXX_FLAGS_Construct "${CMAKE_CXX_FLAGS} -fpermissive")

# Inject OpenCL Headers if present
if(OPENCL_INCLUDE_DIR)
  string(APPEND LITERT_CXX_FLAGS_Construct " -isystem ${OPENCL_INCLUDE_DIR}")
endif()

# Inject the OpenCL "Type Mocks" to satisfy the compiler
# We define these types as void* or ulong to match the binary ABI without headers.
# string(APPEND LITERT_CXX_FLAGS_Construct " \
#   -Dcl_command_buffer_khr=void* \
#   -Dcl_command_buffer_properties_khr=cl_ulong \
#   -Dcl_ndrange_kernel_command_properties_khr=cl_ulong \
#   -Dcl_sync_point_khr=void* \
#   -Dcl_mutable_command_khr=void* \
#   -Dcl_command_buffer_info_khr=cl_uint")

# =========================================================
#  3. The External Project Definition
# =========================================================
ExternalProject_Add(
  litert_external
  DEPENDS
    opencl_headers_external
    absl_external
    protobuf_external
    flatbuffers_external
    tflite_external

  GIT_REPOSITORY
    https://github.com/google-ai-edge/LiteRT.git
  GIT_TAG
    v2.1.0
  PREFIX
    ${LITERT_EXT_PREFIX}
  SOURCE_SUBDIR
    litert

  # ---------------------------------------------------------
  #  PATCHES
  # ---------------------------------------------------------
  PATCH_COMMAND 
    # [A] Fix Compilation Errors (Return types & Missing dirs)
    sed -i "s/ return litert_cpu_buffer_requirements/return litert::Expected<const LiteRtTensorBufferRequirementsT*>(litert_cpu_buffer_requirements)/" <SOURCE_DIR>/litert/runtime/compiled_model.cc
    COMMAND sed -i "s|add_subdirectory(compiler_plugin)|add_subdirectory(compiler)|g" <SOURCE_DIR>/litert/CMakeLists.txt

    # [B] Fix Google's CMake Structure (Comment out overrides)
    COMMAND sed -i "s|set(TFLITE_BUILD_DIR|#set(TFLITE_BUILD_DIR|" <SOURCE_DIR>/litert/CMakeLists.txt
    COMMAND sed -i "s|set(TFLITE_SOURCE_DIR|#set(TFLITE_SOURCE_DIR|" <SOURCE_DIR>/litert/CMakeLists.txt

    # [C] Fix Missing/Moved Source Files
    COMMAND sed -i "s|    litert_accelerator.cc|    internal/litert_accelerator.cc|g" <SOURCE_DIR>/litert/c/CMakeLists.txt
    COMMAND sed -i "s|    litert_accelerator_registration.cc|    internal/litert_accelerator_registration.cc|g" <SOURCE_DIR>/litert/c/CMakeLists.txt
    
    # [D] Comment out broken files we don't need
    COMMAND sed -i "s|    model_graph.cc|#    model_graph.cc|g" <SOURCE_DIR>/litert/core/model/CMakeLists.txt
    COMMAND sed -i "s|    tensor_buffer_conversion.cc|#    tensor_buffer_conversion.cc|g" <SOURCE_DIR>/litert/runtime/CMakeLists.txt
    COMMAND sed -i "s|    webgpu_buffer.cc|#    webgpu_buffer.cc|g" <SOURCE_DIR>/litert/runtime/CMakeLists.txt

    # COMMAND sed -i "s|    message(FATAL_ERROR \"FlatBuffers|#    message(FATAL_ERROR \"FlatBuffers|" <SOURCE_DIR>/litert/core/model/CMakeLists.txt
    # COMMAND sed -i "s|    message(FATAL_ERROR \"FlatBuffers|#    message(FATAL_ERROR \"FlatBuffers|" <SOURCE_DIR>/litert/vender/CMakeLists.txt

    # Stop MediaTek from clearing our FLATC_EXECUTABLE variable
    COMMAND sed -i "s/set(FLATC_EXECUTABLE \"\")/#set(FLATC_EXECUTABLE \"\")/g" <SOURCE_DIR>/litert/vendors/CMakeLists.txt

    # [F] THE "EMPTY()" POLYFILL (Critical for v24 compatibility)
    # The compiler is finding older headers first, so we replace .empty() with .size() != 0
    # COMMAND sed -i "s/!buffers->empty()/buffers->size() != 0/g" <SOURCE_DIR>/tflite/converter/core/model_builder_base.h

    # # [Fix Root Overlay Path]
    # COMMAND sed -i "s|set(_overlay_root.*)|set(_overlay_root \"${TFLITE_SRC_DIR}/converter\")|g" <SOURCE_DIR>/litert/CMakeLists.txt

    # # [Fix Model Schema Output Path]
    # COMMAND sed -i "s|generated/include/tflite/schema/mutable|generated/include/converter/schema/mutable|g" <SOURCE_DIR>/litert/core/model/CMakeLists.txt

    # COMMAND sed -i "s|set(_overlay_root \"${CMAKE_CURRENT_SOURCE_DIR}/../tflite/converter\")|  set(_overlay_root \"${TFLITE_SOURCE_DIR}/tflite/converter\")|" <SOURCE_DIR>/litert/core/model/CMakeLists.txt

        # [E] THE NUCLEAR OPTION (Versioning)
    # Recursively find ALL generated headers and force them to accept our FlatBuffers version.
    COMMAND find <SOURCE_DIR> -name "*generated.h" -exec sed -i "s/FLATBUFFERS_VERSION_MAJOR == 25/FLATBUFFERS_VERSION_MAJOR >= 24/g" {} +
    COMMAND find <SOURCE_DIR> -name "*generated.h" -exec sed -i "s/FLATBUFFERS_VERSION_MINOR == [0-9]*/FLATBUFFERS_VERSION_MINOR >= 0/g" {} +
    COMMAND find <SOURCE_DIR> -name "*generated.h" -exec sed -i "s/FLATBUFFERS_VERSION_REVISION == [0-9]*/FLATBUFFERS_VERSION_REVISION >= 0/g" {} +

     # [Fix] Use double backslashes so CMake passes single backslashes to sed
    COMMAND sed -i "s/constexpr \\(.*\\)Layout(/ \\1Layout(/g" <SOURCE_DIR>/litert/cc/litert_layout.h

    COMMAND sed -i "s|\$<BUILD_INTERFACE:.*/opencl_headers>|        {OPENCL_INCLUDE_DIR}|g" <SOURCE_DIR>/litert/runtime/CMakeLists.txt



  # ---------------------------------------------------------
  #  CMAKE ARGUMENTS
  # ---------------------------------------------------------
  CMAKE_ARGS
    # Core Settings
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5
    -DCMAKE_POLICY_DEFAULT_CMP0169=OLD
    -DCMAKE_POLICY_DEFAULT_CMP0170=OLD
    
    # Toolchain & Standard
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
    -DCMAKE_CXX_STANDARD_REQUIRED=ON
    -DCMAKE_CXX_EXTENSIONS=OFF
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    
    # Inject our constructed flags (Includes OpenCL mocks)
    "-DCMAKE_CXX_FLAGS=${LITERT_CXX_FLAGS_Construct}"
    "-DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}" 

    # OpenCL Versioning
    -DCL_TARGET_OPENCL_VERSION=220
    -DCL_HPP_TARGET_OPENCL_VERSION=220
    -DCL_HPP_MIN_TARGET_OPENCL_VERSION=220

    # Dependency Injection: Abseil
    "-D_abseil-cpp_LICENSE_FILE:FILEPATH=${ABSL_EXT_PREFIX}/src/absl_external/LICENSE"
    "-DFETCHCONTENT_SOURCE_DIR_ABSEIL-CPP=${ABSL_EXT_PREFIX}/src/absl_external"
    -Dabsl_SOURCE_DIR=${ABSL_SRC_DIR}
    -Dabsl_BINARY_DIR=${ABSL_BUILD_DIR}
    -Dabsl_INCLUDE_DIR=${ABSL_INCLUDE_DIR}
    -DABSL_LIBRARIES=${ABSL_LIB_DIR}
    -Dabsl_DIR=${ABSL_LIB_DIR}/cmake/absl

    # FlatBuffers
    -DFLATBUFFERS_BUILD_FLATC=OFF
    -DFLATBUFFERS_INSTALL=OFF
    -DFLATBUFFERS_PROJECT_DIR=${FLATBUFFERS_SRC_DIR}/flatbuffers_external
    -DFlatBuffers_BINARY_DIR=${FLATBUFFERS_BIN_DIR}
    -DFlatBuffers_SOURCE_DIR=${FLATBUFFERS_SRC_DIR}/flatbuffers_external
    -D_flatbuffers_LICENSE_FILE:FILEPATH=${FLATBUFFERS_SRC_DIR}/flatbuffers_external/LICENSE
    -DFLATC_PATHS=${FLATBUFFERS_BIN_DIR}
    -DFLATBUFFERS_FLATC_EXECUTABLE=${FLATC_EXECUTABLE}
    -DFLATC_EXECUTABLE=${FLATC_EXECUTABLE}
    -Dflatbuffers_DIR=${FLATBUFFERS_INSTALL_PREFIX}/lib/cmake/flatbuffers
    -DFETCHCONTENT_SOURCE_DIR_FLATBUFFERS=${FLATBUFFERS_SRC_DIR}/flatbuffers_external
    
    # Dependency Injection: TFLite
    -DTFLite_DIR=${TFLITE_INSTALL_PREFIX}/lib
    -Dtensorflow-lite_DIR=${TFLITE_INSTALL_PREFIX}/lib
    -DTFLITE_BUILD_DIR=${TFLITE_BUILD_DIR}
    -DTFLITE_SOURCE_DIR=${TFLITE_SRC_DIR}
    -DTENSORFLOW_SOURCE_DIR=${TENSORFLOW_SOURCE_DIR}

    # LiteRT Feature Switches
    -DLITERT_AUTO_BUILD_TFLITE=OFF
    -DTFLITE_ENABLE_INSTALL=OFF
    -DTFLITE_ENABLE_XNNPACK=ON
    -DTFLITE_ENABLE_RESOURCE_VARIABLE=OFF
    -DXNNPACK_SET_VERBOSITY=OFF
    -DTFLITE_ENABLE_GPU=OFF
    -DLITERT_ENABLE_GPU=OFF 
    -DLITERT_ENABLE_NPU=OFF
    -DLITERT_ENABLE_QUALCOMM=OFF    
    -DLITERT_DISABLE_KLEIDIAI=OFF
    -DLITERT_BUILD_C_API=ON
    -DLITERT_BUILD_TOOLS=OFF
    "-DCMAKE_SHARED_LINKER_FLAGS=${ABSL_LINK_FLAGS}"
    "-DCMAKE_EXE_LINKER_FLAGS=-L${ABSL_LIB_DIR} -L${PROTO_LIB_DIR} -L${FLATBUFFERS_LIB_DIR} -L${TFLITE_LIB_DIR} -Wl,-z,muldefs"
    "-DCMAKE_CXX_STANDARD_LIBRARIES= \
      -Wl,--start-group \
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
      -labsl_strerror \
      -labsl_poison \
      -labsl_synchronization \
      -labsl_periodic_sampler \
      -labsl_scoped_set_env \
      -labsl_kernel_timeout_internal \
      -labsl_time \
      -labsl_time_zone \
      -labsl_hash \
      -labsl_city \
      -labsl_hashtable_profiler \
      -labsl_log_initialize \
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
      -Wl,--end-group \
      "

  INSTALL_COMMAND ""
)

# ==============================================================================
# IMPORT STATIC LIBRARIES
# ==============================================================================


# --- C++ API (The Wrappers) ---
import_static_lib(imp_litert_cc_api          "${LITERT_BUILD_DIR}/cc/liblitert_cc_api.a")
# This ONE lib contains gpu_options, mediatek_options, etc.
import_static_lib(imp_litert_cc_options      "${LITERT_BUILD_DIR}/cc/options/liblitert_cc_options.a")


# --- C API (The Implementations) ---
import_static_lib(imp_litert_c_api           "${LITERT_BUILD_DIR}/c/liblitert_c_api.a")
import_static_lib(imp_litert_c_options       "${LITERT_BUILD_DIR}/c/options/liblitert_c_options.a")
import_static_lib(imp_litert_logging         "${LITERT_BUILD_DIR}/c/liblitert_logging.a")

# --- Core & Runtime ---
import_static_lib(imp_litert_compiler_plugins "${LITERT_BUILD_DIR}/compiler/liblitert_compiler_plugin.a")
import_static_lib(imp_litert_core            "${LITERT_BUILD_DIR}/core/liblitert_core.a")
import_static_lib(imp_litert_core_model      "${LITERT_BUILD_DIR}/core/model/liblitert_core_model.a")
import_static_lib(imp_litert_runtime         "${LITERT_BUILD_DIR}/runtime/liblitert_runtime.a")


import_static_lib(imp_qnn_context_binary_info "${LITERT_BUILD_DIR}/vendors/qualcomm/libqnn_context_binary_info.a")
import_static_lib(imp_qnn_manager "${LITERT_BUILD_DIR}/vendors/qualcomm/libqnn_manager.a")




# ==============================================================================
# MAIN TARGET
# ==============================================================================
add_library(litert_libs INTERFACE)
target_include_directories(litert_libs SYSTEM INTERFACE 
  ${LITERT_INCLUDE_PATHS}
)

target_link_libraries(litert_libs INTERFACE
  $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wl,--start-group>
    imp_litert_cc_api
    imp_litert_cc_options

    imp_litert_c_api
    imp_litert_c_options

    imp_litert_compiler_plugins
    imp_litert_core
    imp_litert_core_model
    imp_litert_runtime
    imp_litert_logging

    tflite_libs
    farmhash
    proto_lib
    flatbuffers_libs
    absl_libs  
  $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wl,--end-group>
  tflite_libs
  farmhash
  proto_lib
  flatbuffers_libs
  absl_libs  
)