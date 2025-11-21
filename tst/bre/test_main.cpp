#define BOOST_TEST_MODULE orion_bre
#include <boost/test/unit_test.hpp>
#include <orion/api/logger.hpp>
#include <orion/api/boost_test_logger.hpp>
#include <memory>

// Initialize the logger for tests
struct GlobalTestFixture {
    GlobalTestFixture() {
        // Set up Boost.Test logger for all tests
        orion::api::Logger::instance().set_logger(
            std::make_shared<orion::api::BoostTestLogger>()
        );
    }
    ~GlobalTestFixture() = default;
};

BOOST_GLOBAL_FIXTURE(GlobalTestFixture);

// This file serves as the test runner main entry point
// The actual tests are defined in test_tck_levels.cpp