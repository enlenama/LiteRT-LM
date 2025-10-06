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
	"path/filepath"
)

// listModels returns a list of available model names and an error if any.
func listModels() ([]string, error) {
	files, err := os.ReadDir(modelDir)
	if err != nil {
		if os.IsNotExist(err) {
			return nil, nil // No models found is not an error.
		}
		return nil, fmt.Errorf("failed to read model directory: %w", err)
	}

	var models []string
	for _, file := range files {
		if file.IsDir() {
			continue
		}
		if ext := filepath.Ext(file.Name()); ext == ".task" || ext == ".litertlm" {
			models = append(models, file.Name())
		}
	}
	return models, nil
}
