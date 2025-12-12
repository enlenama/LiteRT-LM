include(ExternalProject)

set(PKG_ROOT ${CMAKE_CURRENT_SOURCE_DIR})


set(GTEST_EXT_PREFIX ${EXTERNAL_PROJECT_BINARY_DIR}/googletest)
set(GTEST_INSTALL_PREFIX ${GTEST_EXT_PREFIX}/install)
set(GTEST_INCLUDE_DIR ${GTEST_INSTALL_PREFIX}/include)
set(GTEST_CONFIG_CMAKE_FILE "${GTEST_INSTALL_PREFIX}/lib/cmake/GTest/GTestConfig.cmake")

if(NOT EXISTS "${GTEST_CONFIG_CMAKE_FILE}")
  message(STATUS "GoogleTest not found. Configuring external build...")

  ExternalProject_Add(
    googletest_external
    DEPENDS
      absl_external
    GIT_REPOSITORY
      https://github.com/google/googletest
    GIT_TAG
      v1.17.0
    PREFIX
      ${GTEST_EXT_PREFIX}
    CMAKE_ARGS
      -DCMAKE_PREFIX_PATH=${ABSL_INSTALL_PREFIX}
      -DCMAKE_INSTALL_PREFIX=${GTEST_INSTALL_PREFIX}
      -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
      -DCMAKE_POLICY_DEFAULT_CMP0169=OLD
      -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
      -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
      -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
      -DCMAKE_POSITION_INDEPENDENT_CODE=ON

    STEP_TARGETS
      step_verify_install
  )
  verify_install(googletest_external ${GTEST_CONFIG_CMAKE_FILE})

else()
    message(STATUS "GoogleTest already installed at: ${GTEST_INSTALL_PREFIX}")
    if(NOT TARGET googletest_external)
        add_custom_target(googletest_external)
    endif()
endif()


import_static_lib(imp_gmock                      "${GTEST_LIB_DIR}/libgmock.a")
import_static_lib(imp_gmock_main                 "${GTEST_LIB_DIR}/libgmock_main.a")
import_static_lib(imp_gtest                      "${GTEST_LIB_DIR}/libgtest.a")
import_static_lib(imp_gtest_main                 "${GTEST_LIB_DIR}/libgtest_main.a")

add_library(gtest_libs INTERFACE)
target_link_libraries(gtest_libs INTERFACE
    imp_gmock
    imp_gmock_main
    imp_gtest
    imp_gtest_main
)