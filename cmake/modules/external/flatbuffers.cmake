include(ExternalProject)

set(FLATBUFFERS_EXT_PREFIX ${EXTERNAL_PROJECT_BINARY_DIR}/flatbuffers)
set(FLATBUFFERS_INSTALL_PREFIX ${FLATBUFFERS_EXT_PREFIX}/install)
set(FLATBUFFERS_INCLUDE_DIR ${FLATBUFFERS_INSTALL_PREFIX}/include)
set(FLATBUFFERS_SRC_DIR ${FLATBUFFERS_EXT_PREFIX}/src)
set(FLATBUFFERS_BIN_DIR ${FLATBUFFERS_INSTALL_PREFIX}/bin)
set(FLATBUFFERS_LIB_DIR ${FLATBUFFERS_INSTALL_PREFIX}/lib)


set(FLATBUFFERS_DIR ${FLATBUFFERS_LIB_DIR}/cmake/flatbuffers CACHE INTERNAL "")
set(FLATBUFFERS_CMAKE_CONFIG_FILE ${FLATBUFFERS_LIB_DIR}/cmake/flatbuffers/flatbuffers-config.cmake)
set(FLATC_EXECUTABLE ${FLATBUFFERS_BIN_DIR}/flatc CACHE INTERNAL "")

if(NOT EXISTS "${FLATBUFFERS_CMAKE_CONFIG_FILE}")
  message(STATUS "Flatbuffers not found. Configuring external build...")

  ExternalProject_Add(
    flatbuffers_external
    DEPENDS
      absl_external
      googletest_external
    GIT_REPOSITORY https://github.com/google/flatbuffers.git
    GIT_TAG v25.9.23
    PREFIX ${FLATBUFFERS_EXT_PREFIX}
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${FLATBUFFERS_INSTALL_PREFIX}
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DFLATBUFFERS_BUILD_TESTS=OFF
        -DFLATBUFFERS_BUILD_GRPCTEST=OFF
        -DFLATBUFFERS_INSTALL=ON
        -DFLATBUFFERS_BUILD_FLATC=ON
        -DFLATBUFFERS_BUILD_FLATHASH=OFF
        -DFLATBUFFERS_CPP_STD=20
    STEP_TARGETS
      step_verify_install
  )
  verify_install(flatbuffers_external ${FLATBUFFERS_CMAKE_CONFIG_FILE})
else()
    message(STATUS "Flatbuffers already installed at: ${FLATBUFFERS_INSTALL_PREFIX}")
    if(NOT TARGET flatbuffers_external)
        add_custom_target(flatbuffers_external)
    endif()
endif()

import_static_lib(imp_flatbuffers "${FLATBUFFERS_LIB_DIR}/libflatbuffers.a")

add_library(flatbuffers_libs INTERFACE)
target_link_libraries(flatbuffers_libs INTERFACE imp_flatbuffers)
target_include_directories(flatbuffers_libs INTERFACE ${FLATBUFFERS_INCLUDE_DIR})

set(schema_fbs
  "${PROJECT_ROOT}/schema/core/litertlm_header_schema.fbs"
)
compile_flatbuffer_files(${schema_fbs})