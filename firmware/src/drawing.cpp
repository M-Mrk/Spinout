#include "drawing.h"
#include "Arduino.h"

#include "FastLED.h"
#include "pins.h"
#include "motor.h"

#include <frames.h>

#define DEBUG

constexpr uint16_t kLedCount = 36;
constexpr uint16_t kArmCount = 4;
constexpr uint16_t kFrameCount = 360;

CRGB arm1[kLedCount];
CRGB arm2[kLedCount];
CRGB arm3[kLedCount];
CRGB arm4[kLedCount];
CRGB* arms[] = {arm1, arm2, arm3, arm4};

int g_currentTopArm = 2;
int g_current_order[4] = {2, 3, 0, 1};

void init_leds() {
    FastLED.addLeds<WS2812, arm1_pin, GRB>(arm1, kLedCount);
    FastLED.addLeds<WS2812, arm2_pin, GRB>(arm2, kLedCount);
    FastLED.addLeds<WS2812, arm3_pin, GRB>(arm3, kLedCount);
    FastLED.addLeds<WS2812, arm4_pin, GRB>(arm4, kLedCount);
    FastLED.clear(true);
    FastLED.setBrightness(1);
}

void get_current_order() {
  for (int i = 0; i < 4; ++i) {
    if (i == 0) {
      g_current_order[i] = g_currentTopArm;
    } else {
      g_current_order[i] = (g_currentTopArm + i) % 4;
    }
  }
}

CRGB extract_color(uint32_t color) { // format: 0x00RRGGBB
  return CRGB(color >> 16, (color >> 8) & 0xFF, color & 0xFF);
}

void draw_frame(ImagePtr image, uint16_t degree) {
  long start_time = micros();
  for (uint16_t arm = 0; arm < kArmCount; ++arm) {
    uint16_t arm_index = g_current_order[arm];
    uint16_t frame_degree = (degree + (arm * (kFrameCount / kArmCount))) % kFrameCount;

    for (uint16_t led = 0; led < kLedCount; ++led) {
      uint32_t color = image[frame_degree][led];
      arms[arm_index][led] = extract_color(color);
    }
  }
  FastLED.show();
  #ifdef DEBUG
    long end_time = micros();
    printf("Time taken to show frame: %ld microseconds\n", end_time - start_time);
  #endif
}

void draw_for(ImagePtr image, uint32_t end_time_ms, Motor& motor, void (*service_web)(), volatile bool* active) {
  #ifdef DEBUG
    Serial.println("Drawing until deadline.");
  #endif
    while (millis() < end_time_ms) {
      if (active != nullptr && !*active) {
        break;
      }

      if (service_web != nullptr) {
        service_web();
      }

      get_current_order();
    #ifdef DEBUG
      printf("Current top arm: %d\n", g_currentTopArm);
    #endif

    for (uint16_t degree = 0; degree < kFrameCount; ++degree) {
      if (active != nullptr && !*active) {
        break;
      }

      if (service_web != nullptr) {
        service_web();
      }

      draw_frame(image, degree);
      long wait_time = motor.hall_sensor.micros_till_next_degree();
      if (wait_time > 0) {
        delayMicroseconds(wait_time);
      } else {
        #ifdef DEBUG
          Serial.println("Warning: Negative wait time, skipping delay.");
        #endif
      }
    }

    if (active != nullptr && !*active) {
      break;
    }

    g_currentTopArm = (g_currentTopArm + 1) % 4; // Advance to next arm
  }
  #ifdef DEBUG
    Serial.println("Finished drawing.");
  #endif
}