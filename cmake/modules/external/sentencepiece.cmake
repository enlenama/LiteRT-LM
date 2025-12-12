include(ExternalProject)

set(SENTENCE_EXT_PREFIX ${EXTERNAL_PROJECT_BINARY_DIR}/sentencepiece)
set(SENTENCE_INSTALL_PREFIX ${SENTENCE_EXT_PREFIX}/install)
set(SENTENCE_INCLUDE_DIR ${SENTENCE_INSTALL_PREFIX}/include)

# Detect lib vs lib64
if(EXISTS "${SENTENCE_INSTALL_PREFIX}/lib64")
  set(SENTENCE_LIB_DIR "${SENTENCE_INSTALL_PREFIX}/lib64")
else()
  set(SENTENCE_LIB_DIR "${SENTENCE_INSTALL_PREFIX}/lib")
endif()

set(SENTENCE_LIBRARY_STATIC "${SENTENCE_LIB_DIR}/libsentencepiece.a")
set(SENTENCE_LIBRARY_TRAIN  "${SENTENCE_LIB_DIR}/libsentencepiece_train.a")


# set(ABSL_LINK_FLAGS "-L${ABSL_LIB_DIR} -Wl,--start-group -l:libabsl_*.a -Wl,--end-group -lpthread")


# file(GLOB_RECURSE ABSL_ALL_LIBS "${ABSL_INSTALL_PREFIX}/lib/libabsl_*.a")

# # CMake lists use semicolons (;). The linker needs spaces.
# string(REPLACE ";" " " ABSL_LIBS_STR "${ABSL_ALL_LIBS}")


if(NOT EXISTS "${SENTENCE_LIBRARY_STATIC}")
  message(STATUS "SentencePiece not found. Configuring external build...")
  
  ExternalProject_Add(
    sentencepiece_external
    DEPENDS 
      absl_external
      protobuf_external
    GIT_REPOSITORY https://github.com/google/sentencepiece.git
    GIT_TAG        v0.2.1
    PREFIX         ${SENTENCE_EXT_PREFIX}
    
    PATCH_COMMAND
      ${CMAKE_COMMAND} -E remove_directory <SOURCE_DIR>/third_party/abseil-cpp
      COMMAND ${CMAKE_COMMAND} -E remove_directory <SOURCE_DIR>/third_party/protobuf
      
      # Step 2: Remove C++17 enforcement
      COMMAND sed -i "/set(CMAKE_CXX_STANDARD 17)/d" <SOURCE_DIR>/CMakeLists.txt
      
      # # Step 3: Append logic to link Abseil to SentencePiece targets
      # # We use 'bash -c' because '>>' redirection is a shell feature.
      # COMMAND bash -c "echo '' >> <SOURCE_DIR>/src/CMakeLists.txt"
      # COMMAND bash -c "echo '# --- PATCH: Force link Abseil for Protobuf dependencies ---' >> <SOURCE_DIR>/src/CMakeLists.txt"
      # COMMAND bash -c "echo 'file(GLOB ALL_ABSL_LIBS \"${ABSL_INSTALL_PREFIX}/lib/libabsl_*.a\")' >> <SOURCE_DIR>/src/CMakeLists.txt"
      # COMMAND bash -c "echo 'target_link_libraries(sentencepiece-static PUBLIC \${ALL_ABSL_LIBS})' >> <SOURCE_DIR>/src/CMakeLists.txt"
      # COMMAND bash -c "echo 'target_link_libraries(sentencepiece_train-static PUBLIC \${ALL_ABSL_LIBS})' >> <SOURCE_DIR>/src/CMakeLists.txt"



    CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX=${SENTENCE_INSTALL_PREFIX}
      -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
      -DCMAKE_POLICY_VERSION_MINIMUM=3.5
      
      # Force C++20
      -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
      -DCMAKE_CXX_STANDARD_REQUIRED=ON
      -DCMAKE_POSITION_INDEPENDENT_CODE=ON
      -DCMAKE_PREFIX_PATH="${ABSL_INSTALL_PREFIX};${PROTO_INSTALL_PREFIX}"

      "-DCMAKE_SHARED_LINKER_FLAGS=${ABSL_LINK_FLAGS}"
      "-DCMAKE_EXE_LINKER_FLAGS=-L${ABSL_LIB_DIR} -L${PROTO_INSTALL_PREFIX}/lib"
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
                -labsl_hashtable_profiler \
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
                -lutf8_range \
                -Wl,--end-group \
                -lpthread"

      # Provider Settings
      -DSPM_ABSL_PROVIDER=package
      -DSPM_PROTOBUF_PROVIDER=package
      -DSPM_ENABLE_SHARED=OFF
      -DSPM_ENABLE_TCMALLOC=OFF
      
      -Dabsl_DIR=${ABSL_INSTALL_PREFIX}/lib/cmake/absl
      -DABSL_INCLUDE_DIRS=${ABSL_INCLUDE_DIR}

      -DProtobuf_DIR=${PROTO_INSTALL_PREFIX}/lib/cmake/protobuf
      -DProtobuf_LIBRARIES=${PROTO_LIB_DIR}
      -DProtobuf_INCLUDE_DIR=${PROTO_INCLUDE_DIR}
      -DProtobuf_LIBRARY_DEBUG=${PROTO_LIB_DIR}/libprotobuf.a
      -DProtobuf_LIBRARY_RELEASE=${PROTO_LIB_DIR}/libprotobuf.a
      -DProtobuf_LITE_LIBRARY_DEBUG=${PROTO_LIB_DIR}/libprotobuf-lite.a
      -DProtobuf_LITE_LIBRARY_RELEASE=${PROTO_LIB_DIR}/libprotobuf-lite.a
      -DProtobuf_PROTOC_EXECUTABLE=${PROTO_PROTOC_EXECUTABLE}
      -DProtobuf_PROTOC_LIBRARY_DEBUG=${PROTO_LIB_DIR}/libprotoc.a
      -DProtobuf_PROTOC_LIBRARY_RELEASE=${PROTO_LIB_DIR}/libprotoc.a
    STEP_TARGETS install
  )
else()
  if(NOT TARGET sentencepiece_external)
    add_custom_target(sentencepiece_external)
  endif()
endif()

# Import Libs
import_static_lib(imp_sentencepiece       "${SENTENCE_LIBRARY_STATIC}")
import_static_lib(imp_sentencepiece_train "${SENTENCE_LIBRARY_TRAIN}")

add_library(sentencepiece_libs INTERFACE)
add_dependencies(sentencepiece_libs 
  sentencepiece_external
  proto_lib
  absl_libs
)
target_include_directories(sentencepiece_libs INTERFACE ${SENTENCE_INCLUDE_DIR})

target_link_libraries(sentencepiece_libs INTERFACE 
    imp_sentencepiece_train
    imp_sentencepiece

    proto_lib
    absl_libs
)