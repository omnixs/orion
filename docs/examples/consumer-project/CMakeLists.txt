cmake_minimum_required(VERSION 3.26)
project(orion-consumer-example VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find dependencies (required by orion)
find_package(nlohmann_json CONFIG REQUIRED)

# Find the orion package
find_package(orion CONFIG REQUIRED)

# Create executable
add_executable(consumer-app main.cpp)

# Link against orion library
target_link_libraries(consumer-app PRIVATE orion::orion_lib)

# Optional: If you also want spdlog logging
find_package(spdlog CONFIG QUIET)
if(TARGET spdlog::spdlog)
    target_link_libraries(consumer-app PRIVATE spdlog::spdlog)
    target_compile_definitions(consumer-app PRIVATE HAS_SPDLOG)
endif()
