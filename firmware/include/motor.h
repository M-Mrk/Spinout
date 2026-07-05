#pragma once
#include <Arduino.h>
#include "pins.h"

class Motor;

class HallSensor {
public:
    void update();
    float calculate_rps();
    long micros_till_next_degree();
    
    unsigned long get_last_delta() const { return last_delta; }
    unsigned long get_pulse_count() const { return pulse_count; }
    
private:
    volatile unsigned long last_time = 0;
    volatile unsigned long last_delta = 0;
    volatile unsigned long pulse_count = 0;
    
    friend class Motor;
};

class Motor {
public:
    HallSensor hall_sensor;
    
    void init();
    void set_motor_speed(uint8_t speed);
    void adjust_motor_speed_once();
    void adjust_motor_speed(uint32_t end_time_ms, void (*service_web)() = nullptr, volatile bool* active = nullptr);

    int get_current_speed() const { return current_speed; }
private:
    int target_rpm = 1000;
    float target_rpm_tolerance = 0.05f;
    int pwm_resolution = 8;
    int max_pwm = 255;
    uint8_t current_speed = 0; // Current speed in percentage (0-100)
};
