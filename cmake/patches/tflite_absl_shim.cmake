if(NOT TARGET LiteRTLM::absl::absl)
    add_library(LiteRTLM::absl::absl INTERFACE IMPORTED GLOBAL)
    set_target_properties(LiteRTLM::absl::absl PROPERTIES 
        INTERFACE_LINK_LIBRARIES "${LITERTLM_ABSL_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${LITERTLM_ABSL_INCLUDE_DIRS}"
    )
    
    set(absl_DIR "${LITERTLM_ABSL_CONFIG_DIR}" CACHE INTERNAL "")
    set(Abseil_FOUND TRUE CACHE INTERNAL "")
endif()