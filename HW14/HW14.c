#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hx711.h"

#define SCK_PIN 17
#define DT_PIN  16

int main() {
    stdio_init_all();
    hx711_init(SCK_PIN, DT_PIN);

    int n = 0;
    while (n <= 0) {
        scanf("%d", &n);
    }

    int32_t  *raw_buf      = malloc(n * sizeof(int32_t));
    int32_t  *filtered_buf = malloc(n * sizeof(int32_t));
    uint32_t *time_buf     = malloc(n * sizeof(uint32_t));

    float alpha = 0.1f;
    float filtered = 0.0f;
    bool first = true;

    for (int i = 0; i < n; i++) {
        int32_t sample = hx711_read();
        uint32_t t = to_ms_since_boot(get_absolute_time());

        if (first) {
            filtered = (float)sample;
            first = false;
        } else {
            filtered = alpha * sample + (1.0f - alpha) * filtered;
        }

        raw_buf[i]      = sample;
        filtered_buf[i] = (int32_t)filtered;
        time_buf[i]     = t;
    }

    for (int i = 0; i < n; i++) {
        printf("%lu %ld %ld\n", (unsigned long)time_buf[i], raw_buf[i], filtered_buf[i]);
    }

        free(raw_buf);
        free(filtered_buf);
        free(time_buf);

    return 0;
}