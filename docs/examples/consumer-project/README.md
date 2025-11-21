# Orion Consumer Example

This example demonstrates how to use the Orion Business Rules Engine as a library in## Running

**Linux/macOS:**
```bash
./build/consumer-example
```

**Windows:**
```cmd
build\Release\consumer-example.exe
``` own project.

## Important Note on Binary Caching

This example is configured to **disable vcpkg binary caching** (`-binarycaching` feature flag). This prevents the orion package from being stored in your system's global vcpkg cache, which is desirable for an example/development scenario because:

- ✅ Changes to orion source code are immediately reflected when rebuilding the example
- ✅ No global system state is modified (cache remains clean)
- ✅ Each build uses the latest local orion sources
- ⚠️ Builds will be slower as vcpkg rebuilds orion each time

For production use, you would typically **enable** binary caching to speed up builds.

## Prerequisites

- CMake 3.26 or later
- C++23 compatible compiler:
  - GCC 11+ (Linux)
  - Clang 15+ (Linux/macOS)
  - Visual Studio 2022 17.0+ (Windows)
- vcpkg package manager

## Setup

1. Make sure you have vcpkg installed and the `VCPKG_ROOT` environment variable set:
   
   **Linux/macOS:**
   ```bash
   export VCPKG_ROOT=/path/to/vcpkg
   ```
   
   **Windows (Command Prompt):**
   ```cmd
   set VCPKG_ROOT=C:\path\to\vcpkg
   ```
   
   **Windows (PowerShell):**
   ```powershell
   $env:VCPKG_ROOT = "C:\path\to\vcpkg"
   ```

2. The build will automatically use the vcpkg overlay to find the orion port.

## Building

### Linux/macOS

**Option 1: Using the build script**
```bash
cd examples/consumer-project
./build.sh
```

**Option 2: Manual build**
```bash
export VCPKG_ROOT=/path/to/vcpkg

cmake -B build -S . \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_OVERLAY_PORTS=../../ports \
  -DVCPKG_FEATURE_FLAGS=-binarycaching \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build
```

### Windows

**Option 1: Using PowerShell script**
```powershell
cd examples\consumer-project
.\build.ps1
```

**Option 2: Using batch file**
```cmd
cd examples\consumer-project
build.bat
```

**Option 3: Using Visual Studio 2022**

*Using CMake GUI:*
1. Open CMake GUI
2. Set source directory to `examples/consumer-project`
3. Set build directory to `examples/consumer-project/build`
4. Add Cache Entries:
   - `CMAKE_TOOLCHAIN_FILE` (FILEPATH): `%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake`
   - `VCPKG_OVERLAY_PORTS` (STRING): `../../ports`
   - `VCPKG_FEATURE_FLAGS` (STRING): `-binarycaching`
5. Click "Configure", select "Visual Studio 17 2022" generator
6. Click "Generate"
7. Click "Open Project" or open `build/consumer-example.sln` in Visual Studio
8. Build the solution (F7 or Build → Build Solution)

*Using Developer Command Prompt or PowerShell:*
```cmd
cd examples\consumer-project

cmake -B build -S . ^
  -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake ^
  -DVCPKG_OVERLAY_PORTS=..\..\ports ^
  -DVCPKG_FEATURE_FLAGS=-binarycaching ^
  -DCMAKE_BUILD_TYPE=Release

cmake --build build --config Release
```

## Running

```bash
./build/consumer-app
```

## What It Demonstrates

This example shows:

1. **Custom Logger**: Implementing the `ILogger` interface for custom logging
2. **Engine Creation**: Creating a `BusinessRulesEngine` instance
3. **DMN Evaluation**: Loading DMN XML and evaluating decisions
4. **Context Passing**: Passing JSON context to decisions
5. **Result Handling**: Processing decision results

## Project Structure

- `CMakeLists.txt` - CMake configuration that finds and links orion
- `vcpkg.json` - vcpkg manifest declaring orion dependency
- `main.cpp` - Example application code
- `build.sh` - Build script with vcpkg overlay configuration
- `README.md` - This file

## Key Concepts

### Finding Orion Package

```cmake
find_package(orion CONFIG REQUIRED)
target_link_libraries(consumer-app PRIVATE orion::orion_lib)
```

### Using Custom Logger

```cpp
class ConsoleLogger : public orion::api::ILogger {
    // Implement trace, debug, info, warn, error, critical
};

orion::api::set_logger(std::make_shared<ConsoleLogger>());
```

### Creating Engine

```cpp
auto engine = orion::api::BusinessRulesEngine::create();
```

### Evaluating Decisions

```cpp
nlohmann::json context = {{"age", 25}};
auto result = engine->evaluate_decision(dmn_xml, "decision_001", context);
```

## Adapting for Your Project

To use orion in your own project:

1. Copy `vcpkg.json` to your project root
2. Add the `find_package` and `target_link_libraries` lines to your CMakeLists.txt
3. Set `VCPKG_OVERLAY_PORTS` when configuring with CMake
4. Include `<orion/api/engine.hpp>` in your source files

## Troubleshooting

**Q: Cannot find orion package**  
A: Make sure you're using `-DVCPKG_OVERLAY_PORTS=/path/to/orion/ports` in your cmake command

**Q: Linking errors with nlohmann-json**  
A: The dependency should be transitive. Make sure you're linking with `orion::orion_lib` (with namespace)

**Q: C++23 errors**  
A: Ensure your compiler supports C++23. Use GCC 11+, Clang 15+, or MSVC 2022+
