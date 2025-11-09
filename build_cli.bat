@echo OFF
setlocal

echo This script builds the 'lit' CLI using 'go build' as a workaround for Bazel CGo issues on Windows.

REM Step 1: Build the C++ shared library and its import library with Bazel.
echo.
echo [Step 1/4] Building C++ engine with Bazel...
bazelisk build --config=windows --define=protobuf_allow_msvc=true //c:engine_dll
if %errorlevel% neq 0 (
    echo ERROR: Bazel build for //c:engine_dll failed.
    exit /b 1
)

REM Step 2: Create a build directory and copy over the C++ artifacts.
echo.
echo [Step 2/4] Preparing build directory...
if not exist build mkdir build
copy bazel-bin\c\engine_dll.dll build\engine.dll > NUL
if %errorlevel% neq 0 (
    echo ERROR: Failed to copy engine_dll.dll.
    echo Please check the output of 'bazelisk build --config=windows //c:engine_dll' to find the correct path and name of the generated DLL.
    echo It should be in the 'bazel-bin\c' directory.
    exit /b 1
)
copy bazel-bin\c\engine_dll.lib build\engine.lib > NUL
if %errorlevel% neq 0 (
    echo WARNING: Failed to copy engine_dll.lib. This will cause linking errors.
    exit /b 1
)
echo C++ artifacts copied to 'build' directory.

REM Step 3: Set up Go module environment.
echo.
echo [Step 3/4] Setting up Go module environment...
if not exist go.mod (
    echo Initializing Go module...
    go mod init litert-lm
    echo Fetching Go dependencies...
    go get github.com/spf13/cobra
) else (
    echo Tidying Go module...
    go mod tidy
)
REM Workaround for incorrect import path in the CLI source code.
go mod edit -replace=litert-lm/go_api/cli/cmd/cmd=./go_api/cli/cmd
if %errorlevel% neq 0 (
    echo ERROR: Failed to apply go.mod replacement.
    exit /b 1
)
echo Go environment is ready.

REM Step 4: Build the Go binary with CGo enabled.
echo.
echo [Step 4/4] Building Go CLI...
set CGO_ENABLED=1
set CGO_CXXFLAGS=-I"%CD%\bazel-litert_lm\external\org_tensorflow\tensorflow\tsl"
set CGO_LDFLAGS=-L"%CD%\build" -lengine
go build -o build\lit.exe ./go_api/cli
if %errorlevel% neq 0 (
    echo ERROR: Go build failed.
    exit /b 1
)

echo.
echo --- Build successful! ---
echo The executable is available at: build\lit.exe
echo To run it, make sure 'build\engine.dll' is in the same directory or in your system's PATH.
