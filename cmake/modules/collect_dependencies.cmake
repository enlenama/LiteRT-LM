add_library(LITERTLM_DEPS INTERFACE)
add_dependencies(LITERTLM_DEPS litert_external)

target_link_libraries(LITERTLM_DEPS INTERFACE
    litert_libs           # Depends on TFLite, Abseil, FlatBuffers
    tflite_libs           # Depends on Abseil, FlatBuffers, Ruy
    
    sentencepiece_libs    # Depends on Abseil, Protobuf
    tokenizers_lib        # Depends on SentencePiece
    re2_libs              # Depends on Abseil

    opencl_headers_lib
    libpng_lib
    kissfft_lib
    miniaudio_lib
    minizip_lib
    minja_lib
    zlib_lib

    json_lib
    proto_lib
    flatbuffers_libs
    absl_libs
)

target_include_directories(LITERTLM_DEPS SYSTEM INTERFACE
    ${LITERTLM_INCLUDE_PATHS}
)