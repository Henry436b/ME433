#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "mpu6050.h"

#define PB 16
#define SDA_PIN 12
#define SCL_PIN 13

int main(void) {
    stdio_init_all();

    i2c_init(i2c0, 400 * 1000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    gpio_init(PB);
    gpio_set_dir(PB, GPIO_IN);
    gpio_pull_up(PB);

    mpu6050_init(i2c0);// Probably should check the WHO_AM_I register, but we ball

    imu_data_t d; //Struct d

    while (true) {
        mpu6050_read(i2c0, &d);
        int btn = gpio_get(PB) ? 0 : 1;
        printf("{\"ax\":%.3f,\"ay\":%.3f,\"btn\":%d}\n", d.ax, d.ay, btn); //Print JSON line over USB Serial
        sleep_ms(33);
    }
}