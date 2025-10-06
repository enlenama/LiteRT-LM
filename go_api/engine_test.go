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

package engine

import (
	"os"
	"path/filepath"
	"strings"
	"testing"
)

func findTestModelPath(t *testing.T) string {
	// Bazel makes data dependencies available relative to the workspace root.
	path := "runtime/testdata/test_lm_new_metadata.task"
	if _, err := os.Stat(path); err == nil {
		return path
	}
	// If not found, it might be in the runfiles directory.
	if runfiles := os.Getenv("TEST_SRCDIR"); runfiles != "" {
		path = filepath.Join(runfiles, path)
		if _, err := os.Stat(path); err == nil {
			return path
		}
	}
	t.Fatalf("test model not found")
	return ""
}

func TestGenerateContent(t *testing.T) {
	modelPath := findTestModelPath(t)

	settings, err := NewEngineSettings(modelPath, "cpu")
	if err != nil {
		t.Fatalf("NewEngineSettings() failed: %v", err)
	}
	defer settings.Delete()

	settings.SetMaxNumTokens(16)

	eng, err := NewEngine(settings)
	if err != nil {
		t.Fatalf("NewEngine() failed: %v", err)
	}
	defer eng.Delete()

	session, err := eng.CreateSession()
	if err != nil {
		t.Fatalf("CreateSession() failed: %v", err)
	}
	defer session.Delete()

	prompt := "Hello world!"
	responseText, err := session.GenerateContent(prompt)
	if err != nil {
		t.Fatalf("GenerateContent() failed: %v", err)
	}

	// NOTE: The C++ version of this test checks for non-empty response.
	// We align with that behavior here.

	if len(responseText) == 0 {
		t.Error("Got empty response text, want non-empty response text.")
	}
}

func TestGenerateContentStream(t *testing.T) {
	modelPath := findTestModelPath(t)

	settings, err := NewEngineSettings(modelPath, "cpu")
	if err != nil {
		t.Fatalf("NewEngineSettings() failed: %v", err)
	}
	defer settings.Delete()

	settings.SetMaxNumTokens(16)

	eng, err := NewEngine(settings)
	if err != nil {
		t.Fatalf("NewEngine() failed: %v", err)
	}
	defer eng.Delete()

	session, err := eng.CreateSession()
	if err != nil {
		t.Fatalf("CreateSession() failed: %v", err)
	}
	defer session.Delete()

	prompt := "Hello world!"
	stream, err := session.GenerateContentStream(prompt)
	if err != nil {
		t.Fatalf("GenerateContentStream() failed: %v", err)
	}

	var responseBuilder strings.Builder
	var streamErr error
	for msg := range stream {
		if msg.Err != nil {
			streamErr = msg.Err
		} else {
			responseBuilder.WriteString(msg.Chunk)
		}
	}

	// From engine_c_test.cc:
	// This model is too small and generate random output, so the result may be
	// either success or failure due to maximum kv-cache size reached.
	if streamErr != nil {
		if !strings.Contains(streamErr.Error(), "Maximum kv-cache size reached") {
			t.Fatalf("stream error: %v", streamErr)
		}
	}

	if responseBuilder.Len() == 0 {
		t.Error("Got empty response text, want non-empty response text.")
	}
}
