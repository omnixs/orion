# For local development/testing, use local path
# For production, use: vcpkg_from_git with GitHub URL
set(SOURCE_PATH "${CMAKE_CURRENT_LIST_DIR}/../..")

# Production example (commented out for local testing):
# vcpkg_from_git(
#     OUT_SOURCE_PATH SOURCE_PATH
#     URL https://github.com/your-org/orion.git
#     REF v1.0.0
# )

# Only build the library, not tests/apps/benchmarks
vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DORION_BUILD_TESTS=OFF
        -DCMAKE_DISABLE_FIND_PACKAGE_Boost=ON
        -DCMAKE_DISABLE_FIND_PACKAGE_benchmark=ON
)

vcpkg_cmake_build()

vcpkg_cmake_install()

# Fix CMake config paths
vcpkg_cmake_config_fixup(
    PACKAGE_NAME orion
    CONFIG_PATH lib/cmake/orion
)

# Remove duplicate headers from debug
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

# Handle copyright/license
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)

# Copy usage file
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
