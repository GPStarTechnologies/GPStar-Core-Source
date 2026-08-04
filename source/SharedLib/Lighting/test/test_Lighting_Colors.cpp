/**
 * Static color definition tests for the Lighting class.
 * Tests cover:
 * - HSV values for all 24 static colors
 * - Parameter application (brightness and saturation)
 * - Edge cases for special colors
 */

#include <gtest/gtest.h>
#include "Lighting.h"

// ============================================================================
// Test Fixture
// ============================================================================

class LightingColorsFixture : public ::testing::Test {
protected:
    Lighting lighting;  // Create Lighting instance for testing
    
    LightingColorsFixture() : lighting(1) {}  // Initialize with 1 device
    
    void SetUp() override {
        // Color definition tests don't require state reset
    }
};

// ============================================================================
// Static Color Definition Tests
// ============================================================================

TEST_F(LightingColorsFixture, StaticColor_Red_HasCorrectHSV) {
    LED_HSV hsv = lighting.getColorHSV(C_RED, 255, 255);
    EXPECT_EQ(hsv.h, 0);      // Red hue
    EXPECT_EQ(hsv.s, 255);    // Full saturation
    EXPECT_EQ(hsv.v, 255);    // Full brightness
}

TEST_F(LightingColorsFixture, StaticColor_Green_HasCorrectHSV) {
    LED_HSV hsv = lighting.getColorHSV(C_GREEN, 255, 255);
    EXPECT_EQ(hsv.h, 96);     // Green hue
    EXPECT_EQ(hsv.s, 255);
    EXPECT_EQ(hsv.v, 255);
}

TEST_F(LightingColorsFixture, StaticColor_Blue_HasCorrectHSV) {
    LED_HSV hsv = lighting.getColorHSV(C_BLUE, 255, 255);
    EXPECT_EQ(hsv.h, 180);    // Blue hue
    EXPECT_EQ(hsv.s, 255);
    EXPECT_EQ(hsv.v, 255);
}

TEST_F(LightingColorsFixture, StaticColor_Black_HasZeroValues) {
    LED_HSV hsv = lighting.getColorHSV(C_BLACK, 255, 255);
    EXPECT_EQ(hsv.v, 0);      // Black = no brightness (overrides param)
}

TEST_F(LightingColorsFixture, StaticColor_White_HasZeroSaturation) {
    LED_HSV hsv = lighting.getColorHSV(C_WHITE, 255, 255);
    EXPECT_EQ(hsv.s, 0);      // White = no saturation
}

TEST_F(LightingColorsFixture, StaticColor_BrightnessParameterApplied) {
    LED_HSV bright = lighting.getColorHSV(C_RED, 255, 255);
    LED_HSV dim = lighting.getColorHSV(C_RED, 128, 255);
    
    EXPECT_EQ(bright.v, 255);
    EXPECT_EQ(dim.v, 128);    // Brightness parameter should be applied
}

TEST_F(LightingColorsFixture, StaticColor_SaturationParameterApplied) {
    LED_HSV saturated = lighting.getColorHSV(C_RED, 255, 255);
    LED_HSV desaturated = lighting.getColorHSV(C_RED, 255, 128);
    
    EXPECT_EQ(saturated.s, 255);
    EXPECT_EQ(desaturated.s, 128);  // Saturation parameter should be applied
}

TEST_F(LightingColorsFixture, AllStaticColors_ReturnValidHSV) {
    SingleColor colors[] = {
        C_BLACK, C_WHITE, C_WARM_WHITE, C_PINK, C_PASTEL_PINK,
        C_RED, C_LIGHT_RED, C_RED2, C_RED3, C_RED4, C_RED5,
        C_ORANGE, C_BEIGE, C_YELLOW, C_CHARTREUSE,
        C_GREEN, C_DARK_GREEN, C_MINT, C_AQUA,
        C_LIGHT_BLUE, C_MID_BLUE, C_NAVY_BLUE, C_BLUE, C_PURPLE
    };
    
    for(SingleColor color : colors) {
        LED_HSV hsv = lighting.getColorHSV(color, 255, 255);
        // All values should be in valid range
        EXPECT_GE(hsv.h, 0);
        EXPECT_LE(hsv.h, 255);
        EXPECT_GE(hsv.s, 0);
        EXPECT_LE(hsv.s, 255);
        EXPECT_GE(hsv.v, 0);
        EXPECT_LE(hsv.v, 255);
    }
}
