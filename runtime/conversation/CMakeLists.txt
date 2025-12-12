# Copyright 2026 Google LLC.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set(PKG_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# --- Subdirectories ---
add_subdirectory(model_data_processor)

# --- Conversation ---
add_litertlm_library(runtime_conversation_conversation STATIC
  conversation.cc
)
add_library(LiteRTLM::Runtime::Conversation::Conversation ALIAS runtime_conversation_conversation)

target_include_directories(runtime_conversation_conversation
  PUBLIC
    ${PKG_ROOT}
    ${LITERTLM_INCLUDE_PATHS}
)

target_link_libraries(runtime_conversation_conversation
  PUBLIC
    # Local Dependencies
    LiteRTLM::Runtime::Conversation::InternalCallbackUtil # (Defined below)
    LiteRTLM::Runtime::Conversation::IoTypes # (Defined below)
    LiteRTLM::Runtime::Conversation::Processor::ConfigRegistry
    LiteRTLM::Runtime::Conversation::Processor::Factory
    LiteRTLM::Runtime::Conversation::Processor::Interface

    # Cross-Module Dependencies
    LiteRTLM::Runtime::Components::PromptTemplate
    LiteRTLM::Runtime::Components::ConstrainedDecoding::Constraint
    # runtime_engine_engine_interface (Likely defined in runtime/engine)
    # runtime_engine_engine_settings (Likely defined in runtime/engine)
    # runtime_engine_io_types (Likely defined in runtime/engine)
    runtime_util_litert_status_util

    proto_lib
    LITERTLM_DEPS
    litert_libs
)

# --- Internal Callback Util ---
add_litertlm_library(runtime_conversation_internal_callback_util STATIC
  internal_callback_util.cc
)
add_library(LiteRTLM::Runtime::Conversation::InternalCallbackUtil ALIAS runtime_conversation_internal_callback_util)

target_include_directories(runtime_conversation_internal_callback_util
  PUBLIC
    ${PKG_ROOT}
    ${LITERTLM_INCLUDE_PATHS}
)

target_link_libraries(runtime_conversation_internal_callback_util
  PUBLIC
    LiteRTLM::Runtime::Conversation::IoTypes # (Defined below)
    LiteRTLM::Runtime::Conversation::Processor::ConfigRegistry
    LiteRTLM::Runtime::Conversation::Processor::Interface
    # runtime_engine_io_types (Likely defined in runtime/engine)
    LITERTLM_DEPS
    litert_libs
)

# --- IO Types ---
add_litertlm_library(runtime_conversation_io_types STATIC
  io_types.cc
)
add_library(LiteRTLM::Runtime::Conversation::IoTypes ALIAS runtime_conversation_io_types)

target_include_directories(runtime_conversation_io_types
  PUBLIC
    ${PKG_ROOT}
    ${LITERTLM_INCLUDE_PATHS}
)

target_link_libraries(runtime_conversation_io_types
  PUBLIC 
    LITERTLM_DEPS
    litert_libs
)

# --- Folder Facade ---
add_library(runtime_conversation_libs INTERFACE)
add_library(LiteRTLM::Runtime::Conversation ALIAS runtime_conversation_libs)

target_link_libraries(runtime_conversation_libs INTERFACE
  LiteRTLM::Runtime::Conversation::Conversation
  LiteRTLM::Runtime::Conversation::InternalCallbackUtil
  LiteRTLM::Runtime::Conversation::IoTypes
  LiteRTLM::Runtime::Conversation::Processor
)