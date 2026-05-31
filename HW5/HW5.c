#include <stdio.h>
#include "pico/stdlib.h"
#include <stdlib.h>
#include "hardware/i2c.h"
#include "pico/binary_info.h"

// SSD1306 OLED (128x64, I2C addr 0x3C)
#include "ssd1306.h"  // assumes you have an ssd1306 driver

// config registers
#define CONFIG 0x1A
#define GYRO_CONFIG 0x1B
#define ACCEL_CONFIG 0x1C
#define PWR_MGMT_1 0x6B
#define PWR_MGMT_2 0x6C
// sensor data registers:
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

#define MPU_ADDR  0x68
#define I2C_PORT  i2c0
#define SDA_PIN   12
#define SCL_PIN   13
#define LED_PIN   18

// scale factors
#define ACCEL_SCALE 0.000061f   //2g, 16-bit
#define GYRO_SCALE  0.007630f   //2000dps, 16-bit

// OLED dimensions
#define OLED_W 128
#define OLED_H 32
#define CX 64
#define CY 16
#define LINE_LEN 28  // max pixel length of tilt line

// --- I2C helpers ---
static void imu_write(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    i2c_write_blocking(I2C_PORT, MPU_ADDR, buf, 2, false);
}

static uint8_t imu_read_byte(uint8_t reg) {
    uint8_t val;
    i2c_write_blocking(I2C_PORT, MPU_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, MPU_ADDR, &val, 1, false);
    return val;
}

// burst read 14 bytes starting at ACCEL_XOUT_H
static void imu_read_all(uint8_t *buf) {
    uint8_t reg = ACCEL_XOUT_H;
    i2c_write_blocking(I2C_PORT, MPU_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, MPU_ADDR, buf, 14, false);
}

// combine high/low bytes into signed 16-bit
static inline int16_t combine(uint8_t hi, uint8_t lo) {
    return (int16_t)((hi << 8) | lo);
}

// --- IMU init ---
void mpu6050_init(void) {
    imu_write(PWR_MGMT_1,   0x00);  // wake up
    imu_write(ACCEL_CONFIG, 0x00);  // ±2g
    imu_write(GYRO_CONFIG,  0x18);  // ±2000 dps
}

typedef struct {
    float ax, ay, az;   // g
    float gx, gy, gz;   // dps
    float temp;         // °C
} imu_data_t;

void mpu6050_read(imu_data_t *d) {
    uint8_t buf[14];
    imu_read_all(buf);
    d->ax   = combine(buf[0],  buf[1])  * ACCEL_SCALE;
    d->ay   = combine(buf[2],  buf[3])  * ACCEL_SCALE;
    d->az   = combine(buf[4],  buf[5])  * ACCEL_SCALE;
    d->temp = combine(buf[6],  buf[7])  / 340.0f + 36.53f;
    d->gx   = combine(buf[8],  buf[9])  * GYRO_SCALE;
    d->gy   = combine(buf[10], buf[11]) * GYRO_SCALE;
    d->gz   = combine(buf[12], buf[13]) * GYRO_SCALE;
}

// --- draw a line on OLED from center in direction (dx, dy) ---
void draw_line(int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;

    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;

    int err = dx + dy;

    while (1) {
        ssd1306_drawPixel(x0, y0, 1);

        if (x0 == x1 && y0 == y1) {
            break;
        }

        int e2 = 2 * err;

        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }

        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void draw_tilt(float ax, float ay) {
    ssd1306_clear();

    int ex = CX - (int)(ax * LINE_LEN);
    int ey = CY - (int)(ay * LINE_LEN);

    draw_line(CX, CY, ex, ey);

    ssd1306_drawPixel(CX - 1, CY, 1);
    ssd1306_drawPixel(CX + 1, CY, 1);
    ssd1306_drawPixel(CX, CY - 1, 1);
    ssd1306_drawPixel(CX, CY + 1, 1);

    ssd1306_update();
}

int main(void) {
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // I2C @ 400kHz
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    sleep_ms(100);  // let chip power up

    // check WHO_AM_I
    uint8_t who = imu_read_byte(WHO_AM_I);
    if (who != 0x68 && who != 0x98) {
        gpio_put(LED_PIN, 1);
        while (1) tight_loop_contents();  // hang — bad connection
    }

    mpu6050_init();

    // init OLED (I2C addr 0x3C, same i2c bus)
    ssd1306_setup();
    ssd1306_clear();
    ssd1306_update();

    imu_data_t d;
    uint32_t last = 0;

    while (true) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last >= 10) {  // 100 Hz
            last = now;
            mpu6050_read(&d);
            printf("ax=%.3f ay=%.3f az=%.3f | gx=%.1f gy=%.1f gz=%.1f | t=%.2f\n",
                   d.ax, d.ay, d.az, d.gx, d.gy, d.gz, d.temp);
            draw_tilt(d.ax, d.ay);
        }
    }
}