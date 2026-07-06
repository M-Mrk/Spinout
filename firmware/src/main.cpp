#include <Arduino.h>
#include <FastLED.h>
#include <WiFi.h>
#include <WebServer.h>

#include <pins.h>
#include <html.h>

#include "motor.h"
#include "frames.h"
#include "drawing.h"

#define DEBUG

Motor motor;
IRAM_ATTR void hallISR() {
  motor.hall_sensor.update();
};

ImagePtr g_current_image = test_image;  // default preset

// Helper to select preset by name
bool select_preset(const String& preset) {
    if (preset == "test") {
        g_current_image = test_image;
    } else if (preset == "soup") {
        g_current_image = soup_image;
    } else if (preset == "hackclub") {
        g_current_image = hack_image;
    } else {
        return false; // unknown preset
    }
    return true;
}

constexpr char kAccessPointName[] = "POV-Fan";
constexpr char kAccessPointPassword[] = "povfan123";
constexpr uint32_t kDisplayDurationMs = 60 * 1000;

WebServer server(80);
volatile bool g_fan_running = false;
uint32_t g_display_end_ms = 0;

void service_web() {
  server.handleClient();
}

void set_fan_running(bool running) {
  if (running && g_fan_running) {
    return;
  }

  g_fan_running = running;
  if (!running) {
    g_display_end_ms = 0;
    motor.set_motor_speed(0);
    FastLED.clear(true);
  } else {
    g_display_end_ms = millis() + kDisplayDurationMs;
  }
}

uint32_t remaining_display_ms() {
  if (!g_fan_running) {
    return 0;
  }

  int32_t remaining = static_cast<int32_t>(g_display_end_ms - millis());
  return remaining > 0 ? static_cast<uint32_t>(remaining) : 0;
}

String build_status_json() {
  uint32_t remaining_ms = remaining_display_ms();
  String json = "{";
  json += "\"running\":";
  json += g_fan_running ? "true" : "false";
  json += ",\"displaying\":";
  json += remaining_ms > 0 ? "true" : "false";
  json += ",\"remaining_ms\":";
  json += String(remaining_ms);
  json += ",\"rpm\":";
  json += String(motor.hall_sensor.calculate_rps() * 60.0f, 1);
  json += ",\"motor_speed\":";
  json += String(motor.get_current_speed());
  json += "}";
  return json;
}

void handle_root() {
  server.send(200, "text/html", HTML_CONTENT);
}

void handle_start() {
  if (server.hasArg("preset")) {
    String preset = server.arg("preset");
    if (!select_preset(preset)) {
      Serial.print("Unknown preset requested: ");
      Serial.println(preset);
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"unknown-preset\"}");
      return;
    }
  }

  if (g_fan_running && remaining_display_ms() > 0) {
    server.send(200, "application/json", "{\"ok\":true,\"started\":false,\"reason\":\"already-running\"}");
    return;
  }

  set_fan_running(true);
  server.send(200, "application/json", "{\"ok\":true,\"started\":true,\"running\":true}");
}

void handle_stop() {
  set_fan_running(false);
  server.send(200, "application/json", "{\"ok\":true,\"running\":false}");
}

void handle_status() {
  server.send(200, "application/json", build_status_json());
}

#define AP_MODE
void setup() {
  Serial.begin(115200);
  
  #ifdef AP_MODE
  // AP Mode
  WiFi.mode(WIFI_AP);
  WiFi.softAP(kAccessPointName, NULL);
  #else
  // Station Mode
  WiFi.mode(WIFI_STA);
  wl_status_t status;
  for (int i = 0; i < 10; ++i) {
    status = WiFi.begin("VankeYunCheng", "SJGS6666");
    delay(1000);
    if (status == WL_CONNECTED) {
      break;
    }
  }
  WiFi.setAutoReconnect(true);
  for (int i = 0; i < 20; ++i) {
    switch (status) {
      case WL_CONNECTED:
        Serial.println("WiFi connected");
        break;
      case WL_NO_SSID_AVAIL:
        Serial.println("No SSID available");
        break;
      case WL_CONNECT_FAILED:
        Serial.println("Connection failed");
        break;
      default:
        Serial.print("WiFi status: ");
        Serial.println(status);
    }
  }
  #endif
  
  server.on("/", HTTP_GET, handle_root);
  server.on("/start", HTTP_POST, handle_start);
  server.on("/stop", HTTP_POST, handle_stop);
  server.on("/status", HTTP_GET, handle_status);
  server.begin();

  #ifdef DEBUG
  for (int i = 0; i<10; ++i) {
    #ifdef AP_MODE
    Serial.print("Access point name: ");
    Serial.println(kAccessPointName);
    Serial.print("Access point IP: ");
    Serial.println(WiFi.softAPIP());
    #else
    Serial.print("WiFi connected, IP address: ");
    Serial.println(WiFi.localIP());
    #endif
    delay(100);
  }
  #endif

  // Motor
  motor.set_motor_speed(0);

  // Hall sensor
  pinMode(hall_sensor_pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(hall_sensor_pin), hallISR, FALLING);

  // LEDs
  init_leds();
  pinMode(LED_BUILTIN, OUTPUT);

  set_fan_running(false);
}

void loop() {
  digitalWrite(LED_BUILTIN, LOW);
  service_web();

  if (!g_fan_running) {
    delay(10);
    return;
  }

  if (remaining_display_ms() == 0) {
    set_fan_running(false);
    return;
  }

  digitalWrite(LED_BUILTIN, HIGH);
  uint32_t display_deadline_ms = g_display_end_ms;
  motor.adjust_motor_speed(display_deadline_ms, service_web, &g_fan_running);

  if (!g_fan_running) {
    return;
  }

  if (remaining_display_ms() == 0) {
    set_fan_running(false);
    return;
  }

  while (digitalRead(hall_sensor_pin) == LOW) { //  Wait till arm 1 is on the bottom to align the image
    delayMicroseconds(1);
    if (!g_fan_running || remaining_display_ms() == 0) {
      Serial.println("Fan stopped or display time ended while waiting for arm 1 to align.");
      return;
    }
  }

  draw_for(g_current_image, display_deadline_ms, motor, service_web, &g_fan_running);

  set_fan_running(false);

  #ifdef DEBUG
  Serial.print("RPM: ");
  Serial.println(motor.hall_sensor.calculate_rps() * 60.0f);
  #endif
}
