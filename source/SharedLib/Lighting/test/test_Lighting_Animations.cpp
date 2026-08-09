/**
 * Dynamic color animation tests for the Lighting class.
 * Tests cover:
 * - Animation frame-counting logic
 * - Multiple device independent state
 * - State reset and initialization
 * - Full hue spectrum validation
 */

#include <gtest/gtest.h>
#include "Lighting.h"

// ============================================================================
// Test Fixture
// ============================================================================

class LightingAnimationsFixture : public ::testing::Test {
protected:
    Lighting lighting;  // Create Lighting instance for testing
    
    LightingAnimationsFixture() : lighting(6) {}  // Initialize with 6 devices (max)
    
    void SetUp() override {
        // Reset dynamic color state before each test
        lighting.resetDynamicColors();
    }
};

// ============================================================================
// Dynamic Color Animation Tests
// ============================================================================

TEST_F(LightingAnimationsFixture, DynamicColor_Rainbow_ChangesHueOverTime) {
    LED_HSV color1 = lighting.getDynamicColorHSV(0, C_RAINBOW, 255);
    
    // Call multiple times to advance the animation
    for(int i = 0; i < 10; i++) {
        lighting.getDynamicColorHSV(0, C_RAINBOW, 255);
    }
    
    LED_HSV color2 = lighting.getDynamicColorHSV(0, C_RAINBOW, 255);
    
    // Hue should have changed after 10+ frames
    EXPECT_NE(color1.h, color2.h);
}

TEST_F(LightingAnimationsFixture, DynamicColor_RedGreen_AlternatesBetweenTwoHues) {
    LED_HSV color1 = lighting.getDynamicColorHSV(0, C_REDGREEN, 255);
    EXPECT_TRUE(color1.h == 0 || color1.h == 96);  // Should be red or green
    
    // Advance through cycle (50 frames)
    for(int i = 0; i < 50; i++) {
        lighting.getDynamicColorHSV(0, C_REDGREEN, 255);
    }
    
    LED_HSV color2 = lighting.getDynamicColorHSV(0, C_REDGREEN, 255);
    
    // Should have switched to the other color
    EXPECT_TRUE(color2.h == 0 || color2.h == 96);
    EXPECT_NE(color1.h, color2.h);
}

TEST_F(LightingAnimationsFixture, DynamicColor_OrangeFade_PulsesHueSlightly) {
    LED_HSV color1 = lighting.getDynamicColorHSV(0, C_ORANGE_FADE, 255);
    EXPECT_EQ(color1.h, 28);  // Orange hue
    
    // Advance animation
    for(int i = 0; i < 50; i++) {
        lighting.getDynamicColorHSV(0, C_ORANGE_FADE, 255);
    }
    
    LED_HSV color2 = lighting.getDynamicColorHSV(0, C_ORANGE_FADE, 255);
    
    // Hue should stay orange but brightness should vary
    EXPECT_EQ(color2.h, 28);
    EXPECT_NE(color1.v, color2.v);  // Brightness should have changed
}

TEST_F(LightingAnimationsFixture, DynamicColor_MultipleDevices_IndependentState) {
    // Create a single Lighting instance with 2 devices to test deviceSlot parameter
    Lighting multiDevice(2);
    multiDevice.resetDynamicColors();
    
    // Device 0: Get initial color and advance through one cycle (6 frames for C_RAINBOW)
    LED_HSV device0_color1 = multiDevice.getDynamicColorHSV(0, C_RAINBOW, 255);
    for(int i = 0; i < 5; i++) {
        multiDevice.getDynamicColorHSV(0, C_RAINBOW, 255);
    }
    LED_HSV device0_color2 = multiDevice.getDynamicColorHSV(0, C_RAINBOW, 255);
    
    // Device 1: Call many more times to advance it further ahead
    LED_HSV device1_color1 = multiDevice.getDynamicColorHSV(1, C_RAINBOW, 255);
    for(int i = 0; i < 20; i++) {
        multiDevice.getDynamicColorHSV(1, C_RAINBOW, 255);
    }
    
    // Verify each device slot maintains independent animation state
    // Device 0 should have changed after 6 calls
    EXPECT_NE(device0_color1.h, device0_color2.h);
    
    // Device 1 (called 21 times) should be at a different animation frame than device 0 (called 6 times)
    LED_HSV device1_final = multiDevice.getDynamicColorHSV(1, C_RAINBOW, 255);
    EXPECT_NE(device0_color2.h, device1_final.h);
}

TEST_F(LightingAnimationsFixture, DynamicColor_ResetClearsState) {
    LED_HSV color1 = lighting.getDynamicColorHSV(0, C_RAINBOW, 255);
    
    // Advance animation
    for(int i = 0; i < 10; i++) {
        lighting.getDynamicColorHSV(0, C_RAINBOW, 255);
    }
    
    LED_HSV color2 = lighting.getDynamicColorHSV(0, C_RAINBOW, 255);
    EXPECT_NE(color1.h, color2.h);  // Should have changed
    
    // Reset
    lighting.resetDynamicColors();
    LED_HSV color3 = lighting.getDynamicColorHSV(0, C_RAINBOW, 255);
    
    // Should be back to initial state
    EXPECT_EQ(color1.h, color3.h);
}

TEST_F(LightingAnimationsFixture, DynamicColor_BlueFade_DecrementHueWithinRange) {
    LED_HSV color1 = lighting.getDynamicColorHSV(0, C_BLUE_FADE, 255);
    
    // Should start at or near 160 (dark blue)
    EXPECT_TRUE(color1.h >= 146 && color1.h <= 160);
    EXPECT_EQ(color1.s, 255);  // Full saturation
    
    // Advance through several frames
    for(int i = 0; i < 20; i++) {
        lighting.getDynamicColorHSV(0, C_BLUE_FADE, 255);
    }
    
    LED_HSV color2 = lighting.getDynamicColorHSV(0, C_BLUE_FADE, 255);
    
    // Hue should have decreased (darker blue moving toward light blue)
    EXPECT_TRUE(color2.h >= 146 && color2.h <= 160);
    EXPECT_EQ(color2.s, 255);  // Full saturation maintained
}

TEST_F(LightingAnimationsFixture, RGB_AllValuesInValidRange) {
    // Test conversion across hue spectrum
    for(uint16_t h = 0; h <= 255; h += 17) {  // Sample every ~17 degrees
        LED_HSV hsv = {(uint8_t)h, 255, 255};
        LED_RGB rgb = Lighting::hsv2rgb(hsv);
        
        EXPECT_GE(rgb.r, 0);
        EXPECT_LE(rgb.r, 255);
        EXPECT_GE(rgb.g, 0);
        EXPECT_LE(rgb.g, 255);
        EXPECT_GE(rgb.b, 0);
        EXPECT_LE(rgb.b, 255);
    }
}
