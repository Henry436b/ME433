#pragma once
#include "pico/stdlib.h"

void hx711_init(uint sck_pin, uint dt_pin);
int32_t hx711_read(void);