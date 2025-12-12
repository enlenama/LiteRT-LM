include(ExternalProject)

# --- OpenCL Headers ---
set(OPENCL_EXT_PREFIX ${EXTERNAL_PROJECT_BINARY_DIR}/opencl_headers)
set(OPENCL_SRC_DIR ${OPENCL_EXT_PREFIX}/src/opencl_headers_external)
set(OPENCL_INCLUDE_DIR ${OPENCL_SRC_DIR} CACHE INTERNAL "" FORCE)




ExternalProject_Add(
  opencl_headers_external
  GIT_REPOSITORY https://github.com/KhronosGroup/OpenCL-Headers.git
  GIT_TAG        v2024.05.08
  PREFIX         ${OPENCL_EXT_PREFIX}
  
  # It's header-only, so turn off build/install/configure phases
  CONFIGURE_COMMAND ""
  BUILD_COMMAND     ""
  INSTALL_COMMAND   ""
  
  # Shallow clone to save time/space
  GIT_SHALLOW       TRUE
)

# Create an Interface Library for easy linking
add_library(opencl_headers_lib INTERFACE)
add_dependencies(opencl_headers_lib opencl_headers_external)
target_include_directories(opencl_headers_lib SYSTEM INTERFACE ${OPENCL_INCLUDE_DIR})