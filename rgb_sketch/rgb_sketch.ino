#include <Adafruit_NeoPixel.h>
#include <inttypes.h>

#define NEOPIXEL_PIN 0

// We use "pixel" to refer to a single WS2812B package, which contains three LEDs (red, green and blue).

#define NUM_PIXELS 31

// We copy some type name defines here to keep the sketch self contained.
#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define s16 int16_t

// Create the NeoPixel driver object. On the PC, this becomes the emulator.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// This must be below the short int type names and the NeoPixel library init.
#ifndef Arduino_h
#include "arduino_sketch.hpp"
// When running on the PC, we include stdio.h for printf() debugging.
#include <stdio.h>
#endif

// Example of wrapping stuff that will only compile in the IDE.
#ifdef Arduino_h
void debugMarker()
{
  pinMode(0, OUTPUT);
  for (u8 i = 0; i < 10; ++i) {
    digitalWrite(1, HIGH);
    delay(500);
    digitalWrite(1, LOW);
    delay(500);
  }
}
#endif

// End of boilerplate.

void setup()
{
// Arduino_h is only defined when compiling the sketch in the Arduino IDE.
#ifndef Arduino_h
    // Select line, circle or grid pixel layout for emulator.
     pixels.setPixelLayout(Strip);
    //pixels.setPixelLayout(Ring);
    // pixels.setPixelLayout(Grid);
#endif
    pixels.begin();
    // setBrightness() is intended for use only in setup() and not for animations. It's ignored by the emulator (see
    // the README).
    pixels.setBrightness(40);
    pixels.show();
}


struct Segment {
  u32 color;
  s16 speed;
  u8 lengthPercent;
};


void loop()
{
    int runSec = 5;
    u16 numSegments;

    const int maxSegments = 5;
    Segment segmentArr[maxSegments];
    for (u16 i=0; i<3; i++) {
    	taitoRama(); 
    clear(pixels.Color(0, 0, 0), 1000);
	}
    segmentArr[0] = { 0xffffff, 123, 75 };
    segmentArr[1] = { 0x00ff00, 456, 75 };
    segmentArr[2] = { 0xff00ff, 123, 75 };
    smoothRunners(5, 1, segmentArr, 3, true);

    // darker rainbow
    segmentArr[0] = { 0xff00ff, 60, 100 };
    segmentArr[1] = { 0xff0000, 70, 100 };
    smoothRunners(20, 10, segmentArr, 2, false);
}


void taitoRama()
{
    // 0-11 tait - 6 is yellow 
    // 12-13 o yellow
    // 14-26 rama - 19 is yellow a
    // 27-30 taito - yellow
    for (u16 i = 0; i < 12; ++i) {
        if (i == 5) continue;
        pixels.setPixelColor(i, pixels.Color(200,0,200));
        pixels.show();
        delay(10);
    }      
    for (u16 i = 12; i < 14; ++i) {
        pixels.setPixelColor(i, 0xffff00);
        pixels.show();
        delay(10);
    }
    for (u16 i = 14; i < 27; ++i) {
        if (i == 19) continue;
        pixels.setPixelColor(i, pixels.Color(200,0,200));
        pixels.show();
        delay(10);
    }
    for (u16 i = 27; i < 31; ++i) {
        pixels.setPixelColor(i, 0xffff00);
        pixels.show();
        delay(10);
    }
    pixels.setPixelColor(5, 0xffff00);
    pixels.setPixelColor(19, 0xffff00);
    pixels.show();
    delay(10);
}

void smoothRunners(u16 runSec, u16 delayMs, struct Segment* segmentPtrIn, u8 numSegments, bool onlyBombs)
{
    u16 superPosBuf[numSegments];
    u16 numSuperPositions = pixels.numPixels() << 8;
    u16 superPosOffset = numSuperPositions / numSegments;
    for (u8 i = 0; i < numSegments; ++i) {
        superPosBuf[i] = i * superPosOffset;
    }

    u32 startMs = millis();
    while (millis() < startMs + runSec * 1000UL) {
        clear(0, 0);
        Segment* segmentPtr = segmentPtrIn;
        for (u8 i = 0; i < numSegments; ++i) {
            u16 superPos = superPosBuf[i];
            superPos = wrapAdd(superPos, segmentPtr->speed, numSuperPositions);
            superPosBuf[i] = superPos;
            u8 numSegmentPixels = segmentPtr->lengthPercent * pixels.numPixels() / 100;
            if (numSegmentPixels < 2) {
                numSegmentPixels = 2;
            }
            drawTaperedSegment(superPos, numSegmentPixels, segmentPtr->color, onlyBombs);
            ++segmentPtr;
        }
        pixels.show();
        delay(delayMs);
    }
}

void drawTaperedSegment(u16 superPos, u8 numSegmentPixels, u32 color, bool onlyBombs)
{
    u8 shiftPos = superPos & 0xff;
    u8 pixelPos = superPos >> 8;
    for (u8 i = 0; i <= numSegmentPixels; ++i) {
        u32 color1 = calcTaperedSegmentPixel(numSegmentPixels, i, color);
        u32 color2 = calcTaperedSegmentPixel(numSegmentPixels, i + 1, color);
        u32 avgColor = colorWeightedAvg(color1, color2, 255 - shiftPos);
        addPixelColor(pixelPos, avgColor, onlyBombs);
        pixelPos = wrapAdd(pixelPos, 1, pixels.numPixels() - 1);
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

void addPixelColor(u16 pixelPos, u32 addColor, bool onlyBombs)
{
    // 0-11 tait - 5 is yellow 
    // 12-13 o yellow
    // 14-26 rama - 19 is yellow a
    // 27-30 taito - yellow
    switch (pixelPos) {
      case 5: 
      case 12: 
      case 13: 
      case 18: 
      case 27:
      case 28:
      case 29:
      case 30:
        if (onlyBombs) {
        pixels.setPixelColor(pixelPos, pixels.Color(addColor % 255, addColor %255, 0));
	} else {
        	pixels.setPixelColor(pixelPos, pixels.Color(255, 255, 0));
}
        return;
      default: exit;
} 
    if (onlyBombs) {
    pixels.setPixelColor(pixelPos, pixels.Color(200,0,200));

return; }

    u32 oldColor = pixels.getPixelColor(pixelPos);
    u32 newColor = additiveColorMix(oldColor, addColor);
    pixels.setPixelColor(pixelPos, newColor);
}

u32 additiveColorMix(u32 color1, u32 color2)
{
    u8 r1, g1, b1, r2, g2, b2;
    colorPackedToScalar(&r1, &g1, &b1, color1);
    colorPackedToScalar(&r2, &g2, &b2, color2);
    r1 = colorChannelClampedAdd(r1, r2);
    g1 = colorChannelClampedAdd(g1, g2);
    b1 = colorChannelClampedAdd(b1, b2);
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

u8 colorChannelClampedAdd(u8 a, u8 b) {
    u16 c = a + b;
    if (c > 255) {
        c = 255;
    }
    return c;
}

void xmasRedGreenTwinkles(u16 runSec)
{
    const u8 flashOnDelayMs = 20;
    const u8 flashPauseMs = 150;
    const u8 numTwinklesPerSwap = 3;
    bool redOrGreenFirst = false;
    u32 startMs = millis();
    u8 numTwinklesToNextSwap = 0;
    while (millis() < startMs + runSec * 1000UL) {
        if (!numTwinklesToNextSwap--) {
            numTwinklesToNextSwap = numTwinklesPerSwap;
            redOrGreenFirst = !redOrGreenFirst;
        }
        u8 twinkleLedIdx = random(0, pixels.numPixels() - 1);
        pixels.setPixelColor(twinkleLedIdx, 0xffffff);
        pixels.show();
        delay(flashOnDelayMs);
        for (u8 i = 0; i < pixels.numPixels(); ++i) {
            u32 c = (i + redOrGreenFirst) & 1 ? 0xff0000 : 0x00ff00;
            pixels.setPixelColor(i, c);
        }
        pixels.show();
        delay(flashPauseMs);
    }
}

void clear(u32 color, u16 clearMs)
{
    u16 delayMs = clearMs / pixels.numPixels();
    for (u16 i = 0; i < pixels.numPixels(); ++i) {
        pixels.setPixelColor(i, color);
        if (delayMs) {
            pixels.show();
            delay(delayMs);
        }
    }
}

s16 wrap(s16 val, s16 maxVal)
{
    if (val < 0) {
        return val + maxVal;
    }
    else if (val >= maxVal) {
        return val - maxVal;
    }
    else {
        return val;
    }
}

u16 wrapAdd(s16 val, s16 addVal, s16 maxVal)
{
    s16 t = val + addVal;
    if (t > maxVal) {
        return t - maxVal - 1;
    }
    else if (t < 0) {
        return maxVal + t + 1;
    }
    else {
        return t;
    }
}

