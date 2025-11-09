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

// Package main provides a simple command line to run the LiteRT-LM.
//
// Example usage:
//
//	bazel run //third_party/odml/litert_lm/go:main -- --model_path=/path/to/model.litertlm
package main

import (
	"flag"
	"fmt"
	"os"

	"litert-lm/go_api/engine"
)

// SampleUsage demonstrates how to use the Go wrapper.
func SampleUsage(modelPath string) error {
	settings, err := engine.NewEngineSettings(modelPath, "cpu")
	if err != nil {
		return fmt.Errorf("failed to create engine settings: %w", err)
	}
	defer settings.Delete()

	settings.SetMaxNumTokens(1024)

	eng, err := engine.NewEngine(settings)
	if err != nil {
		return fmt.Errorf("failed to create engine: %w", err)
	}
	defer eng.Delete()
	conv, err := eng.NewConversation()
	if err != nil {
		return fmt.Errorf("failed to create conversation: %w", err)
	}
	defer conv.Delete()

	prompt := "List the the tallest 10 buildings in the world? "
	message := fmt.Sprintf("{\"role\": \"user\", \"content\": [{\"type\": \"text\", \"text\": \"%s\"}]}", prompt)
	stream, err := conv.SendMessageStream(message)
	if err != nil {
		return fmt.Errorf("failed to start message stream: %w", err)
	}

	fmt.Println("Generated Content:")
	for msg := range stream {
		if msg.Err != nil {
			return fmt.Errorf("stream error: %w", msg.Err)
		}
		fmt.Printf("%s", msg.Chunk)
	}
	fmt.Println()

	return nil
}

func main() {
	modelPath := flag.String("model_path", "", "The path to the .litertlm model file.")
	flag.Parse()

	if *modelPath == "" {
		fmt.Fprintln(os.Stderr, "Error: --model_path is a required flag")
		os.Exit(1)
	}

	if err := SampleUsage(*modelPath); err != nil {
		fmt.Printf("Error: %v\n", err)
	}
}
