#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "bsp/board_api.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "mpu6050.h"

#define LED_PIN   18
#define BTN_PIN   17
#define SDA_PIN   12
#define SCL_PIN   13

enum {
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED     = 1000,
  BLINK_SUSPENDED   = 2500,
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;
static bool circle_mode = false;
static float circle_angle = 0.0f;

void led_blinking_task(void);
void hid_task(void);

int main(void) {
  board_init();

  // mode LED
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);
  gpio_put(LED_PIN, 0);

  // toggle button
  gpio_init(BTN_PIN);
  gpio_set_dir(BTN_PIN, GPIO_IN);
  gpio_pull_up(BTN_PIN);

  // I2C + IMU
  i2c_init(i2c0, 400 * 1000);
  gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
  gpio_pull_up(SDA_PIN);
  gpio_pull_up(SCL_PIN);

  if (mpu6050_check(i2c0) != 0) {
    gpio_put(LED_PIN, 1);
    while (1) tight_loop_contents();
  }
  mpu6050_init(i2c0);

  tud_init(BOARD_TUD_RHPORT);
  if (board_init_after_tusb) board_init_after_tusb();

  while (1) {
    tud_task();
    led_blinking_task();
    hid_task();
  }
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+
void tud_mount_cb(void)    { blink_interval_ms = BLINK_MOUNTED; }
void tud_umount_cb(void)   { blink_interval_ms = BLINK_NOT_MOUNTED; }
void tud_suspend_cb(bool remote_wakeup_en) { (void)remote_wakeup_en; blink_interval_ms = BLINK_SUSPENDED; }
void tud_resume_cb(void)   { blink_interval_ms = tud_mounted() ? BLINK_MOUNTED : BLINK_NOT_MOUNTED; }

//--------------------------------------------------------------------+
// HID
//--------------------------------------------------------------------+
static int8_t accel_to_delta(float a) {
  float mag = fabsf(a);
  int8_t speed;
  if      (mag < 0.1f) speed = 0;
  else if (mag < 0.3f) speed = 1;
  else if (mag < 0.6f) speed = 3;
  else                 speed = 5;
  return (a < 0) ? -speed : speed;
}

static void button_task(void) {
  static bool last = false;
  bool pressed = !gpio_get(BTN_PIN);
  if (pressed && !last) {
    circle_mode = !circle_mode;
    gpio_put(LED_PIN, circle_mode);
  }
  last = pressed;
}

static void send_hid_report(uint8_t report_id, uint32_t btn) {
  if (!tud_hid_ready()) return;

  switch (report_id) {
    case REPORT_ID_MOUSE: {
      int8_t dx, dy;
      if (circle_mode) {
        dx = (int8_t)(3.0f * cosf(circle_angle));
        dy = (int8_t)(3.0f * sinf(circle_angle));
        circle_angle += 0.05f;
        if (circle_angle > 2.0f * (float)M_PI) circle_angle -= 2.0f * (float)M_PI;
      } else {
        imu_data_t d;
        mpu6050_read(i2c0, &d);
        dx = -accel_to_delta(d.ax);
        dy = accel_to_delta(d.ay);
      }
      tud_hid_mouse_report(REPORT_ID_MOUSE, 0x00, dx, dy, 0, 0);
    } break;

    // keep remaining cases so composite report chain still works
    case REPORT_ID_KEYBOARD: {
      static bool has_key = false;
      if (btn) {
        uint8_t keycode[6] = { HID_KEY_A };
        tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keycode);
        has_key = true;
      } else {
        if (has_key) tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
        has_key = false;
      }
    } break;

    case REPORT_ID_CONSUMER_CONTROL: {
      static bool has_key = false;
      if (btn) {
        uint16_t vol = HID_USAGE_CONSUMER_VOLUME_DECREMENT;
        tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &vol, 2);
        has_key = true;
      } else {
        uint16_t empty = 0;
        if (has_key) tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &empty, 2);
        has_key = false;
      }
    } break;

    case REPORT_ID_GAMEPAD: {
      static bool has_key = false;
      hid_gamepad_report_t report = { .x=0,.y=0,.z=0,.rz=0,.rx=0,.ry=0,.hat=0,.buttons=0 };
      if (btn) {
        report.hat = GAMEPAD_HAT_UP; report.buttons = GAMEPAD_BUTTON_A;
        tud_hid_report(REPORT_ID_GAMEPAD, &report, sizeof(report));
        has_key = true;
      } else {
        report.hat = GAMEPAD_HAT_CENTERED;
        if (has_key) tud_hid_report(REPORT_ID_GAMEPAD, &report, sizeof(report));
        has_key = false;
      }
    } break;

    default: break;
  }
}

void hid_task(void) {
  const uint32_t interval_ms = 10;
  static uint32_t start_ms = 0;
  if (board_millis() - start_ms < interval_ms) return;
  start_ms += interval_ms;

  button_task();

  uint32_t const btn = board_button_read();
  if (tud_suspended() && btn) {
    tud_remote_wakeup();
  } else {
    send_hid_report(REPORT_ID_MOUSE, btn);
  }
}

void tud_hid_report_complete_cb(uint8_t instance, uint8_t const *report, uint16_t len) {
  (void)instance; (void)len;
  uint8_t next = report[0] + 1u;
  if (next < REPORT_ID_COUNT) send_hid_report(next, board_button_read());
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) {
  (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)reqlen;
  return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {
  (void)instance;
  if (report_type == HID_REPORT_TYPE_OUTPUT && report_id == REPORT_ID_KEYBOARD) {
    if (bufsize < 1) return;
    uint8_t kbd_leds = buffer[0];
    if (kbd_leds & KEYBOARD_LED_CAPSLOCK) {
      blink_interval_ms = 0;
      board_led_write(true);
    } else {
      board_led_write(false);
      blink_interval_ms = BLINK_MOUNTED;
    }
  }
}

//--------------------------------------------------------------------+
// BLINKING TASK (onboard LED, untouched)
//--------------------------------------------------------------------+
void led_blinking_task(void) {
  static uint32_t start_ms = 0;
  static bool led_state = false;
  if (!blink_interval_ms) return;
  if (board_millis() - start_ms < blink_interval_ms) return;
  start_ms += blink_interval_ms;
  board_led_write(led_state);
  led_state = 1 - led_state;
}