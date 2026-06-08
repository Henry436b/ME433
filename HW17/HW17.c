#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "as5600.h"

#define SDA_PIN 4
#define SCL_PIN 5

int main() {
    stdio_init_all();
    as5600_init(i2c0, SDA_PIN, SCL_PIN);

    while (1) {
        float angle = as5600_read_degrees(i2c0);
        uint32_t t = to_ms_since_boot(get_absolute_time());
        printf("%lu,%.2f\n", (unsigned long)t, angle);
        sleep_ms(12);  // ~80Hz
    }
}