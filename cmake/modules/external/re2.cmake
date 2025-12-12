include(ExternalProject)

set(PKG_ROOT ${CMAKE_CURRENT_SOURCE_DIR})


set(RE2_EXT_PREFIX ${EXTERNAL_PROJECT_BINARY_DIR}/re2)
set(RE2_INSTALL_PREFIX ${RE2_EXT_PREFIX}/install)
set(RE2_LIB_DIR ${RE2_INSTALL_PREFIX}/lib)
set(RE2_INCLUDE_DIR ${RE2_INSTALL_PREFIX}/include)
set(RE2_CONFIG_CMAKE_FILE "${RE2_LIB_DIR}/cmake/re2/re2Config.cmake")


if(NOT EXISTS "${RE2_CONFIG_CMAKE_FILE}")
  message(STATUS "RE2 not found. Configuring external build...")
  ExternalProject_Add(
    re2_external
    DEPENDS
      absl_external
    GIT_REPOSITORY
      https://github.com/google/re2/
    GIT_TAG
      main
    PREFIX
      ${RE2_EXT_PREFIX}
    CMAKE_ARGS
      ${STD_CMAKE_ARGS}
      -DCMAKE_PREFIX_PATH=${ABSL_INSTALL_PREFIX}
      -DCMAKE_INSTALL_PREFIX=${RE2_INSTALL_PREFIX}
    STEP_TARGET
      verify_install
  )

  verify_install(re2_external ${RE2_CONFIG_CMAKE_FILE})


else()
  message(STATUS "RE2 already installed at: ${RE2_INSTALL_PREFIX}")
  if(NOT TARGET re2_external)
    add_custom_target(re2_external)
  endif()
endif()


import_static_lib(imp_re2
  "${RE2_LIB_DIR}/libre2.a"
)

add_library(re2_libs INTERFACE)
target_link_libraries(re2_libs INTERFACE
    imp_re2
)