# ORION — Optimized Rule Integration & Operations Native

[![Version](https://img.shields.io/github/v/release/omnixs/orion)](https://github.com/omnixs/orion/releases/latest)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![CI](https://github.com/omnixs/orion/actions/workflows/ci-full.yml/badge.svg)](https://github.com/omnixs/orion/actions/workflows/ci-full.yml)

A native C++ implementation of the DMN™ (Decision Model and Notation) specification,
focusing on **decision tables** with an emphasis on performance, correctness, and simple integration.

- **Language:** C++23
- **License:** Apache-2.0 (see `LICENSE`)
- **Status:** Early stage (APIs may evolve)
- **DMN Compliance:** Level 1 and Level 2 compatible
- **Goals:** fast rule evaluation, clean table model, clear extensibility points

## Download

Download the latest release from [GitHub Releases](https://github.com/omnixs/orion/releases/latest):

- **Linux**: `orion-X.Y.Z-linux-x64.tar.gz`
- **Windows**: `orion-X.Y.Z-windows-x64.zip`

Each release includes:
- Static libraries (`liborion_lib.a` / `orion_lib.lib`)
- Headers (`include/orion/`)
- CMake configuration files
- Command-line tools (`orion_app`, `orion_tck_runner`)
- TCK test baseline for the version
- License files

**Verify downloads** with the corresponding `.sha256` checksum files.

## Features

- Decision Table evaluation compatible with DMN L1/L2 (hit policies, FEEL expressions)
- Business Knowledge Models (BKM) support
- Portable C++ API under `orion::api` namespace
- FEEL (Friendly Enough Expression Language) expression evaluator
- Pluggable logger interface (optional spdlog integration)
- Deterministic tests and benchmarks
- DMN TCK (Technology Compatibility Kit) test runner

## Quick Start

### Prerequisites

- C++23 compatible compiler (GCC 11+, Clang 15+, MSVC 2022+)
- CMake 3.26 or later
- vcpkg package manager

### Setup vcpkg

```bash
git clone https://github.com/Microsoft/vcpkg.git ~/vcpkg
cd ~/vcpkg
./bootstrap-vcpkg.sh  # or bootstrap-vcpkg.bat on Windows
```

### Build Orion

```bash
git clone https://github.com/omnixs/orion.git orion
cd orion

# Initialize DMN TCK test submodule (required for running tests)
git submodule update --init --recursive

# Configure with vcpkg
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_TARGET_TRIPLET=x64-linux

# Build
cmake --build build -j

# Run tests
./build/tst_orion --log_level=test_suite
```

**Note**: The DMN TCK (Test Compatibility Kit) is included as a git submodule in `dat/dmn-tck/`. 
If you skip the submodule initialization, the build will succeed but some tests will fail due to missing test data files.

### Minimal Usage Example

```cpp
#include <orion/api/engine.hpp>
#include <nlohmann/json.hpp>
#include <iostream>

int main() {
    // Create the business rules engine
    orion::api::BusinessRulesEngine engine;
    
    // Load DMN model from XML string or file
    std::string dmn_xml = "<?xml version=\"1.0\"?>...";
    auto result = engine.load_dmn_model(dmn_xml);
    if (!result) {
        std::cerr << "Error: " << result.error() << std::endl;
        return 1;
    }
    
    // Create input context as JSON
    nlohmann::json context = {
        {"age", 25},
        {"category", "premium"}
    };
    
    // Evaluate decision
    std::string result = engine.evaluate(context.dump());
    std::cout << "Result: " << result << std::endl;
    
    return 0;
}
```

See `docs/examples/consumer-project/` for a complete working example with build configuration.

## Contributing

We welcome issues and PRs! Please read:

- `CONTRIBUTING.md` (DCO, coding guidelines, headers)
- `CODE_OF_CONDUCT.md` (Contributor Covenant)

## Security

See `SECURITY.md` for reporting vulnerabilities.

## License

Copyright (c) 2025 ORION contributors.

Licensed under the Apache License, Version 2.0. See `LICENSE` for details.

## Trademarks

DMN™ is a trademark of the Object Management Group (OMG). Any other names are the
property of their respective owners and used for identification only.
