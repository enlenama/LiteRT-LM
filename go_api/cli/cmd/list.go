// Copyright 2025 The ODML Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//	http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Package cmd implements the command line interface for lit.
package cmd

import (
	"fmt"
	"os"
	"path"
	"path/filepath"
	"sort"
	"strings"

	"github.com/spf13/cobra"
)

func init() {
	listCmd.Flags().Bool("show_all", false, "List all models available for download in model registry")
}

var listCmd = &cobra.Command{
	Use:   "list",
	Short: "List locally downloaded models",
	RunE: func(cmd *cobra.Command, args []string) error {
		showAll, err := cmd.Flags().GetBool("show_all")
		if err != nil {
			return err
		}

		if showAll {
			fmt.Println("Available models for download:")
			fmt.Printf("%-20s %-70s\n", "ALIAS", "MODEL")

			// Sort aliases for consistent output
			aliases := make([]string, 0, len(modelRegistry))
			for alias := range modelRegistry {
				aliases = append(aliases, alias)
			}
			sort.Strings(aliases)

			for _, alias := range aliases {
				url := modelRegistry[alias]
				fileName := path.Base(url)
				fmt.Printf("%-20s %-70s\n", alias, fileName)
			}
			return nil
		}

		fmt.Printf("Listing models in: %s\n", modelDir)

		fileNameToAlias := make(map[string]string)
		for alias, url := range modelRegistry {
			fileName := path.Base(url)
			fileNameToAlias[fileName] = alias
		}

		models, err := listModels()
		if err != nil {
			return err
		}

		if len(models) == 0 {
			fmt.Println("No models found. Use 'lit pull <model>' to download models.")
			fmt.Println("\nUse 'lit list --show_all' to see all available models for download.")
			return nil
		}

		// Print table header.
		fmt.Printf("%-20s %-70s %-15s %-20s\n", "ALIAS", "MODEL", "SIZE", "MODIFIED")
		for _, model := range models {
			modelPath := filepath.Join(modelDir, model)
			fileInfo, err := os.Stat(modelPath)
			if err != nil {
				// If we can't get file info, just print the name and dashes for other fields.
				fmt.Printf("%-20s %-70s %-15s %-20s\n", "-", model, "-", "-")
				continue
			}

			alias, ok := fileNameToAlias[model]
			if !ok {
				alias = strings.TrimSuffix(model, filepath.Ext(model))
			}

			// Format the size to be human-readable.
			size := humanizeBytes(fileInfo.Size())
			modTime := fileInfo.ModTime().Format("2006-01-02 15:04:05")

			fmt.Printf("%-20s %-70s %-15s %-20s\n", alias, model, size, modTime)
		}
		fmt.Println("\nUse 'lit list --show_all' to see all available models for download.")
		return nil
	},
}

// humanizeBytes converts bytes to a human-readable string (KB, MB, GB).
func humanizeBytes(b int64) string {
	const unit = 1024
	if b < unit {
		return fmt.Sprintf("%d B", b)
	}
	div, exp := int64(unit), 0
	for n := b / unit; n >= unit; n /= unit {
		div *= unit
		exp++
	}
	return fmt.Sprintf("%.1f %cB", float64(b)/float64(div), "KMGTPE"[exp])
}
