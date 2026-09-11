/**
 *   BLE Queue Manager Unit Tests
 *   GPStar Shared Library - Bluetooth
 *
 *   Tests for BLEQueueManager functions:
 *   - Initialization
 *   - Enqueue/dequeue operations
 *   - Overflow detection
 *   - Queue status checks
 *   - Message preservation
 */

#include <gtest/gtest.h>
#include "BLEQueueManager.h"
#include "BLEMessage.h"
#include "BLEConstants.h"
#include "../../Communication/include/Communication.h"
#include <cstring>

// Test fixture for queue tests
class BLEQueueManagerTest : public ::testing::Test {
protected:
  BLEMessageQueue queue;
  BLEMessage testMsg;
  BLEMessage readMsg;

  void SetUp() override {
    // Initialize queue before each test
    BLEQueueManager_Init(&queue);
    
    // Create a test message
    memset(&testMsg, 0, sizeof(BLEMessage));
    testMsg.packetType = PACKET_COMMAND;
    testMsg.sequence = 42;
    testMsg.length = 6;
    testMsg.status = BLE_MSG_STATUS_QUEUED;
    testMsg.payload[0] = 0x02;  // Frame start
    testMsg.payload[1] = 0x01;  // Command ID low
    testMsg.payload[2] = 0x00;  // Command ID high
    testMsg.payload[3] = 0x00;  // Data low
    testMsg.payload[4] = 0x00;  // Data high
    testMsg.payload[5] = 0x04;  // Frame end
    
    memset(&readMsg, 0, sizeof(BLEMessage));
  }
};

// Test 1: Queue initialization
TEST_F(BLEQueueManagerTest, InitializeQueue) {
  EXPECT_EQ(BLEQueueManager_GetCount(&queue), 0);
  EXPECT_TRUE(BLEQueueManager_IsEmpty(&queue));
  EXPECT_FALSE(BLEQueueManager_IsFull(&queue));
  EXPECT_EQ(queue.head, 0);
  EXPECT_EQ(queue.tail, 0);
  EXPECT_EQ(queue.overflowCount, 0);
}

// Test 2: Enqueue single message
TEST_F(BLEQueueManagerTest, EnqueueSingleMessage) {
  uint8_t result = BLEQueueManager_Enqueue(&queue, &testMsg);
  
  EXPECT_EQ(result, BLE_QUEUE_OK);
  EXPECT_EQ(BLEQueueManager_GetCount(&queue), 1);
  EXPECT_FALSE(BLEQueueManager_IsEmpty(&queue));
  EXPECT_FALSE(BLEQueueManager_IsFull(&queue));
}

// Test 3: Dequeue single message
TEST_F(BLEQueueManagerTest, DequeueMessageFIFO) {
  BLEQueueManager_Enqueue(&queue, &testMsg);
  
  uint8_t result = BLEQueueManager_Dequeue(&queue, &readMsg);
  
  EXPECT_EQ(result, BLE_QUEUE_OK);
  EXPECT_EQ(readMsg.packetType, testMsg.packetType);
  EXPECT_EQ(readMsg.sequence, testMsg.sequence);
  EXPECT_EQ(readMsg.length, testMsg.length);
  EXPECT_EQ(BLEQueueManager_GetCount(&queue), 0);
  EXPECT_TRUE(BLEQueueManager_IsEmpty(&queue));
}

// Test 4: Dequeue empty queue
TEST_F(BLEQueueManagerTest, DequeueFromEmpty) {
  uint8_t result = BLEQueueManager_Dequeue(&queue, &readMsg);
  
  EXPECT_EQ(result, BLE_QUEUE_EMPTY);
  EXPECT_EQ(BLEQueueManager_GetCount(&queue), 0);
}

// Test 5: Enqueue and dequeue 16 messages (full capacity)
TEST_F(BLEQueueManagerTest, EnqueueDequeue16Messages) {
  // Enqueue 16 messages
  for(int i = 0; i < 16; i++) {
    testMsg.sequence = i;
    uint8_t result = BLEQueueManager_Enqueue(&queue, &testMsg);
    EXPECT_EQ(result, BLE_QUEUE_OK) << "Failed at message " << i;
  }
  
  EXPECT_EQ(BLEQueueManager_GetCount(&queue), 16);
  EXPECT_TRUE(BLEQueueManager_IsFull(&queue));
  
  // Dequeue 16 messages in FIFO order
  for(int i = 0; i < 16; i++) {
    uint8_t result = BLEQueueManager_Dequeue(&queue, &readMsg);
    EXPECT_EQ(result, BLE_QUEUE_OK) << "Dequeue failed at message " << i;
    EXPECT_EQ(readMsg.sequence, i) << "Message order incorrect at " << i;
  }
  
  EXPECT_EQ(BLEQueueManager_GetCount(&queue), 0);
  EXPECT_TRUE(BLEQueueManager_IsEmpty(&queue));
}

// Test 6: Queue overflow detection
TEST_F(BLEQueueManagerTest, OverflowDetection) {
  // Fill queue with 16 messages
  for(int i = 0; i < 16; i++) {
    testMsg.sequence = i;
    BLEQueueManager_Enqueue(&queue, &testMsg);
  }
  
  EXPECT_EQ(queue.overflowCount, 0);
  
  // Try to enqueue 17th message
  testMsg.sequence = 99;
  uint8_t result = BLEQueueManager_Enqueue(&queue, &testMsg);
  
  EXPECT_EQ(result, BLE_QUEUE_FULL);
  EXPECT_EQ(queue.overflowCount, 1);
  EXPECT_EQ(BLEQueueManager_GetCount(&queue), 16); // Count unchanged
  
  // Verify the full message wasn't overwritten
  BLEQueueManager_Dequeue(&queue, &readMsg);
  EXPECT_EQ(readMsg.sequence, 0); // First message still there
}

// Test 7: Multiple overflow attempts
TEST_F(BLEQueueManagerTest, MultipleOverflows) {
  // Fill queue
  for(int i = 0; i < 16; i++) {
    testMsg.sequence = i;
    BLEQueueManager_Enqueue(&queue, &testMsg);
  }
  
  // Try to enqueue 3 more times
  for(int i = 0; i < 3; i++) {
    uint8_t result = BLEQueueManager_Enqueue(&queue, &testMsg);
    EXPECT_EQ(result, BLE_QUEUE_FULL);
  }
  
  EXPECT_EQ(queue.overflowCount, 3);
  EXPECT_EQ(BLEQueueManager_GetCount(&queue), 16);
}

// Test 8: Peek non-destructive read
TEST_F(BLEQueueManagerTest, PeekNonDestructive) {
  BLEQueueManager_Enqueue(&queue, &testMsg);
  
  BLEMessage peekMsg;
  uint8_t result = BLEQueueManager_Peek(&queue, &peekMsg);
  
  EXPECT_EQ(result, BLE_QUEUE_OK);
  EXPECT_EQ(peekMsg.sequence, testMsg.sequence);
  EXPECT_EQ(BLEQueueManager_GetCount(&queue), 1); // Count unchanged
  
  // Dequeue should return same message
  BLEQueueManager_Dequeue(&queue, &readMsg);
  EXPECT_EQ(readMsg.sequence, peekMsg.sequence);
}

// Test 9: Peek on empty queue
TEST_F(BLEQueueManagerTest, PeekFromEmpty) {
  BLEMessage peekMsg;
  uint8_t result = BLEQueueManager_Peek(&queue, &peekMsg);
  
  EXPECT_EQ(result, BLE_QUEUE_EMPTY);
}

// Test 10: Clear queue
TEST_F(BLEQueueManagerTest, ClearQueue) {
  // Fill queue completely (16 messages)
  for(int i = 0; i < 16; i++) {
    testMsg.sequence = i;
    BLEQueueManager_Enqueue(&queue, &testMsg);
  }
  
  // Now cause an overflow by adding more messages to a full queue
  testMsg.sequence = 99;
  for(int i = 0; i < 11; i++) {
    BLEQueueManager_Enqueue(&queue, &testMsg);
  }
  
  EXPECT_EQ(queue.overflowCount, 11);
  EXPECT_EQ(BLEQueueManager_GetCount(&queue), 16);
  
  // Clear queue
  BLEQueueManager_Clear(&queue);
  
  EXPECT_EQ(queue.head, 0);
  EXPECT_EQ(queue.tail, 0);
  EXPECT_EQ(BLEQueueManager_GetCount(&queue), 0);
  EXPECT_TRUE(BLEQueueManager_IsEmpty(&queue));
  // overflowCount should NOT be reset (diagnostic)
  EXPECT_EQ(queue.overflowCount, 11);
}

// Test 11: Index wraparound (circular buffer)
TEST_F(BLEQueueManagerTest, IndexWraparound) {
  // Simulate filling and emptying to move indices
  for(int cycle = 0; cycle < 3; cycle++) {
    for(int i = 0; i < 16; i++) {
      testMsg.sequence = (cycle * 16) + i;
      BLEQueueManager_Enqueue(&queue, &testMsg);
    }
    
    for(int i = 0; i < 16; i++) {
      BLEQueueManager_Dequeue(&queue, &readMsg);
      EXPECT_EQ(readMsg.sequence, (cycle * 16) + i);
    }
  }
  
  EXPECT_EQ(BLEQueueManager_GetCount(&queue), 0);
  EXPECT_TRUE(BLEQueueManager_IsEmpty(&queue));
}

// Test 12: Null pointer safety
TEST_F(BLEQueueManagerTest, NullPointerSafety) {
  // All functions should handle null gracefully
  EXPECT_EQ(BLEQueueManager_GetCount(nullptr), 0);
  EXPECT_TRUE(BLEQueueManager_IsEmpty(nullptr));
  EXPECT_TRUE(BLEQueueManager_IsFull(nullptr));
  
  EXPECT_EQ(BLEQueueManager_Enqueue(nullptr, &testMsg), BLE_QUEUE_FULL);
  EXPECT_EQ(BLEQueueManager_Dequeue(nullptr, &readMsg), BLE_QUEUE_EMPTY);
  EXPECT_EQ(BLEQueueManager_Peek(nullptr, &readMsg), BLE_QUEUE_EMPTY);
  
  // Should not crash
  BLEQueueManager_Clear(nullptr);
  BLEQueueManager_Init(nullptr);
}

// Test 13: Payload preservation
TEST_F(BLEQueueManagerTest, PayloadPreservation) {
  // Create message with specific payload
  testMsg.length = 6;
  testMsg.payload[0] = 0x02;
  testMsg.payload[1] = 0xAA;
  testMsg.payload[2] = 0xBB;
  testMsg.payload[3] = 0xCC;
  testMsg.payload[4] = 0xDD;
  testMsg.payload[5] = 0x04;
  
  BLEQueueManager_Enqueue(&queue, &testMsg);
  BLEQueueManager_Dequeue(&queue, &readMsg);
  
  // Verify payload preserved exactly
  EXPECT_EQ(readMsg.length, 6);
  for(int i = 0; i < 6; i++) {
    EXPECT_EQ(readMsg.payload[i], testMsg.payload[i]);
  }
}

// Test 14: CreateMessage with valid payload
TEST_F(BLEQueueManagerTest, CreateMessageValidPayload) {
  uint8_t payload[] = {0x02, 0xAA, 0xBB, 0xCC, 0xDD, 0x04};
  BLEMessage msg;
  
  uint8_t result = BLEQueueManager_CreateMessage(PACKET_COMMAND, 42, payload, 6, &msg);
  
  EXPECT_EQ(result, BLE_QUEUE_OK);
  EXPECT_EQ(msg.packetType, PACKET_COMMAND);
  EXPECT_EQ(msg.sequence, 42);
  EXPECT_EQ(msg.length, 6);
  EXPECT_EQ(msg.status, BLE_MSG_STATUS_QUEUED);
  
  // Verify payload copied correctly
  for(int i = 0; i < 6; i++) {
    EXPECT_EQ(msg.payload[i], payload[i]);
  }
}

// Test 15: CreateMessage with null payload
TEST_F(BLEQueueManagerTest, CreateMessageNullPayload) {
  BLEMessage msg;
  uint8_t result = BLEQueueManager_CreateMessage(PACKET_COMMAND, 42, nullptr, 6, &msg);
  
  EXPECT_EQ(result, BLE_PACKET_INVALID);
}

// Test 16: CreateMessage with null message buffer
TEST_F(BLEQueueManagerTest, CreateMessageNullMessageBuffer) {
  uint8_t payload[] = {0x02, 0xAA, 0xBB, 0xCC, 0xDD, 0x04};
  uint8_t result = BLEQueueManager_CreateMessage(PACKET_COMMAND, 42, payload, 6, nullptr);
  
  EXPECT_EQ(result, BLE_PACKET_INVALID);
}

// Test 17: CreateMessage with payload too large
TEST_F(BLEQueueManagerTest, CreateMessagePayloadTooLarge) {
  uint8_t payload[BLE_MESSAGE_PAYLOAD_SIZE + 1];
  BLEMessage msg;
  
  uint8_t result = BLEQueueManager_CreateMessage(PACKET_COMMAND, 42, payload, 
                                                  BLE_MESSAGE_PAYLOAD_SIZE + 1, &msg);
  
  EXPECT_EQ(result, BLE_PACKET_INVALID);
}

// Test 18: CreateMessage then enqueue
TEST_F(BLEQueueManagerTest, CreateMessageThenEnqueue) {
  uint8_t payload[] = {0x02, 0x11, 0x22, 0x33, 0x44, 0x04};
  BLEMessage msg;
  
  uint8_t create_result = BLEQueueManager_CreateMessage(PACKET_DATA, 99, payload, 6, &msg);
  EXPECT_EQ(create_result, BLE_QUEUE_OK);
  
  uint8_t enqueue_result = BLEQueueManager_Enqueue(&queue, &msg);
  EXPECT_EQ(enqueue_result, BLE_QUEUE_OK);
  
  uint8_t dequeue_result = BLEQueueManager_Dequeue(&queue, &readMsg);
  EXPECT_EQ(dequeue_result, BLE_QUEUE_OK);
  
  // Verify message made it through queue
  EXPECT_EQ(readMsg.packetType, PACKET_DATA);
  EXPECT_EQ(readMsg.sequence, 99);
  EXPECT_EQ(readMsg.length, 6);
  EXPECT_EQ(readMsg.payload[0], 0x02);
  EXPECT_EQ(readMsg.payload[5], 0x04);
}
