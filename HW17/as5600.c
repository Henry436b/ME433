#include "as5600.h"

void as5600_init(i2c_inst_t *i2c, uint sda, uint scl) {
    i2c_init(i2c, 400000);
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    gpio_pull_up(sda);
    gpio_pull_up(scl);
}

uint16_t as5600_read_angle(i2c_inst_t *i2c) {
    uint8_t reg = AS5600_RAW_ANGLE_H;
    uint8_t buf[2];
    i2c_write_blocking(i2c, AS5600_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c, AS5600_ADDR, buf, 2, false);
    return ((uint16_t)(buf[0] & 0x0F) << 8) | buf[1];  // 12-bit value
}

float as5600_read_degrees(i2c_inst_t *i2c) {
    return as5600_read_angle(i2c) * 360.0f / 4096.0f;
}