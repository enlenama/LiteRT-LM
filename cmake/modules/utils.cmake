function(verify_install target_name config_path)
    ExternalProject_Add_Step(${target_name} step_verify_install
        COMMAND ${CMAKE_COMMAND} -E echo "Verifying installation..."
        COMMAND ${CMAKE_COMMAND} -DFILE_TO_CHECK=${config_path} -P "${LITERTLM_SCRIPTS_DIR}/verify_install.cmake"
        DEPENDEES install
        COMMENT "Ensuring ${config_path} was actually generated."
    )
endfunction()




function(generate_src_files OUTPUT_CLEAN_PATHS)
    set(RAW_FILES ${ARGN})
    set(CLEANED_PATHS_OUT "")

    foreach(RAW_FILE IN ITEMS ${RAW_FILES})
        get_filename_component(FILE_NAME ${RAW_FILE} NAME)
        get_filename_component(FILE_DIR ${RAW_FILE} DIRECTORY)
        file(RELATIVE_PATH REL_PATH ${PROJECT_ROOT} ${FILE_DIR})

        set(GEN_DIR "${GENERATED_SRC_DIR}/${REL_PATH}")
        file(MAKE_DIRECTORY ${GEN_DIR})

        set(CLEAN_FILE "${GEN_DIR}/${FILE_NAME}") 

        execute_process(
          COMMAND
              sed -e s:odml/litert_lm/::g
                  -e s:odml/litert/::g

              #     -e s:third_party/c/:c/:g
              #     -e s:third_party/runtime/:runtime/:g
              #     -e s:third_party/schema/:schema/:g
              #     -e s:third_party/absl/:third_party/absl/absl/:g
              #     # -e s:json/include/nlohmann/:include/nlohmann/:g
              #     -e s:json/src/json.hpp:json/include/nlohmann/json.hpp:g
              #     -e s:minja/include/minja/google/:minja/include/minja/:g
              #     -e s:third_party/litert/:_deps/litert_lib-src/litert/:g
              #     -e s:litert/litert/:litert/:g
              
          WORKING_DIRECTORY ${PROJECT_ROOT}
          INPUT_FILE ${RAW_FILE}
          OUTPUT_FILE ${CLEAN_FILE}
          # ERROR_FILE "${CLEAN_FILE}_err"
          RESULT_VARIABLE SED_RESULT
        )

        if (SED_RESULT GREATER 0)
            message(FATAL_ERROR "SED failed on file: ${RAW_FILE}")
        endif()

        list(APPEND CLEANED_PATHS_OUT ${ABS_CLEAN_FILE})
    endforeach()
    
    set(${OUTPUT_CLEAN_PATHS} "${CLEANED_PATHS_OUT}" PARENT_SCOPE)
endfunction()





function(generate_protobuf TARGET_NAME)
    set(GENERATED_SRCS)
    set(GENERATED_HDRS)

    foreach(PROTO_FILE ${PROTO_FILES})
        # Calculate the path relative to PROJECT_ROOT
        # e.g., runtime/proto/engine.proto
        file(RELATIVE_PATH REL_PROTO_PATH "${PROJECT_ROOT}" "${PROTO_FILE}")
        
        # Get the directory and filename
        get_filename_component(REL_DIR "${REL_PROTO_PATH}" DIRECTORY)
        get_filename_component(FIL_WE "${REL_PROTO_PATH}" NAME_WE)

        # Construct the expected output paths
        # This forces the output to sit in build/runtime/proto/, not just build/
        set(OUT_DIR "${CMAKE_BINARY_DIR}/${REL_DIR}")
        set(SRC_FILE "${OUT_DIR}/${FIL_WE}.pb.cc")
        set(HDR_FILE "${OUT_DIR}/${FIL_WE}.pb.h")

        # Ensure the output directory exists before protoc runs
        file(MAKE_DIRECTORY "${OUT_DIR}")

        add_custom_command(
            OUTPUT "${SRC_FILE}" "${HDR_FILE}"
            
            # Run protoc with -I set to PROJECT_ROOT.
            # This is critical! It tells protoc that "runtime/proto/..." is the package root.
            COMMAND protobuf::protoc
            ARGS --cpp_out "${CMAKE_BINARY_DIR}" 
                 -I "${PROJECT_ROOT}" 
                 "${PROTO_FILE}"
                 
            DEPENDS "${PROTO_FILE}" protobuf::protoc
            COMMENT "Generating C++ from ${REL_PROTO_PATH}"
            VERBATIM
        )

        list(APPEND GENERATED_SRCS "${SRC_FILE}")
        list(APPEND GENERATED_HDRS "${HDR_FILE}")
    endforeach()

    # Add the generated files to the target
    target_sources(${TARGET_NAME} PRIVATE ${GENERATED_SRCS} ${GENERATED_HDRS})
endfunction()




# function(generate_protobuf target_name)
#   if(NOT TARGET protobuf_external)
#     message(FATAL_ERROR "ExternalProject_Add failed to create protobuf_external or generate_protobuf was called too soon!")
#   endif()

#   include(vendor/protobuf-generate)
#   message(STATUS "Protobuf_INCLUDE_DIR: ${Protobuf_INCLUDE_DIR}")
#   protobuf_generate(
#     TARGET ${target_name}
#     LANGUAGE cpp
#     IMPORT_DIRS ${PROJECT_ROOT} ${Protobuf_INCLUDE_DIR}
#     APPEND_PATH ${Protobuf_INCLUDE_DIR}
#     PROTOC_OUT_DIR ${CMAKE_BINARY_DIR}
#     PROTOS ${PROTO_FILES}
#   )
# endfunction()


# function(compile_flatbuffer_files flatb_files)
#     set(output_dir "${GENERATED_SRC_DIR}/schema/core")

#     file(MAKE_DIRECTORY "${output_dir}")

#     foreach(fbf ${flatb_files})
#         get_filename_component(fbf_name ${fbf} NAME)
        
#         message(STATUS " [FlatBuffers] Generating header for ${fbf_name}...")

#         execute_process(
#             COMMAND flatc --gen-mutable --gen-object-api --cpp -o "${output_dir}/" "${fbf}"
#             WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
#             RESULT_VARIABLE ret_code
#             OUTPUT_VARIABLE flatc_output
#             ERROR_VARIABLE flatc_error
#         )

#         if(NOT "${ret_code}" STREQUAL "0")
#             message(FATAL_ERROR "flatc failed for ${fbf}: ${flatc_error}")
#         endif()

#     endforeach()
# endfunction()


function(compile_flatbuffer_files FBS_FILE)
    get_filename_component(FILE_NAME ${FBS_FILE} NAME_WE)
    get_filename_component(FILE_DIR ${FBS_FILE} DIRECTORY)
    
    # The output file that will be generated
    set(GENERATED_HEADER "${GENERATED_SRC_DIR}/${FILE_DIR}/${FILE_NAME}_generated.h")

    add_custom_command(
        OUTPUT ${GENERATED_HEADER}
        COMMAND ${FLATC_EXECUTABLE} --cpp --gen-object-api --reflect-names --gen-mutable -o "${PROJECT_ROOT}/${FILE_DIR}" "${FBS_FILE}"
        DEPENDS ${FBS_FILE} flatbuffers_external
        COMMENT "Generating C++ header for ${FILE_NAME}.fbs"
    )

    # Allow the header to be added to library targets
    set_source_files_properties(${GENERATED_HEADER} PROPERTIES GENERATED TRUE)
    
    # Return the generated file path so you can add it to your add_library() sources
    set(GENERATED_FLATBUFFER_HEADERS ${GENERATED_FLATBUFFER_HEADERS} ${GENERATED_HEADER} PARENT_SCOPE)
endfunction()