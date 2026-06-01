#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

#define SPI_PORT    spi0
#define PIN_MISO    16   // not used (DAC is write-only) but must be configured
#define PIN_SCK     18
#define PIN_MOSI    19
#define PIN_CS      17

#define DAC_CHAN_A  0
#define DAC_CHAN_B  1

// update rate: 100 Hz per channel is well above 50x the 2Hz/1Hz signals
#define SAMPLE_RATE_HZ  200   // ticks per second (both channels updated each tick)
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

// channel: 0=A, 1=B   value: 0-1023 (10-bit)
void dac_write(uint8_t channel, uint16_t value) {
    value &= 0x3FF;  // clamp to 10 bits
    // bit15=channel, bit14=0(unbuf), bit13=1(gain 1x), bit12=1(active), bits11-2=data
    uint16_t cmd = ((channel & 1) << 15) | (0 << 14) | (1 << 13) | (1 << 12) | (value << 2);
    uint8_t buf[2] = { cmd >> 8, cmd & 0xFF };
    cs_select();
    spi_write_blocking(SPI_PORT, buf, 2);
    cs_deselect();
}

int main(void) {
    stdio_init_all();

    spi_init(SPI_PORT, 1000 * 1000);  // 1 MHz (set to 12000 for nLab debugging)
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    float sine_t    = 0.0f;
    float triangle_t = 0.0f;
    const float sine_step     = 2.0f * (float)M_PI * 2.0f / SAMPLE_RATE_HZ;  // 2 Hz
    const float triangle_step = 1.0f / SAMPLE_RATE_HZ;                         // 1 Hz, period=1s

    while (1) {
        // 2Hz sine on channel A: 0 to 1023
        uint16_t sine_val = (uint16_t)((sinf(sine_t) * 0.5f + 0.5f) * 1023.0f);
        dac_write(DAC_CHAN_A, sine_val);

        // 1Hz triangle on channel B: 0 to 1023
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