# Build Instructions

## Prerequisites

### Required

- CMake 3.20+
- **C++23 Compiler**:
  - **Windows**: Visual Studio 2022 (MSVC 19.30+) with C++23 support
  - **Linux**: **GCC 13+** (Ubuntu 24.04 LTS includes GCC 13.3.0)
    - ⚠️ **Important**: Clang 18 on Linux cannot access libstdc++'s C++23 features (`std::expected`). Use GCC instead.
- vcpkg (dependency manager)

### Recommended

- Ninja build system (`sudo apt install ninja-build` or `choco install ninja`)
- ccache for faster rebuilds (`sudo apt install ccache` or `choco install ccache`)

### C++23 Feature Requirements

The project requires **full C++23 support**, specifically:
- `std::expected<T, E>` - Error handling without exceptions
- `std::string_view` - Zero-copy string operations
- Concepts and ranges

**Tested Configurations:**
- Ubuntu 24.04 LTS with GCC 13.3.0 ✅
- Windows with MSVC 2022 (TBD)
- Clang 18 with libstdc++ on Linux ❌ (does not work - use GCC)

## Performance Optimizations

### Enable ccache (Linux/macOS)
```bash
# Install ccache
sudo apt install ccache  # Debian/Ubuntu
brew install ccache      # macOS

# Configure CMake to use ccache
export CMAKE_CXX_COMPILER_LAUNCHER=ccache
export CMAKE_C_COMPILER_LAUNCHER=ccache

# Verify ccache is working
ccache -s  # Show cache statistics
```

### Enable Ninja (Faster than Make)
```bash
# Install Ninja
sudo apt install ninja-build  # Linux
choco install ninja          # Windows
brew install ninja           # macOS

# Use -G Ninja flag in CMake configuration (see below)
```

## Windows (PowerShell)

### Initial Configuration
```powershell
# Remove old build
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue

# Configure
cmake -S . -B build `
  -G "Visual Studio 17 2022" `
  -A x64 `
  -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake `
  -DVCPKG_TARGET_TRIPLET=x64-windows-static
```

### Build
```powershell
# Build Debug
cmake --build build --config Debug

# Build Release
cmake --build build --config Release
```

## Linux (Bash)

### Dual Build Setup (Debug + Release)

**⚠️ Important: Use GCC (not Clang) for full C++23 support**

```bash
# Remove old builds
rm -rf build-debug build-release

# Configure Debug build with GCC (required for std::expected)
CC=gcc CXX=g++ cmake -S . -B build-debug \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=/home/$USER/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
  -DCMAKE_C_COMPILER_LAUNCHER=ccache

# Configure Release build with GCC
CC=gcc CXX=g++ cmake -S . -B build-release \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=/home/$USER/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
  -DCMAKE_C_COMPILER_LAUNCHER=ccache

# Build both (parallel, uses all CPU cores)
cmake --build build-debug -j$(nproc)
cmake --build build-release -j$(nproc)
```

### Incremental Builds (Fast)
```bash
# Rebuild only changed files (Debug)
cmake --build build-debug -j$(nproc)

# Rebuild only changed files (Release)
cmake --build build-release -j$(nproc)
```

### Performance Comparison
| Build System | Clean Build Time | Incremental Build Time | Cache Hit Rate |
|--------------|------------------|------------------------|----------------|
| Make (no ccache) | ~120s | ~30s | N/A |
| Make + ccache | ~120s (first), ~15s (cached) | ~8s | 85-95% |
| Ninja (no ccache) | ~45s | ~12s | N/A |
| **Ninja + ccache** | **~45s (first), ~5s (cached)** | **~2-3s** | **90-98%** |

## Common Build Options

- `CMAKE_BUILD_TYPE`: Debug, Release, RelWithDebInfo
- `ORION_BUILD_TESTS`: ON (default) or OFF
- `CMAKE_EXPORT_COMPILE_COMMANDS`: ON (for clang-tidy)

## Expected Output

Successful build produces:
- `build/orion_app` - CLI application
- `build/tst_orion` - Unit test suite
- `build/orion_tck_runner` - TCK compliance runner
- `build/orion-bench` - Performance benchmarks

## Troubleshooting

**vcpkg not found:**
- Update `CMAKE_TOOLCHAIN_FILE` path
- Ensure vcpkg installed and bootstrapped

**Missing dependencies:**
```powershell
# Windows
vcpkg install spdlog nlohmann-json boost-test --triplet x64-windows-static

# Linux
vcpkg install spdlog nlohmann-json boost-test
```

**Build errors:**
- Check CMake version: `cmake --version`
- Verify compiler: Visual Studio 2022+ or GCC 11+
- Clear CMake cache: `Remove-Item build/CMakeCache.txt`
