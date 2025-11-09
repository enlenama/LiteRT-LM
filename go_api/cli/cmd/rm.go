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
	"strings"

	"github.com/spf13/cobra"
)

var rmCmd = &cobra.Command{
	Use:   "rm <model_alias_or_filename>",
	Short: "Remove a locally downloaded model by alias or filename",
	Args:  cobra.ExactArgs(1),
	RunE: func(cmd *cobra.Command, args []string) error {
		input := args[0]
		var fileToDelete string
		isInputAlias := false

		// Is input a registry alias?
		if info, ok := modelRegistry[input]; ok {
			fileToDelete = path.Base(info.url)
			isInputAlias = true
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
			return fmt.Errorf("model '%s' not found in local cache or in registry. Please check filename or alias for typos, or use 'lit list --show_all' to see available models", input)
		}

		modelPath := filepath.Join(modelDir, fileToDelete)
		// Check if model file actually exists before trying to delete it
		if _, err := os.Stat(modelPath); err != nil {
			if os.IsNotExist(err) {
				return fmt.Errorf("model file '%s' not found in local cache. Please use 'lit list' to see downloaded models", fileToDelete)
			}
			return fmt.Errorf("error checking for model file '%s': %w", fileToDelete, err)
		}

		fmt.Fprintf(cmd.ErrOrStderr(), "Removing '%s'...\n", modelPath)
		if err := os.Remove(modelPath); err != nil {
			return fmt.Errorf("failed to remove model '%s': %w", fileToDelete, err)
		}

		// Also remove associated cache files, if any.
		// Cache files could be named after the model filename or the alias,
		// using either '_' or '.' as a separator (e.g., model_cache.bin, model.xnnpack_cache).
		cachePatterns := []string{modelPath + "_*", modelPath + ".*"}
		if isInputAlias && input != strings.TrimSuffix(fileToDelete, filepath.Ext(fileToDelete)) {
			aliasPath := filepath.Join(modelDir, input)
			cachePatterns = append(cachePatterns, aliasPath+"_*", aliasPath+".*")
		}

		seenCacheFiles := make(map[string]bool)
		for _, pattern := range cachePatterns {
			cacheFiles, err := filepath.Glob(pattern)
			if err != nil {
				fmt.Fprintf(cmd.ErrOrStderr(), "Warning: Failed to search for cache files with pattern %q: %v\n", pattern, err)
				continue
			}
			for _, cacheFile := range cacheFiles {
				if !seenCacheFiles[cacheFile] {
					fmt.Fprintf(cmd.ErrOrStderr(), "Removing cache file '%s'...\n", cacheFile)
					if err := os.Remove(cacheFile); err != nil {
						fmt.Fprintf(cmd.ErrOrStderr(), "Warning: Failed to remove cache file %q: %v\n", cacheFile, err)
					}
					seenCacheFiles[cacheFile] = true
				}
			}
		}

		fmt.Fprintf(cmd.ErrOrStderr(), "Successfully removed model '%s'.\n", fileToDelete)
		return nil
	},
}
