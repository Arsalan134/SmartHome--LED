#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_INTERNAL

#include "wifi_info.h"
#include <Arduino.h>
#include <FastLED.h>
#include <arduino_homekit_server.h>

#define DATA_PIN D3
#define LED_TYPE WS2811
#define COLOR_ORDER BRG
#define NUM_LEDS 66

CRGB leds[NUM_LEDS];

// access your HomeKit characteristics defined in my_accessory.c
extern "C" homekit_server_config_t accessory_config;
extern "C" homekit_characteristic_t cha_on;
extern "C" homekit_characteristic_t cha_bright;

void setBrightness(int newBrightness) {
  // int delayTime = 15;

  Serial.println(newBrightness);

  FastLED.setBrightness(map(newBrightness, 0, 100, 0, 255));
  // FastLED.delay(delayTime);
}

void smoothBrightness(int from, int to) {
  if (to > from)
    for (int newBrightness = from; newBrightness <= to; newBrightness++)
      setBrightness(newBrightness);
  else
    for (int newBrightness = from; newBrightness >= to; newBrightness--)
      setBrightness(newBrightness);
}

void set_on(const homekit_value_t v) {
  int oldOnValue = cha_on.value.bool_value;
  cha_on.value.bool_value = v.bool_value; // sync the value

  if (oldOnValue != cha_on.value.bool_value) {
    if (cha_on.value.bool_value)
      smoothBrightness(0, cha_bright.value.int_value);
    else
      smoothBrightness(cha_bright.value.int_value, 0);
  }
}

void set_bright(const homekit_value_t v) {
  int newBrightness = v.int_value;

  Serial.println("set_bright");
  Serial.println(newBrightness);

  cha_bright.value.int_value = newBrightness; // sync the value

  FastLED.setBrightness(map(newBrightness, 0, 100, 0, 255));
}

void my_homekit_setup() {
  cha_on.setter = set_on;
  cha_bright.setter = set_bright;
  arduino_homekit_setup(&accessory_config);
}

void my_homekit_loop() { arduino_homekit_loop(); }

void setup() {
  delay(4000); // for recovery

  Serial.begin(9600);

  wifi_connect();

  my_homekit_setup();

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // .setCorrection(TypicalLEDStrip);
  // .setDither();

  FastLED.setBrightness(cha_bright.value.int_value);
}

// This function draws rainbows with an ever-changing,
// widely-varying set of parameters.
void rainbow() {
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  uint8_t sat8 = beatsin88(87, 220, 250);
  uint8_t brightdepth = beatsin88(341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88(203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16; // gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 1, 3000);

  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis;
  sLastMillis = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88(400, 5, 9);
  uint16_t brightnesstheta16 = sPseudotime;

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;

    brightnesstheta16 += brightnessthetainc16;
    uint16_t b16 = sin16(brightnesstheta16) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    CRGB newcolor = CHSV(hue8, sat8, bri8);

    uint16_t pixelnumber = i;
    pixelnumber = (NUM_LEDS - 1) - pixelnumber;

    nblend(leds[pixelnumber], newcolor, 64);
  }
}

void loop() {
  rainbow();

  FastLED.show();

  my_homekit_loop();
}
