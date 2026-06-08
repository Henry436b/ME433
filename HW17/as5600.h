#pragma once
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define AS5600_ADDR 0x36
#define AS5600_RAW_ANGLE_H 0x0C
#define AS5600_RAW_ANGLE_L 0x0D

void as5600_init(i2c_inst_t *i2c, uint sda, uint scl);
uint16_t as5600_read_angle(i2c_inst_t *i2c);  // returns 0-4095
float as5600_read_degrees(i2c_inst_t *i2c);   // returns 0-360