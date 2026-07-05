#pragma once
#include "Arduino.h"
#include "motor.h"

void init_leds();
void draw_for(uint32_t end_time_ms, Motor& motor, void (*service_web)() = nullptr, volatile bool* active = nullptr);