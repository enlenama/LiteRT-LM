# ==============================================================================
# LITERTLM MACROS
# ==============================================================================

# ------------------------------------------------------------------------------
# Macro: import_static_lib
# Purpose: Imports a built static library (.a) from an absolute path.
# Usage: import_static_lib(my_target_name "/path/to/lib.a")
# ------------------------------------------------------------------------------
macro(import_static_lib target_name lib_full_path)
    if("${lib_full_path}" STREQUAL "")
        message(FATAL_ERROR "Critical Error: Attempted to import '${target_name}' with an empty path.")
    endif()

    add_library(${target_name} STATIC IMPORTED GLOBAL)
    set_target_properties(${target_name} PROPERTIES
        IMPORTED_LOCATION "${lib_full_path}"
    )
endmacro()

# ------------------------------------------------------------------------------
# Macro: add_litertlm_library
# Purpose: Wrapper for add_library that automatically ensures build order.
# Usage: add_litertlm_library(my_lib STATIC src/file.cc)
#        add_litertlm_library(my_interface INTERFACE)
# ------------------------------------------------------------------------------
macro(add_litertlm_library target_name lib_type)
    add_library(${target_name} ${lib_type} ${ARGN})
    
    if(TARGET litert_external)
        add_dependencies(${target_name} litert_external)
    endif()
endmacro()

# ------------------------------------------------------------------------------
# Macro: add_litertlm_executable
# Purpose: Wrapper for add_executable that automatically ensures build order.
# Usage: add_litertlm_executable(my_app src/main.cc)
# ------------------------------------------------------------------------------
macro(add_litertlm_executable target_name)
    add_executable(${target_name} ${ARGN})

    # Inject Build Order Dependency
    if(TARGET litert_external)
        add_dependencies(${target_name} litert_external)
    endif()
endmacro()



macro(import_proto_lib target_name lib_path)
    if(NOT TARGET ${target_name})
        add_library(${target_name} STATIC IMPORTED GLOBAL)
        set_target_properties(${target_name} PROPERTIES
            IMPORTED_LOCATION "${lib_path}"
            INTERFACE_INCLUDE_DIRECTORIES "${PROTO_INCLUDE_DIR}"
        )
        add_dependencies(${target_name} protobuf_external)
    endif()
endmacro()