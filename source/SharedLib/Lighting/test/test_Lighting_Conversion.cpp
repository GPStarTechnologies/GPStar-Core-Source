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

TEST_F(LightingConversionFixture, HSV_HueSectorBoundary_43_RedGreen) {
    // H=43 is near the red/green sector boundary (should be yellow-ish)
    LED_HSV boundary_hsv = {43, 255, 255};
    LED_RGB boundary_rgb = Lighting::hsv2rgb(boundary_hsv);
    
    // Should have significant red and green, minimal blue
    EXPECT_GT(boundary_rgb.r, 100);
    EXPECT_GT(boundary_rgb.g, 100);
    EXPECT_LT(boundary_rgb.b, 50);
}

TEST_F(LightingConversionFixture, HSV_HueSectorBoundary_85_GreenBlue) {
    // H=85 is near green/cyan sector boundary
    LED_HSV boundary_hsv = {85, 255, 255};
    LED_RGB boundary_rgb = Lighting::hsv2rgb(boundary_hsv);
    
    // Should have minimal red, strong green
    EXPECT_LT(boundary_rgb.r, 50);
    EXPECT_GT(boundary_rgb.g, 200);
}

TEST_F(LightingConversionFixture, HSV_HueSectorBoundary_170_BlueRed) {
    // H=170 is near cyan/magenta sector boundary
    LED_HSV boundary_hsv = {170, 255, 255};
    LED_RGB boundary_rgb = Lighting::hsv2rgb(boundary_hsv);
    
    // Should have minimal green, strong blue
    EXPECT_LT(boundary_rgb.g, 50);
    EXPECT_GT(boundary_rgb.b, 200);
}

TEST_F(LightingConversionFixture, HSV_MidRangeSaturation_DesaturatedRed) {
    // Mid-range saturation: H=0, S=128, V=255 should be light red
    LED_HSV desaturated = {0, 128, 255};
    LED_RGB rgb = Lighting::hsv2rgb(desaturated);
    
    // Red dominates but with less intensity than pure color
    EXPECT_EQ(rgb.r, 255);           // Max value
    EXPECT_GT(rgb.g, 100);           // Raised floor due to desaturation
    EXPECT_GT(rgb.b, 100);           // Raised floor due to desaturation
    EXPECT_LT(rgb.g, 200);           // But not too high
    EXPECT_LT(rgb.b, 200);           // But not too high
}

TEST_F(LightingConversionFixture, HSV_White_AnyHueFullValueZeroSaturation) {
    // White is any hue with S=0 and V=255
    // Test multiple hues to ensure they all produce white
    LED_HSV white1 = {0, 0, 255};
    LED_HSV white2 = {85, 0, 255};
    LED_HSV white3 = {170, 0, 255};
    
    LED_RGB rgb1 = Lighting::hsv2rgb(white1);
    LED_RGB rgb2 = Lighting::hsv2rgb(white2);
    LED_RGB rgb3 = Lighting::hsv2rgb(white3);
    
    // All should be white (255, 255, 255)
    EXPECT_EQ(rgb1.r, 255);
    EXPECT_EQ(rgb1.g, 255);
    EXPECT_EQ(rgb1.b, 255);
    
    EXPECT_EQ(rgb2.r, 255);
    EXPECT_EQ(rgb2.g, 255);
    EXPECT_EQ(rgb2.b, 255);
    
    EXPECT_EQ(rgb3.r, 255);
    EXPECT_EQ(rgb3.g, 255);
    EXPECT_EQ(rgb3.b, 255);
}

TEST_F(LightingConversionFixture, HSV_LowValue_DimColor) {
    // Low value with full saturation should produce dim colors
    LED_HSV dim_red = {0, 255, 50};
    LED_RGB rgb = Lighting::hsv2rgb(dim_red);
    
    // All channels should be relatively low
    EXPECT_LT(rgb.r, 100);
    EXPECT_LT(rgb.g, 50);
    EXPECT_LT(rgb.b, 50);
}

TEST_F(LightingConversionFixture, HSV_WarmWhite_ConvertToRGB) {
    // C_WARM_WHITE: H=36, S=183, V=255
    // Expected: RGB(255, 227, 71) - warm orange tone
    LED_HSV warmwhite_hsv = {36, 183, 255};
    LED_RGB warmwhite_rgb = Lighting::hsv2rgb(warmwhite_hsv);
    
    EXPECT_EQ(warmwhite_rgb.r, 255);
    EXPECT_EQ(warmwhite_rgb.g, 227);
    EXPECT_EQ(warmwhite_rgb.b, 71);
    EXPECT_LT(warmwhite_rgb.g, warmwhite_rgb.r);  // Verify G < R for warm tone
}

TEST_F(LightingConversionFixture, HSV_Orange_ConvertToRGB) {
    // C_ORANGE (hue 32): H=32, S=255, V=255
    // Expected: pure orange with R >> G
    LED_HSV orange_hsv = {32, 255, 255};
    LED_RGB orange_rgb = Lighting::hsv2rgb(orange_hsv);
    
    EXPECT_GT(orange_rgb.r, 240);
    EXPECT_GT(orange_rgb.g, 100);
    EXPECT_LT(orange_rgb.b, 50);
    EXPECT_LT(orange_rgb.g, orange_rgb.r);  // Verify G < R for orange
}

TEST_F(LightingConversionFixture, HSV_Yellow_ConvertToRGB_Diagnostic) {
    // C_YELLOW (hue 64): H=64, S=255, V=255 - pure yellow
    LED_HSV yellow_hsv = {64, 255, 255};
    LED_RGB yellow_rgb = Lighting::hsv2rgb(yellow_hsv);
    
    EXPECT_GT(yellow_rgb.r, 100);
    EXPECT_GT(yellow_rgb.g, 100);
    EXPECT_LT(yellow_rgb.b, 100);
}

TEST_F(LightingConversionFixture, HSV_Hue36_AllSaturations_Specific) {
    // Focus specifically on H=36, S=183 vs other saturation values
    LED_HSV test_hsv = {36, 183, 255};
    LED_RGB test_rgb = Lighting::hsv2rgb(test_hsv);
    
    // Verify warm white has R > G
    EXPECT_GT(test_rgb.r, test_rgb.g);
}

TEST_F(LightingConversionFixture, HSV_WarmWhite_ApplyOrderGRB) {
    // If we changed NeutronaWand to use ORDER_GRB instead of ORDER_RGB,
    // what would be sent to pixels.Color()?
    // This shows what happens if we apply "Fix Option B" (pre-swap in library)
    
    LED_HSV warmwhite_hsv = {36, 183, 255};
    LED_RGB rgb = Lighting::hsv2rgb(warmwhite_hsv);  // RGB(255, 227, 71)
    
    // Apply ORDER_GRB ordering
    LED_RGB grb_ordered = Lighting::applyColorOrder(rgb, ORDER_GRB);
    
    // Verify it produces RGB(227, 255, 71) - G and R swapped
    EXPECT_EQ(grb_ordered.r, 227);
    EXPECT_EQ(grb_ordered.g, 255);
    EXPECT_EQ(grb_ordered.b, 71);
    // With NEO_GRB, this would be re-swapped to RGB(255, 227, 71) at hardware
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

TEST_F(LightingConversionFixture, ColorOrder_RBG_SwapsBG) {
    LED_RGB original = {255, 128, 64};
    LED_RGB reordered = Lighting::applyColorOrder(original, ORDER_RBG);
    
    EXPECT_EQ(reordered.r, 255);  // R stays same
    EXPECT_EQ(reordered.g, 64);   // B moved to G
    EXPECT_EQ(reordered.b, 128);  // G moved to B
}

TEST_F(LightingConversionFixture, ColorOrder_BRG_RotatesChannels2) {
    LED_RGB original = {255, 128, 64};
    LED_RGB reordered = Lighting::applyColorOrder(original, ORDER_BRG);
    
    EXPECT_EQ(reordered.r, 64);   // B→R
    EXPECT_EQ(reordered.g, 255);  // R→G
    EXPECT_EQ(reordered.b, 128);  // G→B
}

TEST_F(LightingConversionFixture, ColorOrder_BGR_ReversesChannels) {
    LED_RGB original = {255, 128, 64};
    LED_RGB reordered = Lighting::applyColorOrder(original, ORDER_BGR);
    
    EXPECT_EQ(reordered.r, 64);   // B→R
    EXPECT_EQ(reordered.g, 128);  // G stays same
    EXPECT_EQ(reordered.b, 255);  // R→B
}
