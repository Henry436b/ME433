#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

#define SPI_PORT     spi0
#define PIN_MISO     16
#define PIN_SCK      18
#define PIN_MOSI     19
#define PIN_CS_DAC   17
#define PIN_CS_RAM   20

#define NUM_SAMPLES  1000

// --- CS helpers ---
static inline void cs_select(uint pin)   { asm volatile("nop \n nop \n nop"); gpio_put(pin, 0); asm volatile("nop \n nop \n nop"); }
static inline void cs_deselect(uint pin) { asm volatile("nop \n nop \n nop"); gpio_put(pin, 1); asm volatile("nop \n nop \n nop"); }

// --- DAC ---
void dac_write(uint8_t channel, uint16_t value) {
    uint16_t cmd = ((channel & 1) << 15) | (1 << 13) | (1 << 12) | (value << 2);
    uint8_t buf[2] = { cmd >> 8, cmd & 0xFF };
    cs_select(PIN_CS_DAC);
    spi_write_blocking(SPI_PORT, buf, 2);
    cs_deselect(PIN_CS_DAC);
}

// --- SRAM ---
void sram_init(void) {
    // set sequential mode
    uint8_t cmd[2] = { 0x01, 0x40 };
    cs_select(PIN_CS_RAM);
    spi_write_blocking(SPI_PORT, cmd, 2);
    cs_deselect(PIN_CS_RAM);
}

void sram_write(uint16_t addr, uint8_t *data, uint16_t len) {
    uint8_t header[3] = { 0x02, addr >> 8, addr & 0xFF };
    cs_select(PIN_CS_RAM);
    spi_write_blocking(SPI_PORT, header, 3);
    spi_write_blocking(SPI_PORT, data, len);
    cs_deselect(PIN_CS_RAM);
}

void sram_read(uint16_t addr, uint8_t *buf, uint16_t len) {
    uint8_t header[3] = { 0x03, addr >> 8, addr & 0xFF };
    cs_select(PIN_CS_RAM);
    spi_write_blocking(SPI_PORT, header, 3);
    spi_read_blocking(SPI_PORT, 0x00, buf, len);
    cs_deselect(PIN_CS_RAM);
}

int main(void) {
    stdio_init_all();
    sleep_ms(3000);  // wait for serial monitor to connect

    spi_init(SPI_PORT, 1000 * 1000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    gpio_init(PIN_CS_DAC); gpio_set_dir(PIN_CS_DAC, GPIO_OUT); gpio_put(PIN_CS_DAC, 1);
    gpio_init(PIN_CS_RAM); gpio_set_dir(PIN_CS_RAM, GPIO_OUT); gpio_put(PIN_CS_RAM, 1);

    sram_init();

    uint8_t test_write[2] = { 0xAB, 0xCD };
    uint8_t test_read[2]  = { 0x00, 0x00 };
    sram_write(0, test_write, 2);
    sram_read(0, test_read, 2);
    printf("SRAM test: %02X %02X\n", test_read[0], test_read[1]);

    // compute 1000 sine samples, convert to DAC 16-bit commands, store in SRAM
    for (int i = 0; i < NUM_SAMPLES; i++) {
        float v = (sinf(2.0f * (float)M_PI * i / NUM_SAMPLES) * 0.5f + 0.5f) * 1023.0f;
        uint16_t val = (uint16_t)v;
        uint16_t cmd = (1 << 13) | (1 << 12) | (val << 2);  // channel A
        uint8_t buf[2] = { cmd >> 8, cmd & 0xFF };
        sram_write(i * 2, buf, 2);
    }

    // read back 2 bytes at a time and send to DAC at 1ms intervals → 1Hz sine
    uint16_t addr = 0;
    
    while (true) {
        uint8_t buf[2];
        sram_read(addr, buf, 2);
        cs_select(PIN_CS_DAC);
        spi_write_blocking(SPI_PORT, buf, 2);
        cs_deselect(PIN_CS_DAC);

        addr += 2;
        if (addr >= NUM_SAMPLES * 2) addr = 0;

        sleep_ms(1);
    }
}