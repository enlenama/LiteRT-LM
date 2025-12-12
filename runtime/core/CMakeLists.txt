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

# ==============================================================================
# 1. Tasks
#    Bazel: cc_library(name = "tasks" ...)
#    Status: Lowest level logic. MUST NOT depend on Session or Pipeline.
# ==============================================================================
add_litertlm_library(runtime_core_tasks STATIC
  tasks.cc
)
add_library(LiteRTLM::Runtime::Core::Tasks ALIAS runtime_core_tasks)

target_include_directories(runtime_core_tasks
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LITERTLM_INCLUDE_PATHS}
)

target_link_libraries(runtime_core_tasks
  PUBLIC
    LiteRTLM::Runtime::Components::Sampler::Interface
    LiteRTLM::Runtime::Components::ScoringCpuUtil
    LiteRTLM::Runtime::Components::StopTokenDetector
    LiteRTLM::Runtime::Components::Tokenizer::Interface
    LiteRTLM::Runtime::Components::ConstrainedDecoding::Decoder
    LiteRTLM::Runtime::Components::ConstrainedDecoding::Constraint
    
    runtime_engine_io_types
    
    runtime_executor_llm_executor
    runtime_executor_llm_executor_io_types
    runtime_executor_llm_executor_settings
    runtime_executor_llm_litert_compiled_model_executor
    
    runtime_framework_threadpool
    runtime_util_convert_tensor_buffer
    runtime_util_litert_status_util

    LITERTLM_DEPS
)

# ==============================================================================
# 2. Pipeline
#    Bazel: cc_library(name = "pipeline" ...)
#    Status: Depends on Tasks.
# ==============================================================================
add_litertlm_library(runtime_core_pipeline STATIC
  pipeline.cc
)
add_library(LiteRTLM::Runtime::Core::Pipeline ALIAS runtime_core_pipeline)

target_include_directories(runtime_core_pipeline
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LITERTLM_INCLUDE_PATHS}
)

target_link_libraries(runtime_core_pipeline
  PUBLIC
    runtime_core_tasks
    
    LiteRTLM::Runtime::Components::Sampler::Interface
    LiteRTLM::Runtime::Components::ScoringCpuUtil
    LiteRTLM::Runtime::Components::StopTokenDetector
    LiteRTLM::Runtime::Components::TokenIdUtil
    LiteRTLM::Runtime::Components::Tokenizer::Interface
    LiteRTLM::Runtime::Components::Sampler::TopP
    LiteRTLM::Runtime::Components::ConstrainedDecoding::Decoder
    LiteRTLM::Runtime::Components::ConstrainedDecoding::Constraint
    
    runtime_engine_io_types
    runtime_executor_executor_settings_base
    runtime_executor_llm_executor
    runtime_executor_llm_executor_io_types
    runtime_executor_llm_executor_settings
    runtime_executor_llm_litert_compiled_model_executor
    
    runtime_util_convert_tensor_buffer
    runtime_util_litert_status_util
  
    LITERTLM_DEPS
)

# ==============================================================================
# 3. Session Utils
#    Bazel: cc_library(name = "session_utils" ...)
#    Status: Utility only. MUST NOT depend on SessionBasic.
# ==============================================================================
add_litertlm_library(runtime_core_session_utils STATIC
  session_utils.cc
)
add_library(LiteRTLM::Runtime::Core::SessionUtils ALIAS runtime_core_session_utils)

target_include_directories(runtime_core_session_utils
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LITERTLM_INCLUDE_PATHS}
)

target_link_libraries(runtime_core_session_utils
  PUBLIC
    LiteRTLM::Runtime::Components::Tokenizer::Interface
    runtime_engine_engine_settings
    runtime_engine_io_types
    runtime_util_litert_status_util
  
    litert_libs
)

# ==============================================================================
# 4. Session Basic
#    Bazel: cc_library(name = "session_basic" ...)
#    Status: The core logic. Aggregates Pipeline, Utils, and Tasks.
# ==============================================================================
add_litertlm_library(runtime_core_session_basic STATIC
  session_basic.cc
)
add_library(LiteRTLM::Runtime::Core::SessionBasic ALIAS runtime_core_session_basic)

target_include_directories(runtime_core_session_basic
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LITERTLM_INCLUDE_PATHS}
)

target_link_libraries(runtime_core_session_basic
  PUBLIC
    runtime_core_pipeline
    runtime_core_session_utils
    # Tasks is implicitly linked via pipeline, but we can be explicit
    runtime_core_tasks 
    
    LiteRTLM::Runtime::Components::Sampler::Interface
    LiteRTLM::Runtime::Components::Sampler::Factory
    LiteRTLM::Runtime::Components::StopTokenDetector
    LiteRTLM::Runtime::Components::Tokenizer::Interface
    LiteRTLM::Runtime::Components::ConstrainedDecoding::Constraint
    
    runtime_engine_engine_interface
    runtime_engine_engine_settings
    runtime_engine_io_types
    
    runtime_executor_audio_executor
    runtime_executor_executor_settings_base
    runtime_executor_llm_executor
    runtime_executor_llm_executor_io_types
    runtime_executor_vision_executor
    
    runtime_framework_threadpool
    runtime_util_convert_tensor_buffer
    runtime_util_executor_data_util
    runtime_util_litert_status_util
    runtime_util_model_type_utils
    runtime_util_tensor_buffer_util
  
    LITERTLM_DEPS
)

# ==============================================================================
# 5. Session Factory
#    Bazel: cc_library(name = "session_factory" ...)
# ==============================================================================
add_litertlm_library(runtime_core_session_factory STATIC
  session_factory.cc
)
add_library(LiteRTLM::Runtime::Core::SessionFactory ALIAS runtime_core_session_factory)

target_include_directories(runtime_core_session_factory
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LITERTLM_INCLUDE_PATHS}
)

target_link_libraries(runtime_core_session_factory
  PUBLIC
    runtime_core_session_basic
    LiteRTLM::Runtime::Components::Tokenizer::Interface
    runtime_engine_engine_interface
    runtime_engine_engine_settings
    runtime_engine_io_types
    runtime_executor_audio_executor
    runtime_executor_llm_executor
    runtime_executor_vision_executor
    runtime_framework_threadpool
    runtime_util_litert_status_util
  
    LITERTLM_DEPS
)

# ==============================================================================
# 6. Engine Impl (Common Deps Wrapper)
# ==============================================================================
# NOTE: We don't define a separate target for "COMMON_DEPS", we just link them
# directly to the impl targets below.

# ==============================================================================
# 7. Engine Impl
#    Bazel: cc_library(name = "engine_impl" ...)
# ==============================================================================
add_litertlm_library(runtime_core_engine_impl STATIC
  engine_impl.cc
)
add_library(LiteRTLM::Runtime::Core::EngineImpl ALIAS runtime_core_engine_impl)

target_include_directories(runtime_core_engine_impl
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LITERTLM_INCLUDE_PATHS}
)

target_link_libraries(runtime_core_engine_impl
  PUBLIC
    # --- Common Deps ---
    runtime_core_session_factory
    LiteRTLM::Runtime::Components::ModelResources::Interface
    runtime_engine_engine_interface
    runtime_engine_engine_settings
    runtime_engine_io_types
    runtime_executor_audio_executor
    runtime_executor_audio_executor_settings
    runtime_executor_audio_litert_compiled_model_executor
    runtime_executor_executor_settings_base
    runtime_executor_litert_compiled_model_executor_utils
    runtime_executor_llm_executor
    runtime_executor_llm_executor_settings
    runtime_executor_llm_litert_compiled_model_executor
    runtime_executor_magic_number_configs_helper
    runtime_executor_vision_executor
    runtime_executor_vision_litert_compiled_model_executor
    runtime_framework_threadpool
    runtime_executor_llm_litert_compiled_model_executor_factory
    runtime_util_file_format_util
    runtime_util_litert_status_util
    
    # --- GPU/NPU Specifics (Always link if they exist) ---
    # These are present in the Bazel "engine_impl" target list
    # runtime_executor_default_static_gpu_accelerator
    # LiteRTLM::Runtime::Components::Sampler::GPU
    
  
    LITERTLM_DEPS

)

# ==============================================================================
# 8. Engine Impl (CPU Only)
#    Bazel: cc_library(name = "engine_impl_cpu_only" ...)
# ==============================================================================
add_litertlm_library(runtime_core_engine_impl_cpu_only STATIC
  engine_impl.cc
)
add_library(LiteRTLM::Runtime::Core::EngineImplCPU ALIAS runtime_core_engine_impl_cpu_only)

target_include_directories(runtime_core_engine_impl_cpu_only
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LITERTLM_INCLUDE_PATHS}
)

target_link_libraries(runtime_core_engine_impl_cpu_only
  PUBLIC
    # --- Common Deps Only (No GPU) ---
    runtime_core_session_factory
    LiteRTLM::Runtime::Components::ModelResources::Interface
    runtime_engine_engine_interface
    runtime_engine_engine_settings
    runtime_engine_io_types
    runtime_executor_audio_executor
    runtime_executor_audio_executor_settings
    runtime_executor_audio_litert_compiled_model_executor
    runtime_executor_executor_settings_base
    runtime_executor_litert_compiled_model_executor_utils
    runtime_executor_llm_executor
    runtime_executor_llm_executor_settings
    runtime_executor_llm_litert_compiled_model_executor
    runtime_executor_magic_number_configs_helper
    runtime_executor_vision_executor
    runtime_executor_vision_litert_compiled_model_executor
    runtime_framework_threadpool
    runtime_executor_llm_litert_compiled_model_executor_factory
    runtime_util_file_format_util
    runtime_util_litert_status_util

    LITERTLM_DEPS
)

# ==============================================================================
# 9. Facade
# ==============================================================================
add_library(runtime_core_libs INTERFACE)
add_library(LiteRTLM::Runtime::Core ALIAS runtime_core_libs)

target_link_libraries(runtime_core_libs INTERFACE
  LiteRTLM::Runtime::Core::EngineImpl
  LiteRTLM::Runtime::Core::EngineImplCPU
  LiteRTLM::Runtime::Core::Pipeline
  LiteRTLM::Runtime::Core::SessionBasic
  LiteRTLM::Runtime::Core::SessionFactory
  LiteRTLM::Runtime::Core::SessionUtils
  LiteRTLM::Runtime::Core::Tasks
)