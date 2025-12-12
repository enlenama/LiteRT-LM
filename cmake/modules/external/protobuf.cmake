include(ExternalProject)

set(PROTO_EXT_PREFIX ${EXTERNAL_PROJECT_BINARY_DIR}/protobuf)
set(PROTO_INSTALL_PREFIX ${PROTO_EXT_PREFIX}/install)
set(PROTO_CONFIG_CMAKE_FILE "${PROTO_INSTALL_PREFIX}/lib/cmake/protobuf/protobuf-config.cmake")


set(PROTO_SRC_DIR ${PROTO_INSTALL_PREFIX}/src/protobuf_external/src)
set(PROTO_INCLUDE_DIR ${PROTO_INSTALL_PREFIX}/include)
set(PROTO_LIB_DIR ${PROTO_INSTALL_PREFIX}/lib)
set(PROTO_LITE_LIBRARY ${PROTO_INSTALL_PREFIX}/lib/libprotobuf-lite.a)
set(PROTO_BIN_DIR ${PROTO_INSTALL_PREFIX}/bin)

set(PROTO_PROTOC_EXECUTABLE ${PROTO_BIN_DIR}/protoc)
set(protobuf_generate_PROTOC_EXE ${PROTO_BIN_DIR}/protoc)


set(PROTO_FILES
  ${PROJECT_ROOT}/runtime/proto/engine.proto
  ${PROJECT_ROOT}/runtime/proto/llm_metadata.proto
  ${PROJECT_ROOT}/runtime/proto/llm_model_type.proto
  ${PROJECT_ROOT}/runtime/proto/sampler_params.proto
  ${PROJECT_ROOT}/runtime/proto/token.proto
  ${PROJECT_ROOT}/runtime/executor/proto/constrained_decoding_options.proto
  ${PROJECT_ROOT}/runtime/util/external_file.proto
)



if(NOT EXISTS "${PROTO_CONFIG_CMAKE_FILE}")
  message(STATUS "Protobuf not found. Configuring external build...")
  ExternalProject_Add(
    protobuf_external
    DEPENDS 
      absl_external
      googletest_external
    GIT_REPOSITORY
      https://github.com/protocolbuffers/protobuf
    GIT_TAG
      v6.31.1
    PREFIX
      ${PROTO_EXT_PREFIX}
    CMAKE_ARGS
      -DCMAKE_PREFIX_PATH=${GTEST_INSTALL_PREFIX};${ABSL_INSTALL_PREFIX}
      -DCMAKE_INSTALL_PREFIX=${PROTO_INSTALL_PREFIX}
      -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
      -DCMAKE_POLICY_DEFAULT_CMP0169=OLD
      -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
      -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} -I${ABSL_INCLUDE_DIR}
      -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
      -DCMAKE_POSITION_INDEPENDENT_CODE=ON
      -Dprotobuf_BUILD_TESTS=OFF
      -Dprotobuf_BUILD_LIBPROTOC=ON
      -Dprotobuf_BUILD_PROTOBUF_BINARIES=ON
      -Dprotobuf_LOCAL_DEPENDENCIES_ONLY=ON
      -Dabsl_DIR=${ABSL_INSTALL_PREFIX}/lib/cmake/absl
      -DGTest_DIR=${GTEST_INSTALL_PREFIX}/lib/cmake/GTest
      -DProtobuf_DIR=${PROTO_INSTALL_PREFIX}/lib/cmake/Protobuf
      "-DCMAKE_SHARED_LINKER_FLAGS=${ABSL_LINK_FLAGS}"
      "-DCMAKE_EXE_LINKER_FLAGS=-L${ABSL_LIB_DIR} -Wl,-z,muldefs"
      "-DCMAKE_CXX_STANDARD_LIBRARIES= \
                -Wl,--start-group \
                -labsl_leak_check \
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
                -labsl_log_initialize \
                -labsl_hashtable_profiler \
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
                -Wl,--end-group \
                -lpthread"
    STEP_TARGETS
      verify_install_step
  )
  verify_install(protobuf_external ${PROTO_CONFIG_CMAKE_FILE})

else()
  message(STATUS "Protobuf already installed at: ${PROTO_INSTALL_PREFIX}")
  if(NOT TARGET protobuf_external)
    add_custom_target(protobuf_external)
  endif()
endif()


import_static_lib(imp_protobuf      "${PROTO_LIB_DIR}/libprotobuf.a")
import_static_lib(imp_protobuf_lite "${PROTO_LIB_DIR}/libprotobuf-lite.a")
import_static_lib(imp_protoc        "${PROTO_LIB_DIR}/libprotoc.a")
import_static_lib(imp_upb           "${PROTO_LIB_DIR}/libupb.a")
import_static_lib(imp_utf8_validity "${PROTO_LIB_DIR}/libutf8_validity.a")
import_static_lib(imp_utf8_range    "${PROTO_LIB_DIR}/libutf8_range.a")


if(NOT TARGET protobuf::libprotobuf)
    add_library(protobuf::libprotobuf ALIAS imp_protobuf)
endif()

if(NOT TARGET protobuf::protoc)
    add_executable(protobuf::protoc IMPORTED GLOBAL)
    set_target_properties(protobuf::protoc PROPERTIES
        IMPORTED_LOCATION "${PROTO_PROTOC_EXECUTABLE}"
    )
endif()


add_library(proto_lib STATIC)
add_dependencies(proto_lib protobuf_external)

target_include_directories(proto_lib
  PUBLIC
    ${CMAKE_BINARY_DIR}
    ${PROJECT_ROOT}
    ${PROTO_SRC_DIR}
    ${PROTO_INCLUDE_DIR}
    ${ABSL_INCLUDE_DIR}
)

target_link_libraries(proto_lib
  PUBLIC
    protobuf::libprotobuf
    imp_protobuf_lite
    imp_protoc
    imp_upb
    imp_absl_base
    imp_utf8_validity
    imp_utf8_range
)

generate_protobuf(proto_lib)


# 1. Glob Protobuf and its hidden friends
file(GLOB PROTO_INTERNAL_LIBS 
    "${PROTO_LIB_DIR}/libprotobuf.a"
    "${PROTO_LIB_DIR}/libutf8_range.a"
    "${PROTO_LIB_DIR}/libutf8_validity.a"
    "${PROTO_LIB_DIR}/libprotoc.a"
)


set(PROTO_IMPORTED_TARGETS "")
foreach(LIB_PATH ${PROTO_INTERNAL_LIBS})
    get_filename_component(LIB_FILENAME ${LIB_PATH} NAME)
    string(REPLACE "." "_" SAFE_NAME "imp_${LIB_FILENAME}")
    
    add_library(${SAFE_NAME} STATIC IMPORTED)
    set_target_properties(${SAFE_NAME} PROPERTIES IMPORTED_LOCATION "${LIB_PATH}")
    
    # Add to our list
    list(APPEND PROTO_IMPORTED_TARGETS ${SAFE_NAME})
endforeach()

add_library(proto_kitchen_sink INTERFACE)
add_library(LiteRTLM::protobuf::libprotobuf ALIAS proto_kitchen_sink)

target_include_directories(proto_kitchen_sink SYSTEM INTERFACE 
  ${PROTO_INCLUDE_DIR}
  ${ABSL_INCLUDE_DIR}
)

# 3. Link them all inside a "Start Group" block
target_link_libraries(proto_kitchen_sink INTERFACE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wl,--start-group>
    ${PROTO_IMPORTED_TARGETS}
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wl,--end-group>
    
    # System deps often needed by Abseil
    $<$<PLATFORM_ID:Linux>:pthread>
    $<$<PLATFORM_ID:Darwin>:-framework CoreFoundation>
)
string(REPLACE ";" " " PROTO_LIBS_FLAT "${ALL_PROTO_LIBS}")