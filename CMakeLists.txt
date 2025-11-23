# ==============================================================================
# BUILD INSTRUCTIONS
# ==============================================================================
#
# UBUNTU/LINUX BUILD:
# -------------------
# REQUIREMENTS:
#   - C++23 compiler: GCC 13+ or Clang 16+ (for std::expected support)
#   - Ubuntu 24.04 LTS (Noble) includes: GCC 13.3.0 and Clang 18.1.3
#   - CMake 3.26+
#
# Note: When using Clang on Linux, CMake automatically configures it to use
#       libstdc++ (GCC's standard library) for complete C++23 feature support.
#       Clang's libc++ v18 does not yet have std::expected.
#
# 1. Install vcpkg (recommended for consistent versions):
#    git clone https://github.com/Microsoft/vcpkg.git ~/vcpkg
#    cd ~/vcpkg && ./bootstrap-vcpkg.sh
#
# 2. Build with vcpkg (recommended):
#    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
#      -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake \
#      -DVCPKG_TARGET_TRIPLET=x64-linux
#    cmake --build build -j$(nproc)
#    
# 3. Alternative - Build with system packages:
#    sudo apt update
#    sudo apt install build-essential cmake ninja-build pkg-config
#    sudo apt install libspdlog-dev nlohmann-json3-dev libboost-test-dev rapidxml-dev
#    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
#    cmake --build build -j$(nproc)
#
# 4. Run tests:
#    ./build/tst_orion --log_level=test_suite
#    ./build/orion_tck_runner --verbose
#
# WINDOWS BUILD (original):
# -------------------------
# cmake -E remove_directory build
# cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static
# cmake --build build --config Release
# ctest --test-dir build --output-on-failure
#
# CROSS-PLATFORM TESTING:
# -----------------------
# Unit tests: ./build/tst_orion (Linux) or .\build\Debug\tst_orion.exe (Windows)
# TCK tests: ./build/orion_tck_runner (Linux) or .\build\Debug\orion_tck_runner.exe (Windows)
#
# DMN COMPLIANCE STATUS:
# ---------------------
# Level 2: 126/126 (100%)
# Level 3: 377/1887 (20.0%) as of October 2025
# Next target: Implement For Loop expressions for 25%+ compliance
#
# INSTALL AND CONSUME FROM ANOTHER PROJECT:
# ------------------------------------------
# cmake --install build --prefix install
# downstream CMakeLists.txt:
# find_package(orion CONFIG REQUIRED)
# target_link_libraries(your_app PRIVATE orion::orion_lib)
# ==============================================================================

cmake_minimum_required(VERSION 3.26)

# Option to use Clang instead of MSVC
option(USE_CLANG "Use Clang compiler (clang-cl) instead of MSVC" OFF)

# Set compilers before project() if using Clang
if(USE_CLANG AND NOT CMAKE_CXX_COMPILER)
  set(CMAKE_C_COMPILER "${CMAKE_SOURCE_DIR}/tools/llvm-18.1.8/bin/clang-cl.exe" CACHE STRING "C compiler" FORCE)
  set(CMAKE_CXX_COMPILER "${CMAKE_SOURCE_DIR}/tools/llvm-18.1.8/bin/clang-cl.exe" CACHE STRING "C++ compiler" FORCE)
  message(STATUS "Using Clang compiler: ${CMAKE_CXX_COMPILER}")
endif()

project(orion VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Initialize git submodules (DMN TCK test data)
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
  option(ORION_SUBMODULE_AUTO_UPDATE "Automatically update git submodules" ON)
  if(ORION_SUBMODULE_AUTO_UPDATE)
    message(STATUS "Checking DMN TCK submodule...")
    execute_process(
      COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      RESULT_VARIABLE GIT_SUBMOD_RESULT
    )
    if(NOT GIT_SUBMOD_RESULT EQUAL "0")
      message(WARNING "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, DMN TCK tests may not run correctly")
    endif()
  endif()
endif()

# Verify DMN TCK is present
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/dat/dmn-tck/TestCases")
  message(WARNING "DMN TCK submodule not found. Some tests will fail. Run: git submodule update --init --recursive")
endif()

# Platform-specific compiler settings
if(MSVC OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_SIMULATE_ID MATCHES "MSVC"))
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  add_compile_options(/utf-8)
  # Enable multi-processor compilation for faster builds
  add_compile_options(/MP)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  # Linux/GCC/Clang specific settings
  add_compile_options(-Wall -Wextra -Wpedantic -Werror)
  
  # Force Clang to use libstdc++ for full C++23 support (std::expected, etc.)
  # Clang's libc++ doesn't have complete C++23 as of version 18
  # GCC 13's libstdc++ has full std::expected support
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND UNIX AND NOT APPLE)
    add_compile_options(-stdlib=libstdc++)
    add_link_options(-stdlib=libstdc++)
  endif()
  
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0)
  elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3 -DNDEBUG)
  endif()
endif()

# Set default vcpkg triplet based on platform
if(NOT DEFINED VCPKG_TARGET_TRIPLET)
  if(WIN32)
    set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "vcpkg triplet" FORCE)
  elseif(UNIX AND NOT APPLE)
    set(VCPKG_TARGET_TRIPLET "x64-linux" CACHE STRING "vcpkg triplet" FORCE)
  elseif(APPLE)
    set(VCPKG_TARGET_TRIPLET "x64-osx" CACHE STRING "vcpkg triplet" FORCE)
  endif()
endif()

# Set output directories (cross-platform)
foreach(cfg Debug Release RelWithDebInfo MinSizeRel)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${cfg} "${CMAKE_SOURCE_DIR}/bin/${cfg}")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${cfg} "${CMAKE_SOURCE_DIR}/lib/${cfg}")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${cfg} "${CMAKE_SOURCE_DIR}/bin/${cfg}")
endforeach()

# Also set single-config generators (make, ninja)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build")

# Set up vcpkg integration (cross-platform)
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
  if(DEFINED VCPKG_TARGET_TRIPLET)
    list(PREPEND CMAKE_PREFIX_PATH "$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}")
  elseif(WIN32)
    list(PREPEND CMAKE_PREFIX_PATH "$ENV{VCPKG_ROOT}/installed/x64-windows")
  elseif(UNIX AND NOT APPLE)
    list(PREPEND CMAKE_PREFIX_PATH "$ENV{VCPKG_ROOT}/installed/x64-linux")
  endif()
endif()

# If no VCPKG_ROOT env var, try common locations
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND NOT DEFINED ENV{VCPKG_ROOT})
  if(EXISTS "$ENV{HOME}/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "$ENV{HOME}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "vcpkg toolchain file")
  elseif(EXISTS "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "vcpkg toolchain file")
  endif()
endif()

# Optional dependencies for apps
find_package(spdlog CONFIG QUIET)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(benchmark CONFIG QUIET)

find_path(RAPIDXML_INCLUDE_DIR rapidxml/rapidxml.hpp
  PATHS
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include"
    "$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include"
    "${CMAKE_SOURCE_DIR}/third_party/rapidxml/include"
    "${CMAKE_SOURCE_DIR}/third_party/rapidxml"
)
if(NOT RAPIDXML_INCLUDE_DIR)
  message(FATAL_ERROR "rapidxml/rapidxml.hpp not found. Install 'rapidxml' via vcpkg or add third_party/rapidxml/")
endif()

# Sources
set(ORION_SRC
  src/api/engine.cpp
  src/api/logger.cpp
  src/bre/ast_node.cpp
  src/bre/bkm_manager.cpp
  src/bre/business_knowledge_model.cpp
  src/bre/dmn_model.cpp
  src/bre/dmn_parser.cpp
  src/bre/feel/evaluator.cpp
  src/bre/feel/expr.cpp
  src/bre/feel/function_registry.cpp
  src/bre/feel/functions.cpp
  src/bre/feel/lexer.cpp
  src/bre/feel/parameter_binder.cpp
  src/bre/feel/parser.cpp
  src/bre/feel/types.cpp
  src/bre/feel/unary.cpp
  src/bre/feel/util.cpp
  src/common/util.cpp
  src/common/xml2json.cpp
)

# Header lists (public + internal) so VS shows directory tree mirroring repo
file(GLOB ORION_PUBLIC_HDRS CONFIGURE_DEPENDS 
  "${CMAKE_SOURCE_DIR}/include/orion/api/*.hpp"
  "${CMAKE_SOURCE_DIR}/include/orion/bre/*.hpp"
  "${CMAKE_SOURCE_DIR}/include/orion/bre/feel/*.hpp"
  "${CMAKE_SOURCE_DIR}/include/orion/common/*.hpp"
)
file(GLOB ORION_INTERNAL_HDRS CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/src/common/*.hpp"
  "${CMAKE_SOURCE_DIR}/src/bre/*.hpp"
  "${CMAKE_SOURCE_DIR}/src/bre/feel/*.hpp"
)

# Create the main library
add_library(orion_lib STATIC ${ORION_SRC})
add_library(orion::orion_lib ALIAS orion_lib)

# Attach headers (no compile impact) for IDE view
target_sources(orion_lib PRIVATE ${ORION_PUBLIC_HDRS} ${ORION_INTERNAL_HDRS})
# Group files to mirror on-disk layout
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${ORION_SRC} ${ORION_PUBLIC_HDRS} ${ORION_INTERNAL_HDRS})

# Public interface only exposes install-safe paths; build uses project include dir.
target_include_directories(orion_lib
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bre
    ${RAPIDXML_INCLUDE_DIR}
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(orion_lib PUBLIC
  nlohmann_json::nlohmann_json
)

set_property(TARGET orion_lib PROPERTY VERSION ${PROJECT_VERSION})
set_property(TARGET orion_lib PROPERTY PUBLIC_HEADER "")

option(ORION_BUILD_TESTS "Build Orion BRE unit tests (and benchmarks)" ON)
option(ORION_BUILD_APPS "Build Orion BRE applications (orion_app, orion_tck_runner)" ON)

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

if(ORION_BUILD_TESTS)
  find_package(Boost 1.70 CONFIG REQUIRED COMPONENTS unit_test_framework)
  enable_testing()
  
  # Add test executable - only include files that exist
  add_executable(tst_orion
      tst/bre/test_main.cpp
      tst/bre/test_hit_policy_debug.cpp
      # TCK-specific tests
      tst/bre/tck/test_tck_runner.cpp
      tst/bre/tck/test_tck_0112_comparison.cpp
      tst/bre/tck/test_tck_0112_collection.cpp
      tst/bre/tck/test_tck_0112_rule_order.cpp
      tst/bre/tck/test_tck_0118_priority.cpp
      # FEEL unit tests
      tst/bre/feel/test_lexer.cpp
      tst/bre/feel/test_lexer_decimal_numbers.cpp
      tst/bre/feel/test_parser.cpp
      tst/bre/feel/test_parser_subtraction.cpp
      tst/bre/feel/test_evaluator_exponentiation.cpp
      tst/bre/feel/test_evaluator_function_calls.cpp
      tst/bre/feel/test_evaluator_list_operations.cpp
      tst/bre/feel/test_evaluator_logical_operators.cpp
      tst/bre/feel/test_evaluator_math_debug.cpp
      tst/bre/feel/test_evaluator_math_direct.cpp
      tst/bre/feel/test_evaluator_math_edge_cases.cpp
      tst/bre/feel/test_evaluator_negative_numbers.cpp
      tst/bre/feel/test_evaluator_null_arithmetic.cpp
      tst/bre/feel/test_evaluator_property_access.cpp
      tst/bre/feel/test_evaluator_builtin_math.cpp
      tst/bre/feel/test_feel_segfault_debug.cpp
      tst/bre/feel/test_function_registry.cpp
      tst/bre/feel/test_named_parameters.cpp
      tst/bre/feel/test_conditional_expressions.cpp
      tst/bre/feel/test_debug_not_function.cpp
      tst/bre/feel/test_tck_named_params_quick.cpp
      tst/bre/feel/test_dmn_abs_named_params.cpp
      tst/bre/feel/test_tck_abs_debug.cpp
      tst/common/test_xml2json.cpp
  )

  # Add Boost.Test logger implementation for tests
  target_sources(tst_orion PRIVATE src/api/boost_test_logger.cpp)
  target_link_libraries(tst_orion PRIVATE orion_lib Boost::unit_test_framework fmt::fmt)
  target_include_directories(tst_orion PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  set_property(TARGET tst_orion PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
  add_test(NAME orion_tests COMMAND tst_orion WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
  
  # Ensure test sources appear under tst/bre (only include existing files)
  source_group(TREE ${CMAKE_SOURCE_DIR} FILES
    tst/bre/test_main.cpp
    tst/bre/test_hit_policy_debug.cpp
    # TCK-specific tests
    tst/bre/tck/test_tck_runner.cpp
    tst/bre/tck/test_tck_0112_comparison.cpp
    tst/bre/tck/test_tck_0112_collection.cpp
    tst/bre/tck/test_tck_0112_rule_order.cpp
    tst/bre/tck/test_tck_0118_priority.cpp
    # FEEL unit tests
    tst/bre/feel/test_lexer.cpp
    tst/bre/feel/test_lexer_decimal_numbers.cpp
    tst/bre/feel/test_parser.cpp
    tst/bre/feel/test_parser_subtraction.cpp
    tst/bre/feel/test_evaluator_exponentiation.cpp
    tst/bre/feel/test_evaluator_function_calls.cpp
    tst/bre/feel/test_evaluator_list_operations.cpp
    tst/bre/feel/test_evaluator_logical_operators.cpp
    tst/bre/feel/test_evaluator_math_debug.cpp
    tst/bre/feel/test_evaluator_math_direct.cpp
    tst/bre/feel/test_evaluator_math_edge_cases.cpp
    tst/bre/feel/test_evaluator_negative_numbers.cpp
    tst/bre/feel/test_evaluator_null_arithmetic.cpp
    tst/bre/feel/test_evaluator_property_access.cpp
    tst/common/test_xml2json.cpp
  )

  if(TARGET benchmark::benchmark)
    add_executable(orion-bench src/bench/orion_bench.cpp src/api/spdlog_logger.cpp src/common/log.cpp)
    target_link_libraries(orion-bench PRIVATE orion_lib benchmark::benchmark spdlog::spdlog)
    set_property(TARGET orion-bench PROPERTY VS_GLOBAL_IntDir "$(Configuration)\\$(ProjectName)\\")
    source_group(TREE ${CMAKE_SOURCE_DIR} FILES src/bench/orion_bench.cpp)
  endif()
endif()

if(ORION_BUILD_APPS)
  # Only build apps if spdlog is available
  if(TARGET spdlog::spdlog)
    add_executable(orion_app src/apps/orion_bre_main.cpp src/api/spdlog_logger.cpp src/common/log.cpp)
    target_link_libraries(orion_app PRIVATE orion_lib spdlog::spdlog)
    # Group app source under src/apps
    source_group(TREE ${CMAKE_SOURCE_DIR} FILES src/apps/orion_bre_main.cpp)

    # TCK runner app
    add_executable(orion_tck_runner src/apps/orion_tck_runner.cpp src/api/spdlog_logger.cpp src/common/log.cpp)
    target_link_libraries(orion_tck_runner PRIVATE orion_lib spdlog::spdlog)
    source_group(TREE ${CMAKE_SOURCE_DIR} FILES src/apps/orion_tck_runner.cpp)
  else()
    message(WARNING "Skipping apps build: spdlog not found. Install with -DORION_BUILD_APPS=OFF or install spdlog.")
  endif()
endif()

foreach(tgt orion_lib orion_app orion_tck_runner tst_orion)
  if(TARGET ${tgt})
    set_property(TARGET ${tgt} PROPERTY VS_GLOBAL_IntDir "$(Configuration)\\$(ProjectName)\\")
  endif()
endforeach()

# Handle benchmark target separately since it's conditional
if(TARGET orion-bench)
  set_property(TARGET orion-bench PROPERTY VS_GLOBAL_IntDir "$(Configuration)\\$(ProjectName)\\")
endif()

include(GNUInstallDirs)

install(TARGETS orion_lib
  EXPORT orionTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT orionTargets
  NAMESPACE orion::
  FILE orionTargets.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/orion
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/orionConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/orionConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/orion
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/orionConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/orionConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/orionConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/orion
)
