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
    LED_HSV red_hsv = {0, 255, 255};
    LED_RGB red_rgb = Lighting::hsv2rgb(red_hsv);
    
    EXPECT_EQ(red_rgb.r, 255);
    EXPECT_EQ(red_rgb.g, 0);
    EXPECT_EQ(red_rgb.b, 0);
}

TEST_F(LightingConversionFixture, HSV_Green_ConvertToRGB) {
    LED_HSV green_hsv = {96, 255, 255};
    LED_RGB green_rgb = Lighting::hsv2rgb(green_hsv);
    
    EXPECT_EQ(green_rgb.r, 0);
    EXPECT_EQ(green_rgb.g, 255);
    EXPECT_EQ(green_rgb.b, 0);
}

TEST_F(LightingConversionFixture, HSV_Blue_ConvertToRGB) {
    LED_HSV blue_hsv = {180, 255, 255};
    LED_RGB blue_rgb = Lighting::hsv2rgb(blue_hsv);
    
    EXPECT_EQ(blue_rgb.r, 53);
    EXPECT_EQ(blue_rgb.g, 0);
    EXPECT_EQ(blue_rgb.b, 202);
}

TEST_F(LightingConversionFixture, HSV_Yellow_ConvertToRGB) {
    LED_HSV yellow_hsv = {64, 255, 255};
    LED_RGB yellow_rgb = Lighting::hsv2rgb(yellow_hsv);
    
    EXPECT_EQ(yellow_rgb.r, 171);
    EXPECT_EQ(yellow_rgb.g, 170);
    EXPECT_EQ(yellow_rgb.b, 0);
}

TEST_F(LightingConversionFixture, HSV_Cyan_ConvertToRGB) {
    LED_HSV cyan_hsv = {128, 255, 255};
    LED_RGB cyan_rgb = Lighting::hsv2rgb(cyan_hsv);
    
    EXPECT_EQ(cyan_rgb.r, 0);
    EXPECT_EQ(cyan_rgb.g, 171);
    EXPECT_EQ(cyan_rgb.b, 85);
}

TEST_F(LightingConversionFixture, HSV_Magenta_ConvertToRGB) {
    LED_HSV magenta_hsv = {192, 255, 255};
    LED_RGB magenta_rgb = Lighting::hsv2rgb(magenta_hsv);
    
    EXPECT_EQ(magenta_rgb.r, 85);
    EXPECT_EQ(magenta_rgb.g, 0);
    EXPECT_EQ(magenta_rgb.b, 171);
}

TEST_F(LightingConversionFixture, HSV_ZeroSaturation_ReturnsGray) {
    // When S=0, color should be white (fully desaturated)
    LED_HSV gray_hsv = {100, 0, 200};
    LED_RGB gray_rgb = Lighting::hsv2rgb(gray_hsv);
    
    EXPECT_EQ(gray_rgb.r, 255);
    EXPECT_EQ(gray_rgb.g, 255);
    EXPECT_EQ(gray_rgb.b, 255);
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
    LED_HSV boundary_hsv = {43, 255, 255};
    LED_RGB boundary_rgb = Lighting::hsv2rgb(boundary_hsv);
    
    EXPECT_EQ(boundary_rgb.r, 171);
    EXPECT_EQ(boundary_rgb.g, 114);
    EXPECT_EQ(boundary_rgb.b, 0);
}

TEST_F(LightingConversionFixture, HSV_HueSectorBoundary_85_GreenBlue) {
    LED_HSV boundary_hsv = {85, 255, 255};
    LED_RGB boundary_rgb = Lighting::hsv2rgb(boundary_hsv);
    
    EXPECT_EQ(boundary_rgb.r, 60);
    EXPECT_EQ(boundary_rgb.g, 225);
    EXPECT_EQ(boundary_rgb.b, 0);
}

TEST_F(LightingConversionFixture, HSV_HueSectorBoundary_170_BlueRed) {
    LED_HSV boundary_hsv = {170, 255, 255};
    LED_RGB boundary_rgb = Lighting::hsv2rgb(boundary_hsv);
    
    EXPECT_EQ(boundary_rgb.r, 26);
    EXPECT_EQ(boundary_rgb.g, 0);
    EXPECT_EQ(boundary_rgb.b, 229);
}

TEST_F(LightingConversionFixture, HSV_MidRangeSaturation_DesaturatedRed) {
    LED_HSV desaturated = {0, 128, 255};
    LED_RGB rgb = Lighting::hsv2rgb(desaturated);
    
    EXPECT_EQ(rgb.r, 255);
    EXPECT_EQ(rgb.g, 63);
    EXPECT_EQ(rgb.b, 63);
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
    LED_HSV warmwhite_hsv = {36, 183, 255};
    LED_RGB warmwhite_rgb = Lighting::hsv2rgb(warmwhite_hsv);
    
    EXPECT_EQ(warmwhite_rgb.r, 174);
    EXPECT_EQ(warmwhite_rgb.g, 189);
    EXPECT_EQ(warmwhite_rgb.b, 89);
    EXPECT_LT(warmwhite_rgb.r, warmwhite_rgb.g);  // G is slightly higher than R
}

TEST_F(LightingConversionFixture, HSV_Orange_ConvertToRGB) {
    LED_HSV orange_hsv = {32, 255, 255};
    LED_RGB orange_rgb = Lighting::hsv2rgb(orange_hsv);
    
    EXPECT_EQ(orange_rgb.r, 171);
    EXPECT_EQ(orange_rgb.g, 85);
    EXPECT_EQ(orange_rgb.b, 0);
}

TEST_F(LightingConversionFixture, HSV_Yellow_ConvertToRGB_Diagnostic) {
    LED_HSV yellow_hsv = {64, 255, 255};
    LED_RGB yellow_rgb = Lighting::hsv2rgb(yellow_hsv);
    
    EXPECT_EQ(yellow_rgb.r, 171);
    EXPECT_EQ(yellow_rgb.g, 170);
    EXPECT_EQ(yellow_rgb.b, 0);
}

TEST_F(LightingConversionFixture, HSV_Hue36_AllSaturations_Specific) {
    LED_HSV test_hsv = {36, 183, 255};
    LED_RGB test_rgb = Lighting::hsv2rgb(test_hsv);
    
    EXPECT_EQ(test_rgb.r, 174);
    EXPECT_EQ(test_rgb.g, 189);
    EXPECT_EQ(test_rgb.b, 89);
    EXPECT_GT(test_rgb.g, test_rgb.r);
}

TEST_F(LightingConversionFixture, HSV_WarmWhite_ApplyOrderGRB) {
    LED_HSV warmwhite_hsv = {36, 183, 255};
    LED_RGB rgb = Lighting::hsv2rgb(warmwhite_hsv);
    
    LED_RGB grb_ordered = Lighting::applyColorOrder(rgb, ORDER_GRB);
    
    EXPECT_EQ(grb_ordered.r, 189);
    EXPECT_EQ(grb_ordered.g, 174);
    EXPECT_EQ(grb_ordered.b, 89);
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

// ============================================================================
// Color Strip Type Correction Tests
// ============================================================================

TEST_F(LightingConversionFixture, ColorCorrection_Uncorrected_NoChange) {
    // LED_UNCORRECTED should return colors unchanged (255, 255, 255 correction factor)
    Lighting lighting(1, 6, LED_UNCORRECTED);
    
    LED_RGB original = {200, 150, 100};
    LED_RGB corrected = lighting.applyColorCorrection(original);
    
    EXPECT_EQ(corrected.r, 200);
    EXPECT_EQ(corrected.g, 150);
    EXPECT_EQ(corrected.b, 100);
}

TEST_F(LightingConversionFixture, ColorCorrection_TypicalStrip_ReducesBlue) {
    Lighting lighting(1, 6, LED_TYPICAL_STRIP);
    
    LED_RGB original = {255, 255, 255};
    LED_RGB corrected = lighting.applyColorCorrection(original);
    
    EXPECT_EQ(corrected.r, 255);
    EXPECT_EQ(corrected.g, 176);
    EXPECT_EQ(corrected.b, 240);
}

TEST_F(LightingConversionFixture, ColorCorrection_TypicalStrip_PreservesBlack) {
    Lighting lighting(1, 6, LED_TYPICAL_STRIP);
    
    LED_RGB original = {0, 0, 0};
    LED_RGB corrected = lighting.applyColorCorrection(original);
    
    EXPECT_EQ(corrected.r, 0);
    EXPECT_EQ(corrected.g, 0);
    EXPECT_EQ(corrected.b, 0);
}

TEST_F(LightingConversionFixture, ColorCorrection_TypicalStrip_PartialColor) {
    Lighting lighting(1, 6, LED_TYPICAL_STRIP);
    
    LED_RGB original = {200, 100, 80};
    LED_RGB corrected = lighting.applyColorCorrection(original);
    
    EXPECT_EQ(corrected.r, 200);
    EXPECT_EQ(corrected.g, 69);
    EXPECT_EQ(corrected.b, 75);
}

TEST_F(LightingConversionFixture, ColorCorrection_SetAtRuntime) {
    Lighting lighting(1, 6, LED_UNCORRECTED);
    
    LED_RGB original = {255, 255, 255};
    LED_RGB uncorrected = lighting.applyColorCorrection(original);
    
    EXPECT_EQ(uncorrected.r, 255);
    EXPECT_EQ(uncorrected.g, 255);
    EXPECT_EQ(uncorrected.b, 255);
    
    lighting.setColorCorrection(LED_TYPICAL_STRIP);
    LED_RGB corrected = lighting.applyColorCorrection(original);
    
    EXPECT_EQ(corrected.r, 255);
    EXPECT_EQ(corrected.g, 176);
    EXPECT_EQ(corrected.b, 240);
}
