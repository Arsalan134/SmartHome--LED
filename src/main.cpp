#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_INTERNAL

#include "wifi_info.h"
#include <Arduino.h>
#include <FastLED.h>
#include <arduino_homekit_server.h>

#if FASTLED_VERSION < 3001000
#error "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define DATA_PIN D3
#define LED_TYPE WS2811
#define COLOR_ORDER BRG
#define NUM_LEDS 67

#define LOG_D(fmt, ...) printf_P(PSTR(fmt "\n"), ##__VA_ARGS__);

CRGB leds[NUM_LEDS];

bool is_on = false;
float current_brightness = 100.0;

// access your HomeKit characteristics defined in my_accessory.c
extern "C" homekit_server_config_t accessory_config;
extern "C" homekit_characteristic_t cha_on;
extern "C" homekit_characteristic_t cha_bright;

static uint32_t next_heap_millis = 0;

void updateColor() {
  if (is_on) {
    int b = map(current_brightness, 0, 100, 0, 255);
    Serial.println(b);
    FastLED.setBrightness(b);
  } else if (!is_on) {
    Serial.println("is_on == false");
    FastLED.setBrightness(0);
    fill_solid(leds, NUM_LEDS, CRGB::Black);
  }
}

void set_on(const homekit_value_t v) {
  bool on = v.bool_value;
  cha_on.value.bool_value = on; // sync the value
  is_on = true ? on : false;

  updateColor();
}

void set_bright(const homekit_value_t v) {
  Serial.println("set_bright");
  int bright = v.int_value;
  cha_bright.value.int_value = bright; // sync the value

  current_brightness = bright;

  updateColor();
}

void my_homekit_setup() {

  cha_on.setter = set_on;
  cha_bright.setter = set_bright;

  arduino_homekit_setup(&accessory_config);
}

void my_homekit_loop() {
  arduino_homekit_loop();
  const uint32_t t = millis();
  if (t > next_heap_millis) {
    // show heap info every 5 seconds
    next_heap_millis = t + 5 * 1000;
    LOG_D("Free heap: %d, HomeKit clients: %d", ESP.getFreeHeap(),
          arduino_homekit_connected_clients_count());
  }
}

void setup() {
  // delay(2000); // for recovery

  Serial.begin(9600);

  wifi_connect();

  my_homekit_setup();

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS)
      .setCorrection(TypicalLEDStrip)
      .setDither(current_brightness < 255);

  FastLED.setBrightness(current_brightness);
}

// This function draws rainbows with an ever-changing,
// widely-varying set of parameters.
void pride() {
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
  pride();
  // FastLED.delay(10);
  my_homekit_loop();
  delay(10);
  FastLED.show();
}
