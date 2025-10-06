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

	"github.com/spf13/cobra"
)

var modelDir = filepath.Join(os.Getenv("HOME"), ".litert-lm", "models")

var rootCmd = &cobra.Command{
	Use:           "lit",
	Short:         "lit is a CLI for LiteRT-LM",
	Long:          `A command-line interface to interact with LiteRT-LM models.`,
	SilenceErrors: true, // otherwise errors get printed twice.
}

var completionCmd = &cobra.Command{
	Use:   "completion [bash|zsh|fish|powershell]",
	Short: "Generate completion script",
	Long: `To load completions:

Bash:

  $ source <(lit completion bash)

  # To load completions for each session, execute once:
  # Linux:
  $ lit completion bash > /etc/bash_completion.d/lit
  # macOS:
  $ lit completion bash > /usr/local/etc/bash_completion.d/lit

Zsh:

  # If shell completion is not already enabled in your environment,
  # you will need to enable it.  You can execute the following once:

  $ echo "autoload -U compinit; compinit" >> ~/.zshrc

  # To load completions for each session, execute once:
  $ lit completion zsh > "${fpath[1]}/_lit"

  # You will need to start a new shell for this setup to take effect.

Fish:

  $ lit completion fish | source

  # To load completions for each session, execute once:
  $ lit completion fish > ~/.config/fish/completions/lit.fish

Powershell:

  PS> lit completion powershell | Out-String | Invoke-Expression

  # To load completions for every new session, run:
  PS> lit completion powershell > lit.ps1
  # and source this file from your PowerShell profile.
`,
	DisableFlagsInUseLine: true,
	ValidArgs:             []string{"bash", "zsh", "fish", "powershell"},
	Args:                  cobra.MatchAll(cobra.ExactArgs(1), cobra.OnlyValidArgs),
	Run: func(cmd *cobra.Command, args []string) {
		switch args[0] {
		case "bash":
			cmd.Root().GenBashCompletion(os.Stdout)
		case "zsh":
			cmd.Root().GenZshCompletion(os.Stdout)
		case "fish":
			cmd.Root().GenFishCompletion(os.Stdout, true)
		case "powershell":
			cmd.Root().GenPowerShellCompletionWithDesc(os.Stdout)
		}
	},
}

// Execute adds all child commands to the root command and sets flags appropriately.
// This is called by main.main(). It only needs to happen once to the rootCmd.
func Execute() {
	if err := rootCmd.Execute(); err != nil {
		fmt.Fprintf(os.Stderr, "Error: %v\n", err)
		os.Exit(1)
	}
}

func init() {
	rootCmd.AddCommand(completionCmd)
	rootCmd.AddCommand(listCmd)
	rootCmd.AddCommand(pullCmd)
	rootCmd.AddCommand(rmCmd)
	rootCmd.AddCommand(runCmd)

	runCmd.Flags().BoolP("verbose", "v", false, "Enable verbose logging")
	runCmd.Flags().String("backend", "cpu", "The backend to use for inference (e.g., 'cpu', 'gpu')")
	runCmd.Flags().Bool("benchmark", false, "Run in benchmark mode, printing performance metrics. This silences all logs to avoid affecting performance metrics. Use the -v or --verbose flag to override this behavior for debugging purposes.")
	rootCmd.PersistentFlags().Int("min_log_level", 3, "Set the minimum log level (0=VERBOSE, 1=INFO, 2=WARNING, 3=ERROR, 4=SILENT)")
}
