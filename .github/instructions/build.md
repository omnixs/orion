# Build Instructions

## Prerequisites

- CMake 3.20+
- Visual Studio 2022 (Windows) or GCC 11+ (Linux)
- vcpkg (dependency manager)

## Windows (PowerShell)

### Clean Build
```powershell
# Remove old build
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue

# Configure
cmake -S . -B build `
  -G "Visual Studio 17 2022" `
  -A x64 `
  -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake `
  -DVCPKG_TARGET_TRIPLET=x64-windows-static

# Build (Debug)
cmake --build build --config Debug

# Build (Release)
cmake --build build --config Release
```

### Incremental Build
```powershell
cmake --build build --config Debug
```

## Linux (Bash)

### Clean Build
```bash
# Remove old build
rm -rf build

# Configure
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=/home/$USER/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build
cmake --build build
```

### Incremental Build
```bash
cmake --build build
```

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
