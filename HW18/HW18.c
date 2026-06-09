#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "as5600.h"
#include "hx711.h"

// Pin definitions
#define INA_SDA     10
#define INA_SCL     11
#define ENC_SDA     12
#define ENC_SCL     13
#define AIN2        14
#define AIN1        15
#define HX711_DT    16
#define HX711_SCK   17

#define INA219_ADDR 0x40
#define PWM_MAX     4096

// Tuning
float Kd_position = 0.0f;
float Kp_current  = 1.7f;
float Ki_current  = 0.11f;

// Wall parameters
#define WALL_LOW    45.0f
#define WALL_ZONE   10.0f
#define WALL_FORCE  500.0f

// Bump parameters
#define BUMP_CENTER  90.0f
#define BUMP_WIDTH   10.0f
#define BUMP_FORCE   350.0f

// Shared state
volatile float desired_current  = 0.0f;
volatile float actual_current   = 0.0f;
volatile float position_deg     = 0.0f;
volatile float last_position    = 0.0f;
volatile float current_integral = 0.0f;
volatile int   pwm_duty         = 0;
static   int32_t hx_zero        = 0;

// Timers
struct repeating_timer current_timer;
struct repeating_timer position_timer;

// INA219 Initialization
void ina219_init() {
    uint8_t buf[3];
    buf[0] = 0x05; buf[1] = 0x04; buf[2] = 0x00;
    i2c_write_blocking(i2c1, INA219_ADDR, buf, 3, false);
    buf[0] = 0x00; buf[1] = 0x39; buf[2] = 0x9F;
    i2c_write_blocking(i2c1, INA219_ADDR, buf, 3, false);
}

float ina219_read_ma() {
    uint8_t reg = 0x04;
    uint8_t buf[2];
    i2c_write_blocking(i2c1, INA219_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c1, INA219_ADDR, buf, 2, false);
    int16_t raw = (buf[0] << 8) | buf[1];
    return raw / 3.0f;
}

// MG996r Motor
void pwm_init_pin(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(pin);
    pwm_set_wrap(slice, PWM_MAX - 1);
    pwm_set_enabled(slice, true);
}

void motor_drive(int duty) {
    uint s1 = pwm_gpio_to_slice_num(AIN1), s2 = pwm_gpio_to_slice_num(AIN2);
    uint c1 = pwm_gpio_to_channel(AIN1),   c2 = pwm_gpio_to_channel(AIN2);
    if (duty > 0) {
        pwm_set_chan_level(s1, c1, duty);
        pwm_set_chan_level(s2, c2, 0);
    } else if (duty < 0) {
        pwm_set_chan_level(s1, c1, 0);
        pwm_set_chan_level(s2, c2, -duty);
    } else {
        pwm_set_chan_level(s1, c1, 0);
        pwm_set_chan_level(s2, c2, 0);
    }
}

// Haptics: 1 wall and bump
float haptic_force(float pos) {
    float force = 0.0f;

    // Low wall (hard stop)
    float dist_low = pos - WALL_LOW;
    if (dist_low < WALL_ZONE) {
        float scale = (WALL_ZONE - dist_low) / WALL_ZONE;
        force += scale * WALL_FORCE;
    }

    // Bump at 90° (passable mound)
    float offset = pos - BUMP_CENTER;
    float bump = BUMP_FORCE * expf(-0.5f * (offset / BUMP_WIDTH) * (offset / BUMP_WIDTH));
    if (offset < 0.0f) force -= bump;
    if (offset > 0.0f) force += bump;

    return force;
}

// 1kHz current control
bool current_control(struct repeating_timer *t) {
    actual_current = ina219_read_ma();
    float error = desired_current - actual_current;
    current_integral += error;
    if (current_integral >  10000.0f) current_integral =  10000.0f;
    if (current_integral < -10000.0f) current_integral = -10000.0f;

    int duty = (int)(Kp_current * error + Ki_current * current_integral);
    if (duty >  PWM_MAX-1) duty =  PWM_MAX-1;
    if (duty < -(PWM_MAX-1)) duty = -(PWM_MAX-1);
    pwm_duty = duty;
    motor_drive(duty);
    return true;
}

//200Hz position control
bool position_control(struct repeating_timer *t) {
    float pos = as5600_read_degrees(i2c0);
    float vel = pos - last_position;
    last_position = pos;
    position_deg  = pos;

    float haptic  = haptic_force(pos);
    float damping = Kd_position * (-vel);

    desired_current = haptic + damping;
    if (desired_current >  500.0f) desired_current =  500.0f;
    if (desired_current < -500.0f) desired_current = -500.0f;
    return true;
}

// Main
int main() {
    stdio_init_all();

    i2c_init(i2c1, 400000);
    gpio_set_function(INA_SDA, GPIO_FUNC_I2C);
    gpio_set_function(INA_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(INA_SDA);
    gpio_pull_up(INA_SCL);
    ina219_init();

    as5600_init(i2c0, ENC_SDA, ENC_SCL);
    hx711_init(HX711_SCK, HX711_DT);

    hx_zero = hx711_read();

    pwm_init_pin(AIN1);
    pwm_init_pin(AIN2);

    add_repeating_timer_ms(-1, current_control,  NULL, &current_timer);
    add_repeating_timer_ms(-5, position_control, NULL, &position_timer);

    while (true) {
        #define HX_AVG_N 8
        static int32_t hx_buf[HX_AVG_N] = {0};
        static int     hx_idx = 0;
        static int64_t hx_sum = 0;

        int32_t hx_new = hx711_read() - hx_zero;
        hx_sum -= hx_buf[hx_idx];
        hx_buf[hx_idx] = hx_new;
        hx_sum += hx_new;
        hx_idx = (hx_idx + 1) % HX_AVG_N;

        int32_t hx_raw = (int32_t)(hx_sum / HX_AVG_N);
        if (hx_raw >  50000) hx_raw =  50000;
        if (hx_raw < -50000) hx_raw = -50000;

        printf("%.2f,%.1f,%.1f,%d,%ld\n",
               position_deg,
               desired_current,
               actual_current,
               pwm_duty,
               hx_raw);
        sleep_ms(20);
    }
}