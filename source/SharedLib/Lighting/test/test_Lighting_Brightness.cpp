/**
 * Brightness conversion and scaling tests for the Lighting class.
 * Tests cover:
 * - Percentage to byte conversion
 * - RGB brightness scaling
 * - Edge cases and rounding
 */

#include <gtest/gtest.h>
#include "Lighting.h"

// ============================================================================
// Test Fixture
// ============================================================================

class LightingBrightnessFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Brightness tests don't require state reset
    }
};

// ============================================================================
// Brightness Conversion Tests
// ============================================================================

TEST_F(LightingBrightnessFixture, BrightnessPercent_0_Returns0) {
    uint8_t result = Lighting::getBrightness(0);
    EXPECT_EQ(result, 0);
}

TEST_F(LightingBrightnessFixture, BrightnessPercent_100_Returns255) {
    uint8_t result = Lighting::getBrightness(100);
    EXPECT_EQ(result, 255);
}

TEST_F(LightingBrightnessFixture, BrightnessPercent_50_Returns127or128) {
    uint8_t result = Lighting::getBrightness(50);
    EXPECT_GE(result, 127);
    EXPECT_LE(result, 128);  // Allow for rounding
}

TEST_F(LightingBrightnessFixture, BrightnessPercent_Over100_Clamped) {
    uint8_t result = Lighting::getBrightness(150);
    EXPECT_EQ(result, 255);  // Should clamp to 255
}

TEST_F(LightingBrightnessFixture, ScaleBrightness_Full_NoChange) {
    LED_RGB original = {255, 128, 64};
    LED_RGB scaled = Lighting::scaleBrightness(original, 255);
    
    EXPECT_EQ(scaled.r, 255);
    EXPECT_EQ(scaled.g, 128);
    EXPECT_EQ(scaled.b, 64);
}

TEST_F(LightingBrightnessFixture, ScaleBrightness_Half_ReducesAllChannels) {
    LED_RGB original = {200, 100, 50};
    LED_RGB scaled = Lighting::scaleBrightness(original, 128);
    
    // Each channel should be approximately half
    EXPECT_LE(scaled.r, 100 + 1);  // Allow for rounding
    EXPECT_LE(scaled.g, 50 + 1);
    EXPECT_LE(scaled.b, 25 + 1);
}

TEST_F(LightingBrightnessFixture, ScaleBrightness_Zero_ReturnsBlack) {
    LED_RGB original = {255, 128, 64};
    LED_RGB scaled = Lighting::scaleBrightness(original, 0);
    
    EXPECT_EQ(scaled.r, 0);
    EXPECT_EQ(scaled.g, 0);
    EXPECT_EQ(scaled.b, 0);
}

// ============================================================================
// Math Utility Tests: nscale8()
// ============================================================================

TEST_F(LightingBrightnessFixture, Nscale8_Scale0_ReturnsZero) {
    uint8_t result = Lighting::nscale8(200, 0);
    EXPECT_EQ(result, 0);
}

TEST_F(LightingBrightnessFixture, Nscale8_Scale255_ReturnsValue) {
    // With FastLED rounding: (200 * 255 + 255) >> 8 = 200
    uint8_t result = Lighting::nscale8(200, 255);
    EXPECT_EQ(result, 200);
}

TEST_F(LightingBrightnessFixture, Nscale8_Scale128_ApproximatelyHalf) {
    // With FastLED rounding: (200 * 128 + 128) >> 8 = 51328 >> 8 = 200
    // Wait, that's with the +128 rounding. Let me recalculate:
    // (200 * 128 + 128) >> 8 = (25600 + 128) >> 8 = 25728 >> 8 = 100
    uint8_t result = Lighting::nscale8(200, 128);
    EXPECT_EQ(result, 100);
}

TEST_F(LightingBrightnessFixture, Nscale8_Scale255_FullRange) {
    // Test edge cases with FastLED rounding formula (value * (scale + 1)) >> 8
    EXPECT_EQ(Lighting::nscale8(0, 255), 0);
    EXPECT_EQ(Lighting::nscale8(255, 255), 255);
    
    // Half scale: (127 * (128 + 1)) >> 8 = (127 * 129) >> 8 = 16383 >> 8 = 63
    EXPECT_EQ(Lighting::nscale8(127, 128), 63);
    
    // Half scale: (128 * (128 + 1)) >> 8 = (128 * 129) >> 8 = 16512 >> 8 = 64
    EXPECT_EQ(Lighting::nscale8(128, 128), 64);
}

// ============================================================================
// Math Utility Tests: scale8_video()
// ============================================================================

TEST_F(LightingBrightnessFixture, Scale8Video_Scale0_ReturnsZero) {
    uint8_t result = Lighting::scale8_video(200, 0);
    EXPECT_EQ(result, 0);
}

TEST_F(LightingBrightnessFixture, Scale8Video_Value0_AlwaysReturnsZero) {
    // Zero values should never light up, even with high scale
    EXPECT_EQ(Lighting::scale8_video(0, 255), 0);
    EXPECT_EQ(Lighting::scale8_video(0, 128), 0);
    EXPECT_EQ(Lighting::scale8_video(0, 1), 0);
}

TEST_F(LightingBrightnessFixture, Scale8Video_Scale255_ReturnsValue) {
    // FastLED scale8_video: ((200 * 255) >> 8) + ((200 && 255) ? 1 : 0)
    // = (51000 >> 8) + 1 = 199 + 1 = 200
    uint8_t result = Lighting::scale8_video(200, 255);
    EXPECT_EQ(result, 200);
}

TEST_F(LightingBrightnessFixture, Scale8Video_NonZeroScale_AddsOne) {
    // FastLED scale8_video: ((200 * 128) >> 8) + ((200 && 128) ? 1 : 0)
    // = (25600 >> 8) + 1 = 100 + 1 = 101
    uint8_t result = Lighting::scale8_video(200, 128);
    EXPECT_EQ(result, 101);
}

TEST_F(LightingBrightnessFixture, Scale8Video_PreventsFadeToBlack) {
    // Test that video scaling prevents complete fade
    uint8_t normal = Lighting::nscale8(10, 128);         // = 5
    uint8_t video = Lighting::scale8_video(10, 128);     // = 6 (with +1)
    
    EXPECT_EQ(normal, 5);
    EXPECT_EQ(video, 6);  // Video version is 1 brighter to prevent flicker
    EXPECT_GT(video, normal);
}
