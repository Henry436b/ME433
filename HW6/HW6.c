#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "bsp/board.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "mpu6050.h"

#define LED_PIN    18
#define BTN_PIN    17
#define I2C_PORT   i2c0
#define SDA_PIN    12
#define SCL_PIN    13

// circle mode params
#define CIRCLE_RADIUS  3.0f
#define CIRCLE_STEP    0.05f  // radians per tick

static bool circle_mode = false;
static float circle_angle = 0.0f;

// map acceleration (g) to mouse delta
static int8_t accel_to_delta(float a) {
    float mag = fabsf(a);
    int8_t speed;
    if      (mag < 0.1f) speed = 0;
    else if (mag < 0.3f) speed = 1;
    else if (mag < 0.6f) speed = 3;
    else                 speed = 5;
    return (a < 0) ? -speed : speed;
}

static void send_mouse(int8_t dx, int8_t dy) {
    if (!tud_hid_ready()) return;
    tud_hid_mouse_report(REPORT_ID_MOUSE, 0, dx, dy, 0, 0);
}

void hid_task(void) {
    static uint32_t last_ms = 0;
    uint32_t now = board_millis();
    if (now - last_ms < 10) return;  // 100 Hz
    last_ms = now;

    if (circle_mode) {
        int8_t dx = (int8_t)(CIRCLE_RADIUS * cosf(circle_angle));
        int8_t dy = (int8_t)(CIRCLE_RADIUS * sinf(circle_angle));
        circle_angle += CIRCLE_STEP;
        if (circle_angle > 2.0f * (float)M_PI) circle_angle -= 2.0f * (float)M_PI;
        send_mouse(dx, dy);
    } else {
        imu_data_t d;
        mpu6050_read(I2C_PORT, &d);
        send_mouse(accel_to_delta(d.ax), accel_to_delta(d.ay));
    }
}

static void button_task(void) {
    static bool last_pressed = false;
    bool pressed = !gpio_get(BTN_PIN);  // active low (pull-up)
    if (pressed && !last_pressed) {
        circle_mode = !circle_mode;
        gpio_put(LED_PIN, circle_mode);
    }
    last_pressed = pressed;
}

int main(void) {
    board_init();
    tusb_init();

    // I2C
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    if (mpu6050_check(I2C_PORT) != 0) {
        gpio_init(LED_PIN);
        gpio_set_dir(LED_PIN, GPIO_OUT);
        gpio_put(LED_PIN, 1);
        while (1) tight_loop_contents();
    }
    mpu6050_init(I2C_PORT);

    // LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);  // off = IMU mode

    // button
    gpio_init(BTN_PIN);
    gpio_set_dir(BTN_PIN, GPIO_IN);
    gpio_pull_up(BTN_PIN);

    while (1) {
        tud_task();
        hid_task();
        button_task();
    }
}

//--------------------------------------------------------------------
// TinyUSB callbacks (required)
//--------------------------------------------------------------------
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const *report, uint16_t len) {
    (void)instance; (void)report; (void)len;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                                hid_report_type_t report_type,
                                uint8_t *buffer, uint16_t reqlen) {
    (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                            hid_report_type_t report_type,
                            uint8_t const *buffer, uint16_t bufsize) {
    (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)bufsize;
}