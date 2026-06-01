#include "mpu6050.h"

static void imu_write(i2c_inst_t *i2c, uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    i2c_write_blocking(i2c, MPU_ADDR, buf, 2, false);
}

static uint8_t imu_read_byte(i2c_inst_t *i2c, uint8_t reg) {
    uint8_t val;
    i2c_write_blocking(i2c, MPU_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c, MPU_ADDR, &val, 1, false);
    return val;
}

static void imu_read_all(i2c_inst_t *i2c, uint8_t *buf) {
    uint8_t reg = ACCEL_XOUT_H;
    i2c_write_blocking(i2c, MPU_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c, MPU_ADDR, buf, 14, false);
}

static inline int16_t combine(uint8_t hi, uint8_t lo) {
    return (int16_t)((hi << 8) | lo);
}

void mpu6050_init(i2c_inst_t *i2c) {
    imu_write(i2c, PWR_MGMT_1,   0x00);  // wake up
    imu_write(i2c, ACCEL_CONFIG, 0x00);  // ±2g
    imu_write(i2c, GYRO_CONFIG,  0x18);  // ±2000 dps
}

int mpu6050_check(i2c_inst_t *i2c) {
    uint8_t who = imu_read_byte(i2c, WHO_AM_I);
    return (who == 0x68 || who == 0x98) ? 0 : -1;
}

void mpu6050_read(i2c_inst_t *i2c, imu_data_t *d) {
    uint8_t buf[14];
    imu_read_all(i2c, buf);
    d->ax   = combine(buf[0],  buf[1])  * ACCEL_SCALE;
    d->ay   = combine(buf[2],  buf[3])  * ACCEL_SCALE;
    d->az   = combine(buf[4],  buf[5])  * ACCEL_SCALE;
    d->temp = combine(buf[6],  buf[7])  / 340.0f + 36.53f;
    d->gx   = combine(buf[8],  buf[9])  * GYRO_SCALE;
    d->gy   = combine(buf[10], buf[11]) * GYRO_SCALE;
    d->gz   = combine(buf[12], buf[13]) * GYRO_SCALE;
}
