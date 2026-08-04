/**
 * HSV→RGB conversion and color channel ordering tests for the Lighting class.
 * Tests cover:
 * - HSV→RGB conversion accuracy across color spectrum
 * - Color channel reordering for different LED strip types
 */

#include <gtest/gtest.h>
#include "Lighting.h"

// ============================================================================
// Test Fixture
// ============================================================================

class LightingConversionFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Conversion tests don't require state reset
    }
};

// ============================================================================
// HSV→RGB Conversion Tests
// ============================================================================

TEST_F(LightingConversionFixture, HSV_Red_ConvertToRGB) {
    // Pure red: H=0, S=255, V=255 should be RGB(255, 0, 0)
    LED_HSV red_hsv = {0, 255, 255};
    LED_RGB red_rgb = Lighting::hsv2rgb(red_hsv);
    
    EXPECT_EQ(red_rgb.r, 255);
    EXPECT_EQ(red_rgb.g, 0);
    EXPECT_EQ(red_rgb.b, 0);
}

TEST_F(LightingConversionFixture, HSV_Green_ConvertToRGB) {
    // Green: H=96, S=255, V=255 should be mostly green with minimal red/blue
    // Uses integer math, so allow some tolerance
    LED_HSV green_hsv = {96, 255, 255};
    LED_RGB green_rgb = Lighting::hsv2rgb(green_hsv);
    
    EXPECT_LT(green_rgb.r, 100);    // Minimal red
    EXPECT_GT(green_rgb.g, 150);    // Strong green
    EXPECT_LT(green_rgb.b, 100);    // Minimal blue
}

TEST_F(LightingConversionFixture, HSV_Blue_ConvertToRGB) {
    // Blue: H=180, S=255, V=255 should be mostly blue with minimal red/green
    // Uses integer math, so allow some tolerance
    LED_HSV blue_hsv = {180, 255, 255};
    LED_RGB blue_rgb = Lighting::hsv2rgb(blue_hsv);
    
    EXPECT_LT(blue_rgb.r, 100);     // Minimal red
    EXPECT_LT(blue_rgb.g, 100);     // Minimal green
    EXPECT_GT(blue_rgb.b, 150);     // Strong blue
}

TEST_F(LightingConversionFixture, HSV_Yellow_ConvertToRGB) {
    // Yellow: H=64 (red+green), S=255, V=255 should be mostly red+green
    // Uses integer math, so allow some tolerance
    LED_HSV yellow_hsv = {64, 255, 255};
    LED_RGB yellow_rgb = Lighting::hsv2rgb(yellow_hsv);
    
    EXPECT_GT(yellow_rgb.r, 100);   // Strong red component
    EXPECT_GT(yellow_rgb.g, 100);   // Strong green component
    EXPECT_LT(yellow_rgb.b, 100);   // Minimal blue
}

TEST_F(LightingConversionFixture, HSV_Cyan_ConvertToRGB) {
    // Cyan: H=128 (green+blue), S=255, V=255 should be mostly green+blue
    // Uses integer math, so allow some tolerance
    LED_HSV cyan_hsv = {128, 255, 255};
    LED_RGB cyan_rgb = Lighting::hsv2rgb(cyan_hsv);
    
    EXPECT_LT(cyan_rgb.r, 100);     // Minimal red
    EXPECT_GT(cyan_rgb.g, 100);     // Strong green component
    EXPECT_GT(cyan_rgb.b, 100);     // Strong blue component
}

TEST_F(LightingConversionFixture, HSV_Magenta_ConvertToRGB) {
    // Magenta: H=192 (red+blue), S=255, V=255 should be mostly red+blue
    // Uses integer math, so allow some tolerance
    LED_HSV magenta_hsv = {192, 255, 255};
    LED_RGB magenta_rgb = Lighting::hsv2rgb(magenta_hsv);
    
    EXPECT_GT(magenta_rgb.r, 100);  // Strong red component
    EXPECT_LT(magenta_rgb.g, 100);  // Minimal green
    EXPECT_GT(magenta_rgb.b, 100);  // Strong blue component
}

TEST_F(LightingConversionFixture, HSV_ZeroSaturation_ReturnsGray) {
    // When S=0, color should be gray (all channels equal to V)
    LED_HSV gray_hsv = {100, 0, 200};
    LED_RGB gray_rgb = Lighting::hsv2rgb(gray_hsv);
    
    EXPECT_EQ(gray_rgb.r, 200);
    EXPECT_EQ(gray_rgb.g, 200);
    EXPECT_EQ(gray_rgb.b, 200);
}

TEST_F(LightingConversionFixture, HSV_ZeroValue_ReturnsBlack) {
    // When V=0, result should be black (all zeros)
    LED_HSV black_hsv = {0, 255, 0};
    LED_RGB black_rgb = Lighting::hsv2rgb(black_hsv);
    
    EXPECT_EQ(black_rgb.r, 0);
    EXPECT_EQ(black_rgb.g, 0);
    EXPECT_EQ(black_rgb.b, 0);
}

TEST_F(LightingConversionFixture, HSV_ReducedBrightness_ScalesAllChannels) {
    // RGB scaled by value should be proportional
    LED_HSV bright_red = {0, 255, 255};
    LED_HSV dim_red = {0, 255, 128};
    
    LED_RGB bright_rgb = Lighting::hsv2rgb(bright_red);
    LED_RGB dim_rgb = Lighting::hsv2rgb(dim_red);
    
    // Dim red should have proportionally lower values
    EXPECT_LT(dim_rgb.r, bright_rgb.r);
    EXPECT_LE(dim_rgb.g, bright_rgb.g);
    EXPECT_LE(dim_rgb.b, bright_rgb.b);
}

// ============================================================================
// Color Channel Ordering Tests
// ============================================================================

TEST_F(LightingConversionFixture, ColorOrder_RGB_NoChange) {
    LED_RGB original = {255, 0, 128};
    LED_RGB reordered = Lighting::applyColorOrder(original, ORDER_RGB);
    
    EXPECT_EQ(reordered.r, 255);
    EXPECT_EQ(reordered.g, 0);
    EXPECT_EQ(reordered.b, 128);
}

TEST_F(LightingConversionFixture, ColorOrder_GRB_SwapsRG) {
    LED_RGB original = {255, 128, 64};
    LED_RGB reordered = Lighting::applyColorOrder(original, ORDER_GRB);
    
    EXPECT_EQ(reordered.r, 128);  // G moved to R
    EXPECT_EQ(reordered.g, 255);  // R moved to G
    EXPECT_EQ(reordered.b, 64);   // B stays same
}

TEST_F(LightingConversionFixture, ColorOrder_GBR_RotatesChannels) {
    LED_RGB original = {255, 128, 64};
    LED_RGB reordered = Lighting::applyColorOrder(original, ORDER_GBR);
    
    EXPECT_EQ(reordered.r, 128);  // G→R
    EXPECT_EQ(reordered.g, 64);   // B→G
    EXPECT_EQ(reordered.b, 255);  // R→B
}
