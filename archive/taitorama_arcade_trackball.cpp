
#include <Adafruit_NeoPixel.h>
#include <inttypes.h>

#define NEOPIXEL_PIN 1

// We use "pixel" to refer to a single WS2812B package, which contains three LEDs (red, green and blue).

// Select number of pixels.
#define NUM_PIXELS 16

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
    pixels.setPixelLayout(Grid);
    //pixels.setPixelLayout(Grid);
#endif
    pixels.begin();
    pixels.setBrightness(255);
    pixels.show();
    clear(0xff0000, 1500);
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
    // what the ATtiny85 can handle 
    //with the current code. TODO: Try initializing the segments as const from literals.
    // That should cause them to stay in flash.
    const int maxSegments = 10;
    Segment segmentArr[maxSegments];


    // Show single colors
    fadeOut();
    fadeIn(); 
    delay(300);
}


void fadeOut()
{
   for (u16 i = 255; i > 0; i-=5) {
    for (u16 j = 0; j < pixels.numPixels(); ++j) {
        pixels.setPixelColor(j, pixels.Color(i,0,0));
    }
        delay(20);
        pixels.show();
    }
}

void fadeIn()
{
   for (u16 i =0; i <255; i+=5) {
    for (u16 j = 0; j < pixels.numPixels(); ++j) {
        pixels.setPixelColor(j, pixels.Color(i,0,0));
    }
        delay(20);
        pixels.show();
    }
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


