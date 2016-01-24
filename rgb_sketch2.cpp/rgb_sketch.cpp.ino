#include <Adafruit_NeoPixel.h>
#include <inttypes.h>

#define NEOPIXEL_PIN 1

// We use "pixel" to refer to a single WS2812B package, which contains three LEDs (red, green and blue).

// Select number of pixels.
#define NUM_PIXELS 24

// We copy some type name defines here to keep the sketch self contained.
#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define s16 int16_t

// Create the NeoPixel driver object. On the PC, this becomes the emulator.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// This must be below the short int type names and the NeoPixel library init.
#ifndef Arduino_h
#include "rgb_sketch.hpp"
// When running on the PC, we include stdio.h for printf() debugging.
#include <stdio.h>
#endif

// End of boilerplate.

void setup()
{
// Arduino_h is only defined when compiling the sketch in the Arduino IDE.
#ifndef Arduino_h
    // Select line, circle or grid pixel layout for emulator.
    //pixels.setPixelLayout(Strip);
    pixels.setPixelLayout(Ring);
    //pixels.setPixelLayout(Grid);
#endif
    pixels.begin();
    pixels.setBrightness(255);
    pixels.show();
}


struct Segment {
  u32 color;
  s16 speed;
  u8 lengthPercent;
};


void loop()
{
    int runSeconds = 10;
    u16 numSegments;

    // Because I build the segment arrays in memory, I use both flash and sram for the segments and 5 is max for
    // what the ATtiny85 can handle with the current code. TODO: Try initializing the segments as const from literals.
    // That should cause them to stay in flash.
    const int maxSegments = 10;
    Segment segmentArr[maxSegments];

    for (u8 i = 0; i < maxSegments; ++i) {
        segmentArr[i] = { 0xff0000, -60, 20 };
    }
    smoothRunners(runSeconds, 10, segmentArr, maxSegments);
    clear(pixels.Color(0, 0, 0), 1000);


    // Show single colors
    clear(0xff0000, 1500);
    //delay(1000);
    clear(0x000000, 1500);
}

void smoothRunners(u16 run_s, u16 delay_ms, struct Segment* segmentPtrIn, u8 numSegments)
{
    u16 superPosBuf[numSegments];
    u16 numSuperPositions = pixels.numPixels() << 8;
    u16 superPosOffset = numSuperPositions / numSegments;
    for (u8 i = 0; i < numSegments; ++i) {
        superPosBuf[i] = i * superPosOffset;
    }

    u32 start_ms = millis();
    while (millis() < start_ms + (run_s * 1000)) {
        clear(0, 0);
        Segment* segmentPtr = segmentPtrIn;
        for (u8 i = 0; i < numSegments; ++i) {
            u16 superPos = superPosBuf[i];
            superPos = wrap_add(superPos, segmentPtr->speed, numSuperPositions);
            superPosBuf[i] = superPos;
            u8 numSegmentPixels = segmentPtr->lengthPercent * pixels.numPixels() / 100;
            if (numSegmentPixels < 2) {
                numSegmentPixels = 2;
            }
            drawTaperedSegment(superPos, numSegmentPixels, segmentPtr->color);
            ++segmentPtr;
        }
        pixels.show();
        delay(delay_ms);
    }
}

void drawTaperedSegment(u16 superPos, u8 numSegmentPixels, u32 color)
{
    u8 shiftPos = superPos & 0xff;
    u8 pixelPos = superPos >> 8;
    for (u8 i = 0; i <= numSegmentPixels; ++i) {
        u32 color1 = calcTaperedSegmentPixel(numSegmentPixels, i, color);
        u32 color2 = calcTaperedSegmentPixel(numSegmentPixels, i + 1, color);
        u32 avgColor = colorWeightedAvg(color1, color2, 255 - shiftPos);
        addPixelColor(pixelPos, avgColor);
        pixelPos = wrap_add(pixelPos, 1, pixels.numPixels() - 1);
    }
}

// Example for 7 pixels.
//
//        X
//       X X
//      X   X
//     X     X
//
// pixel 3 is center and will be set to brightestVal.
// There are 4 pixels on each side, sharing the center pixel
// The values of the 4 pixels are assigned in such a way that an imagined 5th pixel on each side is 0.
// So, for 4 pixels:
//   single step = 255 / 4 = 63
//   steps for 7 pixels: 64, 127, 191, 255, 191, 127, 64
u32 calcTaperedSegmentPixel(u8 numSegmentPixels, u8 pos, u32 centerColor)
{
    if (pos == 0 || pos == numSegmentPixels + 1) {
        return 0x000000;
    }

    u8 centerPos = numSegmentPixels / 2;
    u8 rCenter, gCenter, bCenter;
    colorPackedToScalar(&rCenter, &gCenter, &bCenter, centerColor);
    u8 rStep = rCenter / (centerPos + 1);
    u8 gStep = gCenter / (centerPos + 1);
    u8 bStep = bCenter / (centerPos + 1);

    u8 segmentIndex;
    if (pos <= centerPos) {
        segmentIndex = pos + 1;
    }
    else {
        segmentIndex = numSegmentPixels - pos;
    }

    return pixels.Color(rStep * segmentIndex, gStep * segmentIndex, bStep * segmentIndex);
}

u32 colorWeightedAvg(u32 color1, u32 color2, u8 weight)
{
    u8 r1, g1, b1, r2, g2, b2;
    colorPackedToScalar(&r1, &g1, &b1, color1);
    colorPackedToScalar(&r2, &g2, &b2, color2);
    r1 = channelWeightedAvg(r1, r2, weight);
    g1 = channelWeightedAvg(g1, g2, weight);
    b1 = channelWeightedAvg(b1, b2, weight);
    return pixels.Color(r1, g1, b1);
}

u8 channelWeightedAvg(u8 a, u8 b, u8 weight)
{
    u16 aa = a * (256 - weight);
    u16 bb = b * weight;
    return (aa + bb) >> 8;
}

void addPixelColor(u16 pixelPos, u32 addColor)
{
    u32 oldColor = pixels.getPixelColor(pixelPos);
    u32 newColor = additiveColorMix(oldColor, addColor);
    pixels.setPixelColor(pixelPos, newColor);
}

u32 additiveColorMix(u32 color1, u32 color2)
{
    u8 r1, g1, b1, r2, g2, b2;
    colorPackedToScalar(&r1, &g1, &b1, color1);
    colorPackedToScalar(&r2, &g2, &b2, color2);
    r1 = colorClampedChannelAdd(r1, r2);
    g1 = colorClampedChannelAdd(g1, g2);
    b1 = colorClampedChannelAdd(b1, b2);
    u32 c = pixels.Color(r1, g1, b1);
    return c;
}

void colorPackedToScalar(u8* r, u8* g, u8* b, u32 color) {
    *b = color;
    color >>= 8;
    *g = color;
    color >>= 8;
    *r = color;
}

u8 colorClampedChannelAdd(u8 a, u8 b) {
    u16 c = a + b;
    if (c > 255) {
        c = 255;
    }
    return c;
}

void clear(u32 color, u16 clear_ms)
{
    u16 delay_ms = clear_ms / pixels.numPixels();
    for (u16 i = 0; i < pixels.numPixels(); ++i) {
        pixels.setPixelColor(i, color);
        if (delay_ms) {
            pixels.show();
            delay(delay_ms);
        }
    }
}

void xmasRedGreenTwinkles(u16 run_s)
{
    u8 flash_on = 20;
    u8 flash_pause = 150;
    u8 twinkles_per_swap = 3;
    u16 p = 0;
    u16 p_cnt = 0;
    u32 start_ms = millis();
    while (millis() < start_ms + (run_s * 1000)) {
        u8 twinkle_i = random(0, pixels.numPixels() - 1);
        pixels.setPixelColor(twinkle_i, pixels.Color(255, 255, 255));
        pixels.show();
        delay(flash_on);
        for (u8 i = 0; i < pixels.numPixels(); ++i) {
            u32 c = (i + p) & 1 ? pixels.Color(255, 0, 0) : pixels.Color(0, 255, 0);
            pixels.setPixelColor(i, c);
        }
        pixels.show();
        delay(flash_pause);
        if (++p_cnt == twinkles_per_swap) {
            p_cnt = 0;
            p += 1;
        }
    }
}

int wrap(int p, int w)
{
    if (p < 0) {
        return p + w;
    }
    else if (p >= w) {
        return p - w;
    }
    else {
        return p;
    }
}

u16 wrap_add(s16 v, s16 add_v, s16 max_v)
{
    s16 t = v + add_v;
    if (t > max_v) {
        return t - max_v - 1;
    }
    else if (t < 0) {
        return max_v + t + 1;
    }
    else {
        return t;
    }
}

