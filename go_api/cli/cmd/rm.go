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
	"fmt"
	"os"
	"path"
	"path/filepath"

	"github.com/spf13/cobra"
)

var rmCmd = &cobra.Command{
	Use:   "rm <model_alias_or_filename>",
	Short: "Remove a locally downloaded model by alias or filename",
	Args:  cobra.ExactArgs(1),
	RunE: func(cmd *cobra.Command, args []string) error {
		input := args[0]
		var fileToDelete string

		// Is input a registry alias?
		if url, ok := modelRegistry[input]; ok {
			fileToDelete = path.Base(url)
		} else {
			// Is input a filename?
			if _, err := os.Stat(filepath.Join(modelDir, input)); err == nil {
				fileToDelete = input
			} else if os.IsNotExist(err) {
				// If not a filename and has no extension, check for alias.ext
				if filepath.Ext(input) == "" {
					for _, ext := range []string{".litertlm", ".task"} {
						fileName := input + ext
						if _, err := os.Stat(filepath.Join(modelDir, fileName)); err == nil {
							fileToDelete = fileName
							break
						}
					}
				}
			} else {
				// Some other error with stating input as a filename.
				return fmt.Errorf("error checking for model file '%s': %w", input, err)
			}
		}

		if fileToDelete == "" {
			return fmt.Errorf("model '%s' not found in local cache or in registry", input)
		}

		modelPath := filepath.Join(modelDir, fileToDelete)
		fmt.Printf("Removing '%s'...\n", modelPath)
		if err := os.Remove(modelPath); err != nil {
			return fmt.Errorf("failed to remove model '%s': %w", fileToDelete, err)
		}
		fmt.Printf("Successfully removed model '%s'.\n", fileToDelete)
		return nil
	},
}
