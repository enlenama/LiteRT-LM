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

package cmd

import (
	"bufio"
	"fmt"
	"io"
	"os"
	"path"
	"path/filepath"
	"strings"

	"github.com/spf13/cobra"
	"litert-lm/go_api/engine"
)

var runCmd = &cobra.Command{
	Use:   "run <model>",
	Short: "Run a LiteRT-LM model and start an interactive session",
	Args:  cobra.ExactArgs(1),
	ValidArgsFunction: func(cmd *cobra.Command, args []string, toComplete string) ([]string, cobra.ShellCompDirective) {
		// This provides shell completion for the model argument.
		models := make([]string, 0, len(modelRegistry))
		for model := range modelRegistry {
			if strings.HasPrefix(model, toComplete) {
				models = append(models, model)
			}
		}
		return models, cobra.ShellCompDirectiveNoFileComp
	},
	RunE: func(cmd *cobra.Command, args []string) error {
		benchmark, err := cmd.Flags().GetBool("benchmark")
		if err != nil {
			return fmt.Errorf("failed to get 'benchmark' flag: %w", err)
		}
		userInput := args[0]
		modelName := ""

		// First, check if userInput is a filename in modelDir.
		if _, err := os.Stat(filepath.Join(modelDir, userInput)); err == nil {
			modelName = userInput
		} else if _, ok := modelRegistry[userInput]; ok {
			// If not a file, check for an exact match in the model registry.
			modelName = userInput
		} else {
			// If no exact match, search for a unique partial match.
			var matches []string
			for model := range modelRegistry {
				if strings.Contains(model, userInput) {
					matches = append(matches, model)
				}
			}

			if len(matches) == 1 {
				modelName = matches[0]
				fmt.Printf("Assuming you meant '%s'\n", modelName)
			} else if len(matches) > 1 {
				return fmt.Errorf("ambiguous model name '%s'.\n\nPlease be more specific. It could refer to any of the following:\n  - %s", userInput, strings.Join(matches, "\n  - "))
			}
		}

		// After checking the registry, check the local cache using the same logic.
		if modelName == "" {
			localModels, err := listModels()
			if err != nil {
				return fmt.Errorf("failed to list local models: %w", err)
			}

			var matches []string
			for _, model := range localModels {
				if strings.Contains(model, userInput) {
					matches = append(matches, model)
				}
			}

			if len(matches) == 1 {
				modelName = matches[0]
				fmt.Printf("Assuming you meant '%s' from your local cache\n", modelName)
			} else if len(matches) > 1 {
				return fmt.Errorf("ambiguous model name '%s' in local cache.\n\nPlease be more specific. It could refer to any of the following:\n  - %s", userInput, strings.Join(matches, "\n  - "))
			}
		}

		if modelName == "" {
			return fmt.Errorf("model '%s' not found in the model registry or the local cache", userInput)
		}

		var fileName string
		url, isRegistryAlias := modelRegistry[modelName]
		if isRegistryAlias {
			fileName = path.Base(url)
		} else {
			// If not in registry, assume modelName is already a filename from local cache match.
			fileName = modelName
		}
		modelPath := filepath.Join(modelDir, fileName)

		// Check if the model file exists locally, otherwise prompt to pull it.
		if _, err := os.Stat(modelPath); os.IsNotExist(err) {
			pullHint := modelName
			if !isRegistryAlias {
				// If modelName is a filename and not a registry alias, try to find matching alias for pull hint.
				for alias, regURL := range modelRegistry {
					if path.Base(regURL) == fileName {
						pullHint = alias
						break
					}
				}
			}
			return fmt.Errorf("model '%s' not found in local cache. Please run 'lit pull %s' first", modelName, pullHint)
		}

		verbose, err := cmd.Flags().GetBool("verbose")
		if err != nil {
			return fmt.Errorf("failed to get 'verbose' flag: %w", err)
		}
		logLevel, err := cmd.Flags().GetInt("min_log_level")
		if err != nil {
			return fmt.Errorf("failed to get 'min_log_level' flag: %w", err)
		}
		if verbose {
			fmt.Printf("Loading model '%s'...\n", modelName)
			logLevel = 0 // VERBOSE
		} else if benchmark && !cmd.Flags().Changed("min_log_level") {
			logLevel = 4 // SILENT
		}
		engine.SetMinLogLevel(logLevel)

		backend, err := cmd.Flags().GetString("backend")
		if err != nil {
			return fmt.Errorf("failed to get 'backend' flag: %w", err)
		}
		if verbose {
			fmt.Printf("Using backend: %s\n", backend)
		}

		settings, err := engine.NewEngineSettings(modelPath, backend)
		if err != nil {
			return fmt.Errorf("failed to create engine settings: %w", err)
		}
		defer settings.Delete()

		if benchmark {
			settings.EnableBenchmark()
		}

		// TODO(user): Allow configuring max tokens via flag.
		settings.SetMaxNumTokens(1024)

		eng, err := engine.NewEngine(settings)
		if err != nil {
			return fmt.Errorf("failed to create engine: %w", err)
		}
		defer eng.Delete()

		session, err := eng.CreateSession()
		if err != nil {
			return fmt.Errorf("failed to create session: %w", err)
		}
		defer session.Delete()

		fmt.Printf("Model '%s' loaded. Start chatting (type /bye to exit):\n", modelName)
		reader := bufio.NewReader(os.Stdin)
		for {
			fmt.Print(">>> ")
			input, err := reader.ReadString('\n')
			if err != nil {
				if err == io.EOF {
					fmt.Println("\nExiting.")
					break
				}
				return fmt.Errorf("error reading input: %w", err)
			}
			input = strings.TrimSpace(input)

			if input == "" {
				continue
			}

			if strings.HasPrefix(input, "/") {
				switch input {
				case "/bye":
					fmt.Println("Exiting chat.")
					return nil
				case "/clear":
					fmt.Println("Session context cleared. (Not yet implemented)")
					// TODO(b/443324802): Implement session clearing logic if needed by the engine.
				case "/show":
					fmt.Printf("Model: %s\n", modelName)
					fmt.Println("Backend: cpu")
					// TODO(b/443324802): Add more model info if available.
				case "/?", "/help":
					fmt.Println("Available Commands:")
					fmt.Println("  /show           Show model information")
					fmt.Println("  /clear          Clear session context")
					fmt.Println("  /bye            Exit")
					fmt.Println("  /?, /help       Help for a command")
				default:
					fmt.Printf("Unknown command: %s\n", input)
				}
				continue
			}

			// Generate content from the model.
			stream, err := session.GenerateContentStream(input)
			if err != nil {
				fmt.Printf("Error starting stream: %v\n", err)
				continue
			}

			// Print streamed response
			for msg := range stream {
				if msg.Err != nil {
					fmt.Printf("Stream error: %v\n", msg.Err)
					break
				}
				fmt.Printf("%s", msg.Chunk)
			}
			fmt.Println() // Newline after stream ends

			if benchmark {
				info, err := session.BenchmarkInfo()
				if err != nil {
					fmt.Printf("Error getting benchmark info: %v\n", err)
					continue
				}
				fmt.Println("---")
				fmt.Println("Benchmark:")
				fmt.Printf("  Time to first token: %.2fs\n", info.TimeToFirstToken)
				if n := len(info.PrefillTokensPerSec); n > 0 {
					fmt.Printf("  Prefill turn %d: %.2f tokens/sec\n", n-1, info.PrefillTokensPerSec[n-1])
				}
				if n := len(info.DecodeTokensPerSec); n > 0 {
					fmt.Printf("  Decode turn %d: %.2f tokens/sec\n", n-1, info.DecodeTokensPerSec[n-1])
				}
				fmt.Println("---")
			}
		}
		return nil
	},
}
