// Copyright 2025 The ODML Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Package engine provides a Go wrapper for the LiteRtLmEngine.
package engine

// #cgo windows CFLAGS: -I${SRCDIR}/../c -DLITERT_LOG=litert_lm_log
// #cgo linux CFLAGS: -I${SRCDIR}/../c -DLITERT_LOG=litert_lm_log
// #cgo darwin CFLAGS: -I${SRCDIR}/../c -DLITERT_LOG=litert_lm_log
// #include "c/engine.h"
// #include "c/litert_lm_logging.h"
// #include <stdlib.h>
//
// // Forward declaration of the Go callback function.
// void streamCallback(void* userData, char* chunk, _Bool isFinal, char* errorMsg);
import "C"
import (
	"errors"
	"fmt"
	"runtime"
	"sync"
	"unsafe"
)

func init() {
	// By default, set the log level to FATAL to suppress INFO, WARNING and ERROR logs.
	// This can be overridden by the command-line flags.
	SetMinLogLevel(3)
}

// SetMinLogLevel sets the minimum log level for the LiteRT LM library.
// Log levels are: 0=INFO, 1=WARNING, 2=ERROR, 3=FATAL.
func SetMinLogLevel(level int) {
	C.litert_lm_set_min_log_level(C.int(level))
}

// Settings is a Go wrapper for the LiteRtLmEngineSettings.
type Settings struct {
	cSettings *C.LiteRtLmEngineSettings
}

// NewEngineSettings creates new Settings.
func NewEngineSettings(modelPath, backend string) (*Settings, error) {
	cModelPath := C.CString(modelPath)
	defer C.free(unsafe.Pointer(cModelPath))
	cBackend := C.CString(backend)
	defer C.free(unsafe.Pointer(cBackend))

	cSettings := C.litert_lm_engine_settings_create(cModelPath, cBackend)
	if cSettings == nil {
		return nil, errors.New("failed to create engine settings, most likely due to device memory size limit. Please try a smaller model or use `--verbose` to see more details of the error message")
	}
	settings := &Settings{cSettings: cSettings}
	runtime.SetFinalizer(settings, (*Settings).Delete)
	return settings, nil
}

// Delete frees the underlying C engine settings.
func (s *Settings) Delete() {
	if s.cSettings != nil {
		C.litert_lm_engine_settings_delete(s.cSettings)
		s.cSettings = nil
	}
}

// SetMaxNumTokens sets the maximum number of tokens.
func (s *Settings) SetMaxNumTokens(maxNumTokens int) {
	C.litert_lm_engine_settings_set_max_num_tokens(s.cSettings, C.int(maxNumTokens))
}

// EnableBenchmark enables benchmarking for the engine.
func (s *Settings) EnableBenchmark() {
	C.litert_lm_engine_settings_enable_benchmark(s.cSettings)
}

// Engine is a Go wrapper for the LiteRtLmEngine.
type Engine struct {
	cEngine *C.LiteRtLmEngine
}

// NewEngine creates a new Engine.
func NewEngine(settings *Settings) (*Engine, error) {
	cEngine := C.litert_lm_engine_create(settings.cSettings)
	if cEngine == nil {
		return nil, errors.New("failed to create engine")
	}
	engine := &Engine{cEngine: cEngine}
	runtime.SetFinalizer(engine, (*Engine).Delete)
	return engine, nil
}

// Delete frees the underlying C engine.
func (e *Engine) Delete() {
	if e.cEngine != nil {
		C.litert_lm_engine_delete(e.cEngine)
		e.cEngine = nil
	}
}

// Session is a Go wrapper for the LiteRtLmSession.
type Session struct {
	cSession *C.LiteRtLmSession
}

// CreateSession creates a new Session from an Engine.
func (e *Engine) CreateSession() (*Session, error) {
	cSession := C.litert_lm_engine_create_session(e.cEngine)
	if cSession == nil {
		return nil, errors.New("failed to create session")
	}
	session := &Session{cSession: cSession}
	runtime.SetFinalizer(session, (*Session).Delete)
	return session, nil
}

// Delete frees the underlying C session.
func (s *Session) Delete() {
	if s.cSession != nil {
		C.litert_lm_session_delete(s.cSession)
		s.cSession = nil
	}
}

// StreamMessage holds a chunk of text or an error from the stream.
type StreamMessage struct {
	Chunk string
	Err   error
}

// Conversation is a Go wrapper for the LiteRtLmConversation.
type Conversation struct {
	cConversation *C.LiteRtLmConversation
}

// JSONResponse is a Go wrapper for the LiteRtLmJsonResponse.
type JSONResponse struct {
	cJSONResponse *C.LiteRtLmJsonResponse
}

var (
	streamChannels      = make(map[uintptr]chan<- StreamMessage)
	streamChannelsMutex = &sync.Mutex{}
	nextStreamID        = uintptr(0)
)

//export streamCallback
func streamCallback(userData unsafe.Pointer, chunk *C.char, isFinal C._Bool, errorMsg *C.char) {
	streamID := uintptr(userData)

	streamChannelsMutex.Lock()
	ch, ok := streamChannels[streamID]
	streamChannelsMutex.Unlock()

	if !ok {
		return
	}

	if errorMsg != nil {
		ch <- StreamMessage{Err: errors.New(C.GoString(errorMsg))}
	} else if chunk != nil {
		ch <- StreamMessage{Chunk: C.GoString(chunk)}
	}

	if isFinal {
		streamChannelsMutex.Lock()
		// Channel is removed, and we can close it from the calling side.
		delete(streamChannels, streamID)
		streamChannelsMutex.Unlock()
		close(ch)
	}
}

// NewConversation creates a new Conversation from an Engine.
func (e *Engine) NewConversation() (*Conversation, error) {
	cConversation := C.litert_lm_conversation_create(e.cEngine)
	if cConversation == nil {
		return nil, errors.New("failed to create conversation")
	}
	conversation := &Conversation{cConversation: cConversation}
	runtime.SetFinalizer(conversation, (*Conversation).Delete)
	return conversation, nil
}

// Delete frees the underlying C conversation.
func (c *Conversation) Delete() {
	if c.cConversation != nil {
		C.litert_lm_conversation_delete(c.cConversation)
		c.cConversation = nil
	}
}

// SendMessage sends a message to the conversation and returns the response.
// This is a blocking call.
func (c *Conversation) SendMessage(messageJSON string) (*JSONResponse, error) {
	cMessageJSON := C.CString(messageJSON)
	defer C.free(unsafe.Pointer(cMessageJSON))

	cJSONResponse := C.litert_lm_conversation_send_message(c.cConversation, cMessageJSON)
	if cJSONResponse == nil {
		return nil, errors.New("failed to send message")
	}
	response := &JSONResponse{cJSONResponse: cJSONResponse}
	runtime.SetFinalizer(response, (*JSONResponse).Delete)
	return response, nil
}

// SendMessageStream sends a message to the conversation and streams the response.
// This is a non-blocking call.
func (c *Conversation) SendMessageStream(messageJSON string) (<-chan StreamMessage, error) {
	cMessageJSON := C.CString(messageJSON)
	defer C.free(unsafe.Pointer(cMessageJSON))

	ch := make(chan StreamMessage)

	streamChannelsMutex.Lock()
	streamID := nextStreamID
	nextStreamID++
	streamChannels[streamID] = ch
	streamChannelsMutex.Unlock()

	ret := C.litert_lm_conversation_send_message_stream(
		c.cConversation,
		cMessageJSON,
		(C.LiteRtLmStreamCallback)(C.streamCallback),
		unsafe.Pointer(streamID),
	)

	if ret != 0 {
		streamChannelsMutex.Lock()
		delete(streamChannels, streamID)
		streamChannelsMutex.Unlock()
		close(ch)
		return nil, fmt.Errorf("failed to start message stream with code %d", ret)
	}

	return ch, nil
}

// BenchmarkInfo returns the benchmark info from the conversation.
func (c *Conversation) BenchmarkInfo() (*BenchmarkInfo, error) {
	cBenchmarkInfo := C.litert_lm_conversation_get_benchmark_info(c.cConversation)
	if cBenchmarkInfo == nil {
		return nil, errors.New("failed to get benchmark info")
	}
	defer C.litert_lm_benchmark_info_delete(cBenchmarkInfo)

	info := &BenchmarkInfo{
		TimeToFirstToken: float64(C.litert_lm_benchmark_info_get_time_to_first_token(cBenchmarkInfo)),
	}

	prefillTurns := int(C.litert_lm_benchmark_info_get_num_prefill_turns(cBenchmarkInfo))
	info.PrefillTokensPerSec = make([]float64, prefillTurns)
	for i := 0; i < prefillTurns; i++ {
		info.PrefillTokensPerSec[i] = float64(C.litert_lm_benchmark_info_get_prefill_tokens_per_sec_at(cBenchmarkInfo, C.int(i)))
	}

	decodeTurns := int(C.litert_lm_benchmark_info_get_num_decode_turns(cBenchmarkInfo))
	info.DecodeTokensPerSec = make([]float64, decodeTurns)
	for i := 0; i < decodeTurns; i++ {
		info.DecodeTokensPerSec[i] = float64(C.litert_lm_benchmark_info_get_decode_tokens_per_sec_at(cBenchmarkInfo, C.int(i)))
	}

	return info, nil
}

// Delete frees the underlying C json response.
func (r *JSONResponse) Delete() {
	if r.cJSONResponse != nil {
		C.litert_lm_json_response_delete(r.cJSONResponse)
		r.cJSONResponse = nil
	}
}

// String returns the JSON response as a string.
func (r *JSONResponse) String() string {
	cStr := C.litert_lm_json_response_get_string(r.cJSONResponse)
	if cStr == nil {
		return ""
	}
	return C.GoString(cStr)
}

// GenerateContent generates content from a prompt.
func (s *Session) GenerateContent(prompt string) (string, error) {
	cPrompt := C.CString(prompt)
	defer C.free(unsafe.Pointer(cPrompt))

	inputData := C.InputData{
		_type: C.kInputText,
		data:  unsafe.Pointer(cPrompt),
		size:  C.size_t(len(prompt)),
	}

	cResponses := C.litert_lm_session_generate_content(s.cSession, &inputData, 1)
	if cResponses == nil {
		return "", errors.New("failed to generate content")
	}
	defer C.litert_lm_responses_delete(cResponses)

	if C.litert_lm_responses_get_num_candidates(cResponses) == 0 {
		return "", errors.New("no response candidates generated")
	}

	cText := C.litert_lm_responses_get_response_text_at(cResponses, 0)
	if cText == nil {
		return "", errors.New("failed to get response text")
	}
	return C.GoString(cText), nil
}

// GenerateContentStream starts a non-blocking stream for generating content.
// It returns a channel that will receive stream messages. The channel will be
// closed by the wrapper when the stream is complete.
func (s *Session) GenerateContentStream(prompt string) (<-chan StreamMessage, error) {
	cPrompt := C.CString(prompt)
	defer C.free(unsafe.Pointer(cPrompt))

	inputData := C.InputData{
		_type: C.kInputText,
		data:  unsafe.Pointer(cPrompt),
		size:  C.size_t(len(prompt)),
	}

	ch := make(chan StreamMessage)

	streamChannelsMutex.Lock()
	streamID := nextStreamID
	nextStreamID++
	streamChannels[streamID] = ch
	streamChannelsMutex.Unlock()

	ret := C.litert_lm_session_generate_content_stream(
		s.cSession,
		&inputData,
		1,
		(C.LiteRtLmStreamCallback)(C.streamCallback),
		unsafe.Pointer(streamID),
	)

	if ret != 0 {
		streamChannelsMutex.Lock()
		delete(streamChannels, streamID)
		streamChannelsMutex.Unlock()
		close(ch)
		return nil, fmt.Errorf("failed to start stream with code %d", ret)
	}

	return ch, nil
}

// BenchmarkInfo holds the benchmark data.
type BenchmarkInfo struct {
	TimeToFirstToken    float64
	PrefillTokensPerSec []float64
	DecodeTokensPerSec  []float64
}

// BenchmarkInfo returns the benchmark info from the session.
func (s *Session) BenchmarkInfo() (*BenchmarkInfo, error) {
	cBenchmarkInfo := C.litert_lm_session_get_benchmark_info(s.cSession)
	if cBenchmarkInfo == nil {
		return nil, errors.New("failed to get benchmark info")
	}
	defer C.litert_lm_benchmark_info_delete(cBenchmarkInfo)

	info := &BenchmarkInfo{
		TimeToFirstToken: float64(C.litert_lm_benchmark_info_get_time_to_first_token(cBenchmarkInfo)),
	}

	prefillTurns := int(C.litert_lm_benchmark_info_get_num_prefill_turns(cBenchmarkInfo))
	info.PrefillTokensPerSec = make([]float64, prefillTurns)
	for i := 0; i < prefillTurns; i++ {
		info.PrefillTokensPerSec[i] = float64(C.litert_lm_benchmark_info_get_prefill_tokens_per_sec_at(cBenchmarkInfo, C.int(i)))
	}

	decodeTurns := int(C.litert_lm_benchmark_info_get_num_decode_turns(cBenchmarkInfo))
	info.DecodeTokensPerSec = make([]float64, decodeTurns)
	for i := 0; i < decodeTurns; i++ {
		info.DecodeTokensPerSec[i] = float64(C.litert_lm_benchmark_info_get_decode_tokens_per_sec_at(cBenchmarkInfo, C.int(i)))
	}

	return info, nil
}
