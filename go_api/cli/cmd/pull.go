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
	"io"
	"net/http"
	"os"
	"path"
	"path/filepath"
	"strings"

	"github.com/spf13/cobra"
)

// TODO(user): Move this to a config file.
type modelInfo struct {
	url  string
	size string
}

var modelRegistry = map[string]modelInfo{
	"gemma-3n-E2B": {url: "https://huggingface.co/google/gemma-3n-E2B-it-litert-lm/resolve/main/gemma-3n-E2B-it-int4.litertlm", size: "3.2 GB"},
	"gemma-3n-E4B": {url: "https://huggingface.co/google/gemma-3n-E4B-it-litert-lm/resolve/main/gemma-3n-E4B-it-int4.litertlm", size: "4.3 GB"},
	"gemma3-1b":    {url: "https://huggingface.co/litert-community/Gemma3-1B-IT/resolve/main/gemma3-1b-it-int4.litertlm", size: "1.2 GB"},
	"qwen2.5-1.5b": {url: "https://huggingface.co/litert-community/Qwen2.5-1.5B-Instruct/resolve/main/Qwen2.5-1.5B-Instruct_multi-prefill-seq_q8_ekv4096.litertlm", size: "1.5 GB"},
	"phi-4-mini":   {url: "https://huggingface.co/litert-community/Phi-4-mini-instruct/resolve/main/Phi-4-mini-instruct_multi-prefill-seq_q8_ekv4096.litertlm", size: "3.6 GB"},
}

var pullCmd = &cobra.Command{
	Use:   "pull <model_or_url>",
	Short: "Download a model from registry or URL (e.g., Hugging Face)",
	Args:  cobra.ExactArgs(1),
	RunE: func(cmd *cobra.Command, args []string) error {
		input := args[0]
		var downloadURL string
		modelName := input

		info, inRegistry := modelRegistry[input]
		if inRegistry {
			downloadURL = info.url
		} else if strings.HasPrefix(input, "http://") || strings.HasPrefix(input, "https://") {
			downloadURL = input
			modelName = path.Base(downloadURL)
		} else {
			return fmt.Errorf("model '%s' not found in registry. If you are providing a URL, it must start with http:// or https://", input)
		}

		alias, _ := cmd.Flags().GetString("alias")
		if alias != "" && inRegistry {
			return fmt.Errorf("--alias flag cannot be used with models from model registry")
		}

		fmt.Printf("Downloading model '%s' from '%s'...\n", modelName, downloadURL)

		// Create model directory if it doesn't exist.
		if err := os.MkdirAll(modelDir, 0755); err != nil {
			return fmt.Errorf("failed to create model directory: %w", err)
		}

		// Get token from flag or environment variable.
		token, _ := cmd.Flags().GetString("hf_token")
		if token == "" {
			token = os.Getenv("HUGGING_FACE_HUB_TOKEN")
		}

		// Create a new request.
		req, err := http.NewRequest("GET", downloadURL, nil)
		if err != nil {
			return fmt.Errorf("failed to create request: %w", err)
		}

		if token != "" {
			req.Header.Add("Authorization", "Bearer "+token)
		}

		// Download the model.
		client := &http.Client{}
		resp, err := client.Do(req)
		if err != nil {
			return fmt.Errorf("failed to download model: %w", err)
		}
		defer resp.Body.Close()

		if resp.StatusCode != http.StatusOK {
			if resp.StatusCode == http.StatusUnauthorized || resp.StatusCode == http.StatusForbidden {
				if token == "" {
					fmt.Fprintln(cmd.ErrOrStderr(), "Authorization failed. This model likely requires a Hugging Face Hub token, but none was provided.")
					fmt.Fprintln(cmd.ErrOrStderr(), "Please get a token from https://huggingface.co/settings/tokens and provide it via the --hf_token flag or the HUGGING_FACE_HUB_TOKEN environment variable.")
				} else {
					fmt.Fprintln(cmd.ErrOrStderr(), "Authorization failed. The provided Hugging Face Hub token may be invalid or lack the necessary permissions.")
					fmt.Fprintln(cmd.ErrOrStderr(), "Please check your token at https://huggingface.co/settings/tokens")
				}
			}
			return fmt.Errorf("failed to download model: status code %d", resp.StatusCode)
		}

		// Create a new file in the model directory.
		fileName := path.Base(downloadURL)
		if alias != "" {
			ext := filepath.Ext(fileName)
			if ext == "" {
				return fmt.Errorf("could not determine file extension from URL to use with alias")
			}
			fileName = alias + ext
		}
		out, err := os.Create(filepath.Join(modelDir, fileName))
		if err != nil {
			return fmt.Errorf("failed to create model file: %w", err)
		}
		defer out.Close()

		// Write the body to the file and show progress.
		progressWriter := &progressWriter{total: resp.ContentLength}
		if _, err = io.Copy(out, io.TeeReader(resp.Body, progressWriter)); err != nil {
			return fmt.Errorf("failed to save model: %w", err)
		}

		fmt.Println("\nDownload complete.")
		return nil
	},
}

func init() {
	pullCmd.Flags().String("hf_token", "", "Hugging Face API token for authentication. Can also be set with HUGGING_FACE_HUB_TOKEN env var.")
	pullCmd.Flags().String("alias", "", "Alias to save the model as. Only valid when downloading from a URL.")
}

// progressWriter is a simple writer that prints download progress.
type progressWriter struct {
	total   int64
	written int64
}

func (pw *progressWriter) Write(p []byte) (int, error) {
	n := len(p)
	pw.written += int64(n)
	pw.printProgress()
	return n, nil
}

func (pw *progressWriter) printProgress() {
	if pw.total <= 0 {
		fmt.Printf("\rDownloading... %d bytes", pw.written)
		return
	}
	percent := float64(pw.written) / float64(pw.total) * 100
	bar := strings.Repeat("=", int(percent/2))
	fmt.Printf("\r[%-50s] %.2f%%", bar, percent)
}
