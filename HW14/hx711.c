#include "hx711.h"

static uint _sck;
static uint _dt;

void hx711_init(uint sck_pin, uint dt_pin) {
    _sck = sck_pin;
    _dt  = dt_pin;

    gpio_init(_sck);
    gpio_set_dir(_sck, GPIO_OUT);
    gpio_put(_sck, 0);

    gpio_init(_dt);
    gpio_set_dir(_dt, GPIO_IN);
}

int32_t hx711_read(void) {
    while (gpio_get(_dt) == 1) tight_loop_contents();

    uint32_t raw = 0;

    // 24 clock pulses, read DT
    for (int i = 0; i < 24; i++) {
        gpio_put(_sck, 1);
        sleep_us(1);
        raw = (raw << 1) | gpio_get(_dt);
        gpio_put(_sck, 0);
        sleep_us(1);
    }

    // 25th pulse
    gpio_put(_sck, 1);
    sleep_us(1);
    gpio_put(_sck, 0);
    sleep_us(1);
    
    if (raw & 0x800000) {
        raw |= 0xFF000000;
    }

    return (int32_t)raw;
}