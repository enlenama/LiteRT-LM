if(NOT TARGET LiteRTLM::protobuf::libprotobuf)
    # 1. Define the target TFLite is looking for
    add_library(LiteRTLM::protobuf::libprotobuf INTERFACE IMPORTED GLOBAL)
    set_target_properties(LiteRTLM::protobuf::libprotobuf PROPERTIES 
        INTERFACE_LINK_LIBRARIES "-Wl,--start-group;${LITERTLM_PROTO_LIBRARIES};-Wl,--end-group"
        INTERFACE_INCLUDE_DIRECTORIES "${LITERTLM_PROTO_INCLUDE_DIRS};${LITERTLM_ABSL_INCLUDE_DIRS}"
    )

    # 2. Map standard Protobuf namespaced targets to our shim
    if(NOT TARGET protobuf::libprotobuf)
        add_library(protobuf::libprotobuf ALIAS LiteRTLM::protobuf::libprotobuf)
    endif()
    if(NOT TARGET protobuf::protobuf)
        add_library(protobuf::protobuf ALIAS LiteRTLM::protobuf::libprotobuf)
    endif()

    # 3. Provide the 'Legacy' variables TF still checks for
    set(Protobuf_INCLUDE_DIR "${LITERTLM_PROTO_INCLUDE_DIRS}" CACHE INTERNAL "")
    set(Protobuf_LIBRARIES LiteRTLM::protobuf::libprotobuf CACHE INTERNAL "")
    set(Protobuf_PROTOC_EXECUTABLE "${LITERTLM_PROTOC_EXECUTABLE}" CACHE INTERNAL "")
    
    # 4. Convince TFLite that find_package(Protobuf) already succeeded
    set(Protobuf_FOUND TRUE CACHE INTERNAL "")
    set(PROTOBUF_FOUND TRUE CACHE INTERNAL "")
endif()