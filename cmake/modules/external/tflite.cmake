include(ExternalProject)

set(TFLITE_EXT_PREFIX ${EXTERNAL_PROJECT_BINARY_DIR}/tensorflow)
set(TFLITE_INSTALL_PREFIX ${TFLITE_EXT_PREFIX}/install)

# --- Parameters for consumption by higher layers (LiteRT-LM) ---
set(TFLITE_INCLUDE_DIR ${TFLITE_INSTALL_PREFIX}/include)
set(TFLITE_LIB_DIR     ${TFLITE_INSTALL_PREFIX}/lib)
set(TFLITE_SRC_DIR     ${TFLITE_EXT_PREFIX}/src/tflite_external/tensorflow/lite)
set(TFLITE_BUILD_DIR   ${TFLITE_EXT_PREFIX}/src/tflite_external-build CACHE INTERNAL "")
set(TENSORFLOW_SOURCE_DIR ${TFLITE_EXT_PREFIX}/src/tflite_external)

set(TFLITE_STATIC_LIB "${TFLITE_BUILD_DIR}/libtensorflow-lite.a")

set(RUY_INCLUDE_DIR ${EXTERNAL_PROJECT_BINARY_DIR}/tflite_external-build)


if(NOT EXISTS "${TFLITE_STATIC_LIB}")
  message(STATUS "TFLite not found. Configuring external build...")



set(SHIM_CODE 
  "add_library(LiteRTLM::absl::absl INTERFACE IMPORTED GLOBAL)
   set_target_properties(LiteRTLM::absl::absl PROPERTIES INTERFACE_LINK_LIBRARIES \"-Wl,--start-group ${ABSL_LIBS_FLAT} -Wl,--end-group\")")

ExternalProject_Add(
  tflite_external
  DEPENDS 
    absl_external
    flatbuffers_external
    googletest_external
    opencl_headers_external
    protobuf_external
    tokenizers-cpp_external
    # protobuf and tokenizers are not required for TFLite Core
  GIT_REPOSITORY
    https://github.com/tensorflow/tensorflow.git
  GIT_TAG
    061041963ead867e8f47fb63e153db3e61e3b20b
  PREFIX
    ${TFLITE_EXT_PREFIX}
  SOURCE_SUBDIR
    tensorflow/lite
  PATCH_COMMAND
    sed -i "s/FLATBUFFERS_VERSION_MAJOR == [0-9]*/FLATBUFFERS_VERSION_MAJOR >= 25/" <SOURCE_DIR>/tensorflow/lite/acceleration/configuration/configuration_generated.h
    COMMAND unzip -o "${PROJECT_ROOT}/cmake/patches/converter.zip" -d "${TFLITE_SRC_DIR}"
    # COMMAND bash -c "echo 'target_sources(tensorflow-lite PRIVATE \
    #         \${TF_SOURCE_DIR}/compiler/mlir/lite/allocation.cc \
    #         \${TF_SOURCE_DIR}/compiler/mlir/lite/mmap_allocation.cc \
    #         \${TF_SOURCE_DIR}/compiler/mlir/lite/core/model_builder_base.cc \
    #         \${TF_SOURCE_DIR}/compiler/mlir/lite/core/api/error_reporter.cc \
    #         \${TF_SOURCE_DIR}/compiler/mlir/lite/core/api/flatbuffer_conversions.cc \
    #         )' >> <SOURCE_DIR>/tensorflow/lite/CMakeLists.txt"

    COMMAND sed -i "s/FLATBUFFERS_VERSION_MAJOR == 24/FLATBUFFERS_VERSION_MAJOR >= 24/g" <SOURCE_DIR>/tensorflow/compiler/mlir/lite/schema/schema_generated.h
    COMMAND sed -i "s/FLATBUFFERS_VERSION_MINOR == 3/FLATBUFFERS_VERSION_MINOR >= 0/g" <SOURCE_DIR>/tensorflow/compiler/mlir/lite/schema/schema_generated.h
    COMMAND sed -i "s/FLATBUFFERS_VERSION_REVISION == 25/FLATBUFFERS_VERSION_REVISION >= 0/g" <SOURCE_DIR>/tensorflow/compiler/mlir/lite/schema/schema_generated.h 
    COMMAND sed -i "s|--proto_path=${CMAKE_CURRENT_SOURCE_DIR}//..//..//..|--proto_path=${CMAKE_CURRENT_SOURCE_DIR}|g" <SOURCE_DIR>/tensorflow/lite/profiling/proto/CMakeLists.txt

# Kill the TF downloaders
    COMMAND sed -i "1i return()" <SOURCE_DIR>/tensorflow/lite/tools/cmake/modules/abseil-cpp.cmake
    COMMAND sed -i "1i return()" <SOURCE_DIR>/tensorflow/lite/tools/cmake/modules/protobuf.cmake

    # Inject BOTH shims
    COMMAND sed -i "1i include(\"${PROJECT_ROOT}/cmake/patches/tflite_absl_shim.cmake\")" <SOURCE_DIR>/tensorflow/lite/CMakeLists.txt
    COMMAND sed -i "1i include(\"${PROJECT_ROOT}/cmake/patches/tflite_proto_shim.cmake\")" <SOURCE_DIR>/tensorflow/lite/CMakeLists.txt

    # RECURSIVE REDIRECTION (The Global Hammer)
    COMMAND find <SOURCE_DIR>/tensorflow/lite -name "CMakeLists.txt" -exec sed -i "s|[[:space:]]absl::[a-zA-Z0-9_]*| LiteRTLM::absl::absl|g" {} +
    COMMAND find <SOURCE_DIR>/tensorflow/lite -name "CMakeLists.txt" -exec sed -i "s|[[:space:]]protobuf::[a-zA-Z0-9_-]*| LiteRTLM::protobuf::libprotobuf|g" {} +

    CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=${TFLITE_INSTALL_PREFIX}
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_POLICY_DEFAULT_CMP0169=OLD
    -DCMAKE_POLICY_DEFAULT_CMP0170=OLD
    -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    
    # Consolidated CXX Flags
    "-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} -DTF_MAJOR_VERSION=2 -DTF_MINOR_VERSION=20 -DTF_PATCH_VERSION=0 -DTF_VERSION_SUFFIX=\"\""
    
    # Consolidated C Flags
    "-DCMAKE_C_FLAGS=${CMAKE_C_FLAGS} -DTF_MAJOR_VERSION=2 -DTF_MINOR_VERSION=20 -DTF_PATCH_VERSION=0 -DTF_VERSION_SUFFIX=\"\""

    # --- Dependency Injection ---
    # TFLite uses find_package(absl), so we just point it to the config dir
    -Dabsl_DIR=${ABSL_INSTALL_PREFIX}/lib/cmake/absl
    -D_abseil-cpp_LICENSE_FILE:FILEPATH=${ABSL_SRC_DIR}/absl_external/LICENSE
    # TFLite uses find_package(Flatbuffers), so we point it to the config dir

    -DFLATBUFFERS_BUILD_FLATC=OFF
    -DFLATBUFFERS_INSTALL=OFF
    -DFlatBuffers_BINARY_DIR=${FLATBUFFERS_BIN_DIR}
    -DFLATBUFFERS_PROJECT_DIR=${FLATBUFFERS_SRC_DIR}/flatbuffers_external
    -DFlatBuffers_SOURCE_DIR=${FLATBUFFERS_SRC_DIR}/flatbuffers_external
    -D_flatbuffers_LICENSE_FILE=${FLATBUFFERS_SRC_DIR}/flatbuffers_external/LICENSE
    -DFLATC_PATHS=${FLATBUFFERS_BIN_DIR}
    -DFLATBUFFERS_FLATC_EXECUTABLE=${FLATC_EXECUTABLE}
    -Dflatbuffers_DIR=${FLATBUFFERS_INSTALL_PREFIX}/lib/cmake/flatbuffers



    -Dprotobuf_BINARY_DIR=${PROTO_BIN_DIR}
    -Dprotobuf_BUILD_PROTOC_BINARIES=OFF
    -Dprotobuf_SOURCE_DIR=${PROTO_SRC_DIR}


    # --- TFLite Specific Configuration ---
    -DTFLITE_ENABLE_INSTALL=OFF
    -DTFLITE_ENABLE_XNNPACK=ON
    -DTFLITE_ENABLE_RESOURCE_VARIABLE=OFF
    -DXNNPACK_SET_VERBOSITY=OFF
    -DTFLITE_ENABLE_GPU=OFF
    -DTENSORFLOW_SOURCE_DIR=${TENSORFLOW_SOURCE_DIR}
    -DTFLITE_HOST_TOOLS_DIR=${FLATBUFFERS_BIN_DIR}

    "-DCMAKE_PREFIX_PATH=${ABSL_INSTALL_PREFIX};${libpng_lib_BINARY_DIR}"
    -DPNG_FOUND=ON
    -DPNG_LIBRARY=${libpng_lib_BINARY_DIR}/libpng.a
    -DPNG_PNG_INCLUDE_DIR=${libpng_lib_SOURCE_DIR}
    "-DCMAKE_EXE_LINKER_FLAGS=-L${ABSL_LIB_DIR} -L${PROTO_INSTALL_PREFIX}/lib"
    "-DLITERTLM_ABSL_LIBRARIES=${ABSL_LIBS_FLAT}"
    "-DCMAKE_CXX_STANDARD_LIBRARIES= -Wl,-z,multidefs\
        -lpthread"
    
    "-DLITERTLM_ABSL_LIBRARIES=${ABSL_LIBS_FLAT}"
    "-DLITERTLM_ABSL_INCLUDE_DIRS=${ABSL_INCLUDE_DIR}"

    "-DLITERTLM_PROTO_LIBRARIES=${PROTO_LIBS_FLAT}"
    "-DLITERTLM_PROTO_INCLUDE_DIRS=${PROTO_INCLUDE_DIR}"
    "-DLITERTLM_PROTOC_EXECUTABLE=${PROTO_PROTOC_EXECUTABLE}"



)
  
  # Assuming you have a verify_install macro similar to your protobuf setup
  # verify_install(tflite_external ${TFLITE_CONFIG_MARKER})

else()
    message(STATUS "TFLite already installed at: ${TFLITE_STATIC_LIB}")
    if(NOT TARGET tflite_external)
        add_custom_target(tflite_external)
    endif()
endif()



# --- TFLITE AUTO-GENERATED IMPORTS ---
import_static_lib(imp_XNNPACK                    "${TFLITE_LIB_DIR}/libXNNPACK.a")
import_static_lib(imp_cpuinfo                    "${TFLITE_LIB_DIR}/libcpuinfo.a")
import_static_lib(imp_eight_bit_int_gemm         "${TFLITE_LIB_DIR}/libeight_bit_int_gemm.a")
import_static_lib(imp_fft2d_fftsg                "${TFLITE_LIB_DIR}/libfft2d_fftsg.a")
import_static_lib(imp_fft2d_fftsg2d              "${TFLITE_LIB_DIR}/libfft2d_fftsg2d.a")
#import_static_lib(imp_flatbuffers                "${TFLITE_LIB_DIR}/libflatbuffers.a")
import_static_lib(imp_pthreadpool                "${TFLITE_LIB_DIR}/libpthreadpool.a")
import_static_lib(imp_ruy_allocator              "${TFLITE_LIB_DIR}/libruy_allocator.a")
import_static_lib(imp_ruy_apply_multiplier       "${TFLITE_LIB_DIR}/libruy_apply_multiplier.a")
import_static_lib(imp_ruy_block_map              "${TFLITE_LIB_DIR}/libruy_block_map.a")
import_static_lib(imp_ruy_blocking_counter       "${TFLITE_LIB_DIR}/libruy_blocking_counter.a")
import_static_lib(imp_ruy_context                "${TFLITE_LIB_DIR}/libruy_context.a")
import_static_lib(imp_ruy_context_get_ctx        "${TFLITE_LIB_DIR}/libruy_context_get_ctx.a")
import_static_lib(imp_ruy_cpuinfo                "${TFLITE_LIB_DIR}/libruy_cpuinfo.a")
import_static_lib(imp_ruy_ctx                    "${TFLITE_LIB_DIR}/libruy_ctx.a")
import_static_lib(imp_ruy_denormal               "${TFLITE_LIB_DIR}/libruy_denormal.a")
import_static_lib(imp_ruy_frontend               "${TFLITE_LIB_DIR}/libruy_frontend.a")
import_static_lib(imp_ruy_have_built_path_for_avx "${TFLITE_LIB_DIR}/libruy_have_built_path_for_avx.a")
import_static_lib(imp_ruy_have_built_path_for_avx2_fma "${TFLITE_LIB_DIR}/libruy_have_built_path_for_avx2_fma.a")
import_static_lib(imp_ruy_have_built_path_for_avx512 "${TFLITE_LIB_DIR}/libruy_have_built_path_for_avx512.a")
import_static_lib(imp_ruy_kernel_arm             "${TFLITE_LIB_DIR}/libruy_kernel_arm.a")
import_static_lib(imp_ruy_kernel_avx             "${TFLITE_LIB_DIR}/libruy_kernel_avx.a")
import_static_lib(imp_ruy_kernel_avx2_fma        "${TFLITE_LIB_DIR}/libruy_kernel_avx2_fma.a")
import_static_lib(imp_ruy_kernel_avx512          "${TFLITE_LIB_DIR}/libruy_kernel_avx512.a")
import_static_lib(imp_ruy_pack_arm               "${TFLITE_LIB_DIR}/libruy_pack_arm.a")
import_static_lib(imp_ruy_pack_avx               "${TFLITE_LIB_DIR}/libruy_pack_avx.a")
import_static_lib(imp_ruy_pack_avx2_fma          "${TFLITE_LIB_DIR}/libruy_pack_avx2_fma.a")
import_static_lib(imp_ruy_pack_avx512            "${TFLITE_LIB_DIR}/libruy_pack_avx512.a")
import_static_lib(imp_ruy_prepacked_cache        "${TFLITE_LIB_DIR}/libruy_prepacked_cache.a")
import_static_lib(imp_ruy_prepare_packed_matrices "${TFLITE_LIB_DIR}/libruy_prepare_packed_matrices.a")
import_static_lib(imp_ruy_profiler_instrumentation "${TFLITE_LIB_DIR}/libruy_profiler_instrumentation.a")
import_static_lib(imp_ruy_profiler_profiler      "${TFLITE_LIB_DIR}/libruy_profiler_profiler.a")
import_static_lib(imp_ruy_system_aligned_alloc   "${TFLITE_LIB_DIR}/libruy_system_aligned_alloc.a")
import_static_lib(imp_ruy_thread_pool            "${TFLITE_LIB_DIR}/libruy_thread_pool.a")
import_static_lib(imp_ruy_trmul                  "${TFLITE_LIB_DIR}/libruy_trmul.a")
import_static_lib(imp_ruy_tune                   "${TFLITE_LIB_DIR}/libruy_tune.a")
import_static_lib(imp_ruy_wait                   "${TFLITE_LIB_DIR}/libruy_wait.a")
import_static_lib(imp_xnnpack-microkernels-prod  "${TFLITE_LIB_DIR}/libxnnpack-microkernels-prod.a")
import_static_lib(imp_libtflite                  "${TFLITE_BUILD_DIR}/libtensorflow-lite.a")
import_static_lib(imp_xnnpack_delegate            "${TFLITE_BUILD_DIR}/libxnnpack-delegate.a")


add_library(tflite_libs INTERFACE)
target_include_directories(tflite_libs SYSTEM INTERFACE
  ${TFLITE_INCLUDE_DIR}
  ${EXTERNAL_PROJECT_BINARY_DIR}/tflite_external-build/ruy
)
target_link_libraries(tflite_libs INTERFACE
    imp_libtflite
    imp_xnnpack_delegate
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wl,--start-group>
        imp_xnnpack_delegate
        imp_XNNPACK
        imp_cpuinfo
        imp_eight_bit_int_gemm
        imp_fft2d_fftsg
        imp_fft2d_fftsg2d
        imp_pthreadpool
        imp_ruy_allocator
        imp_ruy_apply_multiplier
        imp_ruy_block_map
        imp_ruy_blocking_counter
        imp_ruy_context
        imp_ruy_context_get_ctx
        imp_ruy_cpuinfo
        imp_ruy_ctx
        imp_ruy_denormal
        imp_ruy_frontend
        imp_ruy_have_built_path_for_avx
        imp_ruy_have_built_path_for_avx2_fma
        imp_ruy_have_built_path_for_avx512
        imp_ruy_kernel_arm
        imp_ruy_kernel_avx
        imp_ruy_kernel_avx2_fma
        imp_ruy_kernel_avx512
        imp_ruy_pack_arm
        imp_ruy_pack_avx
        imp_ruy_pack_avx2_fma
        imp_ruy_pack_avx512
        imp_ruy_prepacked_cache
        imp_ruy_prepare_packed_matrices
        imp_ruy_profiler_instrumentation
        imp_ruy_profiler_profiler
        imp_ruy_system_aligned_alloc
        imp_ruy_thread_pool
        imp_ruy_trmul
        imp_ruy_tune
        imp_ruy_wait
        imp_xnnpack-microkernels-prod
        imp_libtflite
        flatbuffers_libs    
        absl_libs
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wl,--end-group>
    flatbuffers_libs    
    absl_libs
)


# 1. Glob all TFLite and related dependency libraries
# We look in both TFLITE_LIB_DIR (external deps) and TFLITE_BUILD_DIR (the core libs)
file(GLOB ALL_TFLITE_LIBS 
    "${TFLITE_LIB_DIR}/*.a"
    "${TFLITE_BUILD_DIR}/*.a"
)

if(NOT ALL_TFLITE_LIBS)
    message(WARNING "No TFLite libs found. Run build then re-run CMake.")
endif()

set(TFLITE_HAMMER_TARGETS "")   # Core libs that need --whole-archive
set(TFLITE_SUPPORT_TARGETS "")  # Math/utility libs (standard link)

# 2. Iterate and create imported targets
foreach(LIB_PATH ${ALL_TFLITE_LIBS})
    get_filename_component(LIB_FILENAME ${LIB_PATH} NAME)
    string(REPLACE "." "_" SAFE_NAME "imp_${LIB_FILENAME}")
    
    if(NOT TARGET ${SAFE_NAME})
        add_library(${SAFE_NAME} STATIC IMPORTED)
        set_target_properties(${SAFE_NAME} PROPERTIES IMPORTED_LOCATION "${LIB_PATH}")
    endif()

    # Determine if this library needs the "Hammer"
    # Core TFLite, LiteRT, and Delegates must be whole-archived for op registration.
    if(LIB_FILENAME MATCHES "libtensorflow-lite.a" OR 
       LIB_FILENAME MATCHES "libxnnpack-delegate.a" OR 
       LIB_FILENAME MATCHES "liblitert")
        list(APPEND TFLITE_HAMMER_TARGETS ${SAFE_NAME})
    else()
        list(APPEND TFLITE_SUPPORT_TARGETS ${SAFE_NAME})
    endif()
endforeach()

# ==============================================================================
# 3. THE KITCHEN SINK INTERFACE
# ==============================================================================

add_library(tflite_kitchen_sink INTERFACE)
add_library(LiteRTLM::tflite::tflite ALIAS tflite_kitchen_sink)

target_include_directories(tflite_kitchen_sink SYSTEM INTERFACE 
    ${TFLITE_INCLUDE_DIR}
    ${TFLITE_BUILD_DIR} # Often needed for generated ruy/cpuinfo headers
)

target_link_libraries(tflite_kitchen_sink INTERFACE
    # --- PHASE 1: THE HAMMER ---
    # Force load registration symbols from core libs
    $<$<PLATFORM_ID:Linux,Android,FreeBSD>:-Wl,--whole-archive>
    $<$<PLATFORM_ID:Darwin>:-Wl,-force_load>
        ${TFLITE_HAMMER_TARGETS}
    $<$<PLATFORM_ID:Linux,Android,FreeBSD>:-Wl,--no-whole-archive>

    # --- PHASE 2: THE GROUP ---
    # Standard resolution for math/util libs (XNNPACK, Ruy, cpuinfo, etc.)
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wl,--start-group>
        ${TFLITE_SUPPORT_TARGETS}
        # Include your existing absl/flatbuffers kitchen sinks here
        absl_kitchen_sink
        flatbuffers_libs
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wl,--end-group>

    # System Dependencies
    pthread
    $<$<PLATFORM_ID:Linux>:dl>
    $<$<PLATFORM_ID:Android>:log>
)
