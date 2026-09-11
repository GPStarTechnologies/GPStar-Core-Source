// This file forces the linker to include the library implementation
#include "../src/BLEQueueManager.cpp"

// Include the Google Test framework
#include <gtest/gtest.h>

// Include all test suites
#include "test_ble_queue_manager.h"

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
