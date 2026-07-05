#include "motor.h"
#include "pins.h"

void Motor::init() {
  analogWriteResolution(this->pwm_resolution);
  pinMode(motor_pin, OUTPUT);
  this->set_motor_speed(0);
}

void Motor::set_motor_speed(uint8_t speed) {
  int constrained_speed = constrain(speed, 0, 100);
  int mapped_speed = map(constrained_speed, 0, 100, 0, this->max_pwm);
  this->current_speed = constrained_speed;
  analogWrite(motor_pin, mapped_speed);
}

void Motor::adjust_motor_speed_once() {
  float rpm = this->hall_sensor.calculate_rps() * 60.0f;
  if (rpm < this->target_rpm * (1.0f - this->target_rpm_tolerance)) {
    if (this->current_speed < 100) {
      this->current_speed += 1;
    }
    this->set_motor_speed(this->current_speed);
  } else if (rpm > this->target_rpm * (1.0f + this->target_rpm_tolerance)) {
    if (this->current_speed > 0) {
      this->current_speed -= 1;
    }
    this->set_motor_speed(this->current_speed);
  }
}

void Motor::adjust_motor_speed(uint32_t end_time_ms, void (*service_web)(), volatile bool* active) {
  #ifdef DEBUG
  Serial.print("Adjusting motor speed. Current speed: ");
  Serial.println(this->current_speed);
  #endif

  int tries = 0;
  while (millis() < end_time_ms) {
    if (active != nullptr && !*active) {
      break;
    }

    if (service_web != nullptr) {
      service_web();
    }

    float rpm = this->hall_sensor.calculate_rps() * 60.0f;
    if (rpm < this->target_rpm * (1.0f - this->target_rpm_tolerance)) {
      if (this->current_speed < 100) {
        this->current_speed += 1;
      }
      this->set_motor_speed(this->current_speed);
    } else if (rpm > this->target_rpm * (1.0f + this->target_rpm_tolerance)) {
      if (this->current_speed > 0) {
        this->current_speed -= 1;
      }
      this->set_motor_speed(this->current_speed);
    } else {
      break; // Exit the loop if RPM is within the tolerance range
    }
    tries++;

    if (service_web != nullptr) {
      service_web();
    }

    if (active != nullptr && !*active) {
      break;
    }

    delay(100); // Small delay to allow the motor speed to adjust
    if (tries > 100) {
      #ifdef DEBUG
      Serial.println("Warning: Unable to stabilize RPM within tolerance after 100 tries.");
      #endif
      break; // Prevent infinite loop in case of issues
    }
  }
}

void HallSensor::update() {
    unsigned long current_time = micros();
    if (last_time != 0) {
        this->last_delta = current_time - this->last_time;
        pulse_count++;
    }
    this->last_time = current_time;
}

#define NUM_MAGNETS 2
float HallSensor::calculate_rps() {
    if (this->last_delta == 0) {
        return 0.0f;
    }
    float period_in_seconds = this->last_delta / 1000000.0f;
    float frequency = 1.0f / period_in_seconds;
    float rps = frequency * NUM_MAGNETS;
    return rps;
}
long HallSensor::micros_till_next_degree() {
    if (this->last_delta == 0) {
        return 0;
    }
    float rps = this->calculate_rps();
    if (rps == 0) {
        return 0;
    }
    float micros_per_degree = 1000000.0f / (rps * 360.0f);
    uint32_t current_micros = micros();
    uint32_t micros_since_last_hall = current_micros - this->last_time;
    long remaining_micros = static_cast<long>(micros_per_degree - micros_since_last_hall);
    if (remaining_micros < 0) {
        remaining_micros = 0;
    }
    return remaining_micros;
}