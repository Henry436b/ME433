#pragma once
#include <stdint.h>
#include "hardware/i2c.h"

// config registers
#define CONFIG       0x1A
#define GYRO_CONFIG  0x1B
#define ACCEL_CONFIG 0x1C
#define PWR_MGMT_1   0x6B
#define PWR_MGMT_2   0x6C
// sensor data registers
#define ACCEL_XOUT_H 0x3B
#define ACCEL_XOUT_L 0x3C
#define ACCEL_YOUT_H 0x3D
#define ACCEL_YOUT_L 0x3E
#define ACCEL_ZOUT_H 0x3F
#define ACCEL_ZOUT_L 0x40
#define TEMP_OUT_H   0x41
#define TEMP_OUT_L   0x42
#define GYRO_XOUT_H  0x43
#define GYRO_XOUT_L  0x44
#define GYRO_YOUT_H  0x45
#define GYRO_YOUT_L  0x46
#define GYRO_ZOUT_H  0x47
#define GYRO_ZOUT_L  0x48
#define WHO_AM_I     0x75

#define MPU_ADDR    0x68
#define ACCEL_SCALE 0.000061f  // ±2g, 16-bit
#define GYRO_SCALE  0.007630f  // ±2000dps, 16-bit

typedef struct {
    float ax, ay, az;  // g
    float gx, gy, gz;  // dps
    float temp;        // °C
} imu_data_t;

// pass in your i2c instance (e.g. i2c0)
void    mpu6050_init(i2c_inst_t *i2c);
// returns 0 on success, -1 if WHO_AM_I check fails
int     mpu6050_check(i2c_inst_t *i2c);
void    mpu6050_read(i2c_inst_t *i2c, imu_data_t *d);
