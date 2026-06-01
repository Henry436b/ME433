#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

#define SPI_PORT    spi0
#define PIN_MISO    16   // not used(DAC --> write-only)
#define PIN_SCK     18
#define PIN_MOSI    19
#define PIN_CS      17

#define DAC_CHAN_A  0
#define DAC_CHAN_B  1

#define SAMPLE_RATE_HZ  200
#define SAMPLE_PERIOD_US (1000000 / SAMPLE_RATE_HZ)

static inline void cs_select(void) {
    asm volatile("nop \n nop \n nop");
    gpio_put(PIN_CS, 0);
    asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect(void) {
    asm volatile("nop \n nop \n nop");
    gpio_put(PIN_CS, 1);
    asm volatile("nop \n nop \n nop");
}

void dac_write(uint8_t channel, uint16_t value) {
    uint16_t cmd = ((channel & 1) << 15) | (0 << 14) | (1 << 13) | (1 << 12) | (value << 2);
    uint8_t buf[2] = { cmd >> 8, cmd & 0xFF };
    cs_select();
    spi_write_blocking(SPI_PORT, buf, 2);
    cs_deselect();
}

int main(void) {
    stdio_init_all();

    spi_init(SPI_PORT, 1000*1000);// 1 MHz (12000 for nLab debugging)
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    float sine_t = 0.0f;
    float triangle_t = 0.0f;
    const float sine_step = 2.0f * (float)M_PI * 2.0f / SAMPLE_RATE_HZ; // 2 Hz sin
    const float triangle_step = 1.0f / SAMPLE_RATE_HZ; // 1 Hz triangle

    while (true) {
        uint16_t sine_val = (uint16_t)((sinf(sine_t) * 0.5f + 0.5f) * 1023.0f);
        dac_write(DAC_CHAN_A, sine_val);

        float tri = triangle_t < 0.5f ? triangle_t * 2.0f : (1.0f - triangle_t) * 2.0f;
        uint16_t tri_val = (uint16_t)(tri * 1023.0f);
        dac_write(DAC_CHAN_B, tri_val);

        sine_t     += sine_step;
        if (sine_t > 2.0f * (float)M_PI) sine_t -= 2.0f * (float)M_PI;
        triangle_t += triangle_step;
        if (triangle_t >= 1.0f) triangle_t -= 1.0f;
        
        sleep_us(SAMPLE_PERIOD_US);
    }
}