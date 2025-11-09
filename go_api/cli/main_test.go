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

package main

import (
	"errors"
	"io"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"testing"
)

// setupTestEnv creates a temporary directory to act as the user's home,
// ensuring tests are hermetic and don't use the real model cache.
func setupTestEnv(t *testing.T) (string, func()) {
	t.Helper()
	tempHome, err := os.MkdirTemp("", "lit-cli-test-home")
	if err != nil {
		t.Fatalf("Failed to create temp home dir: %v", err)
	}
	modelDir := filepath.Join(tempHome, ".litert-lm", "models")
	if err := os.MkdirAll(modelDir, 0755); err != nil {
		t.Fatalf("Failed to create temp model dir: %v", err)
	}

	// Override the HOME environment variable for the duration of the test,
	// which the lit binary uses to determine the model cache directory.
	t.Setenv("HOME", tempHome)

	cleanup := func() {
		os.RemoveAll(tempHome)
	}
	return modelDir, cleanup
}

// getLitBinaryPath finds the compiled 'lit' binary in the test environment.
func getLitBinaryPath(t *testing.T) string {
	t.Helper()
	litPath, ok := os.LookupEnv("TEST_SRCDIR")
	if !ok {
		t.Fatal("TEST_SRCDIR not set")
	}
	return filepath.Join(litPath, "go_api/cli/lit")
}

func TestListCmd(t *testing.T) {
	modelDir, cleanup := setupTestEnv(t)
	defer cleanup()

	litPath := getLitBinaryPath(t)

	// 1. Test with no models in the cache.
	cmd := exec.Command(litPath, "list")
	output, err := cmd.CombinedOutput()
	if err != nil {
		t.Fatalf("failed to run 'list' command: %v\n%s", err, output)
	}
	if !strings.Contains(string(output), "No models found.") {
		t.Errorf("Expected a message about no model files, but got: %s", output)
	}

	// 2. Add a dummy model and test again.
	dummyModelPath := filepath.Join(modelDir, "dummy.litertlm")
	if err := os.WriteFile(dummyModelPath, []byte(""), 0644); err != nil {
		t.Fatalf("Failed to create dummy model file: %v", err)
	}

	cmd = exec.Command(litPath, "list")
	output, err = cmd.CombinedOutput()
	if err != nil {
		t.Fatalf("failed to run 'list' command: %v\n%s", err, output)
	}
	if !strings.Contains(string(output), "dummy.litertlm") {
		t.Errorf("Expected to see 'dummy.litertlm' in list, got: %s", output)
	}
}

func TestRmCmd(t *testing.T) {
	modelDir, cleanup := setupTestEnv(t)
	defer cleanup()

	litPath := getLitBinaryPath(t)
	const modelName = "test-model.litertlm"
	modelPath := filepath.Join(modelDir, modelName)

	// 1. Test removing a non-existent model.
	cmd := exec.Command(litPath, "rm", "non-existent-model.litertlm")
	output, err := cmd.CombinedOutput()
	if err == nil {
		t.Fatalf("Expected 'rm' command to fail for non-existent model, but it succeeded.\nOutput: %s", output)
	}
	if !strings.Contains(string(output), "not found") {
		t.Errorf("Expected 'not found' error, got: %s", output)
	}

	// 2. Create a dummy model file and test removing it.
	if err := os.WriteFile(modelPath, []byte("dummy data"), 0644); err != nil {
		t.Fatalf("Failed to create dummy model: %v", err)
	}

	cmd = exec.Command(litPath, "rm", modelName)
	output, err = cmd.CombinedOutput()
	if err != nil {
		t.Fatalf("failed to run 'rm' command: %v\n%s", err, output)
	}
	if !strings.Contains(string(output), "Successfully removed model") {
		t.Errorf("Expected success message, got: %s", output)
	}
	if _, err := os.Stat(modelPath); !os.IsNotExist(err) {
		t.Error("Model file should have been removed, but it still exists")
	}
}

func TestPullModel(t *testing.T) {
	modelDir, cleanup := setupTestEnv(t)
	defer cleanup()

	litPath := getLitBinaryPath(t)

	// This model is in the model registry but the URL will fail (401),
	// which allows us to test the fallback-to-local logic.
	const modelName = "test_lm.litertlm"
	modelPath := filepath.Join(modelDir, modelName)

	// Ensure the model exists in testdata for the fallback to work.
	testDataPath, ok := os.LookupEnv("TEST_SRCDIR")
	if !ok {
		t.Fatal("TEST_SRCDIR not set")
	}
	srcModelPath := filepath.Join(testDataPath, "runtime/testdata", modelName)
	if _, err := os.Stat(srcModelPath); os.IsNotExist(err) {
		t.Skipf("model file not found in testdata, skipping test: %s", srcModelPath)
	}

	cmd := exec.Command(litPath, "pull", modelName)
	output, err := cmd.CombinedOutput()
	if err != nil {
		t.Fatalf("failed to pull model: %v\n%s", err, output)
	}

	if !strings.Contains(string(output), "Copied model") {
		t.Errorf("Expected to see 'Copied model' message, got: %s", output)
	}

	if _, err := os.Stat(modelPath); os.IsNotExist(err) {
		t.Fatalf("model file not found in cache after pulling: %s", modelPath)
	}
}

func TestRunModel(t *testing.T) {
	_, cleanup := setupTestEnv(t)
	defer cleanup()

	litPath := getLitBinaryPath(t)

	// Use the model available in testdata. The `run` command should find it.
	const modelName = "test_lm.litertlm"

	// Ensure the model exists in testdata so `lit run` can find it.
	testDataPath, ok := os.LookupEnv("TEST_SRCDIR")
	if !ok {
		t.Fatal("TEST_SRCDIR not set")
	}
	srcModelPath := filepath.Join(testDataPath, "runtime/testdata", modelName)
	if _, err := os.Stat(srcModelPath); os.IsNotExist(err) {
		t.Skipf("model file not found in testdata, skipping test: %s", srcModelPath)
	}

	cmd := exec.Command(litPath, "run", modelName)

	// The 'run' command is interactive. We pipe '/bye' to its stdin to make it
	// exit gracefully, allowing us to test that it starts up correctly.
	stdin, err := cmd.StdinPipe()
	if err != nil {
		t.Fatal(err)
	}

	go func() {
		defer stdin.Close()
		io.WriteString(stdin, "/bye\n")
	}()

	output, err := cmd.CombinedOutput()
	if err != nil {
		// The interactive session might exit with a non-zero code. This is not
		// an error as long as we see the graceful exit message.
		var exitErr *exec.ExitError
		if !errors.As(err, &exitErr) {
			t.Fatalf("failed to run model with an unexpected error type: %v\n%s", err, output)
		}
	}

	// The command should exit gracefully, even if there is an exit error.
	if !strings.Contains(string(output), "Exiting chat.") {
		t.Fatalf("failed to run model; command exited without the graceful exit message: %v\n%s", err, output)
	}

	if !strings.Contains(string(output), "Model 'test_lm.litertlm' loaded.") {
		t.Errorf("Expected 'Model ... loaded' message, got: %s", output)
	}
}

func TestRunModelWithBenchmark(t *testing.T) {
	_, cleanup := setupTestEnv(t)
	defer cleanup()

	litPath := getLitBinaryPath(t)

	// Use the model available in testdata. The `run` command should find it.
	const modelName = "test_lm.litertlm"

	// Ensure the model exists in testdata so `lit run` can find it.
	testDataPath, ok := os.LookupEnv("TEST_SRCDIR")
	if !ok {
		t.Fatal("TEST_SRCDIR not set")
	}
	srcModelPath := filepath.Join(testDataPath, "runtime/testdata", modelName)
	if _, err := os.Stat(srcModelPath); os.IsNotExist(err) {
		t.Skipf("model file not found in testdata, skipping test: %s", srcModelPath)
	}

	cmd := exec.Command(litPath, "run", modelName, "--benchmark")

	// The 'run' command is interactive. We pipe a prompt and then '/bye' to
	// its stdin to make it exit gracefully.
	stdin, err := cmd.StdinPipe()
	if err != nil {
		t.Fatal(err)
	}

	go func() {
		defer stdin.Close()
		io.WriteString(stdin, "hello\n")
		io.WriteString(stdin, "/bye\n")
	}()

	output, err := cmd.CombinedOutput()
	if err != nil {
		// The interactive session might exit with a non-zero code. This is not
		// an error as long as we see the graceful exit message.
		var exitErr *exec.ExitError
		if !errors.As(err, &exitErr) {
			t.Fatalf("failed to run model with an unexpected error type: %v\n%s", err, output)
		}
	}

	// The command should exit gracefully, even if there is an exit error.
	if !strings.Contains(string(output), "Exiting chat.") {
		t.Fatalf("failed to run model; command exited without the graceful exit message: %v\n%s", err, output)
	}

	// Print the output to the terminal for visibility.
	t.Logf("Command output:\n%s", output)

	if !strings.Contains(string(output), "Benchmark:") {
		t.Errorf("Expected 'Benchmark:' in output, got: %s", output)
	}
}
