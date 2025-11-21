# ORION Business Rule Engine (BRE)

This module provides a self-contained **DMN Level 1** Business Rule Engine.

## Public headers
Headers live under `include/orion/bre/` and are included as:
```cpp
#include <orion/api/engine.hpp>
#include <orion/bre/dmn_parser.hpp>
#include <orion/bre/hit_policy.hpp>
```

## Build & Test
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

## Install & Consume
```bash
cmake --install build --prefix install
```
In a downstream project:
```cmake
find_package(orion CONFIG REQUIRED)
target_link_libraries(app PRIVATE orion::orion_bre)
```
