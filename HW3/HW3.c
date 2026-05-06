#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// I2C Init
#define I2C_PORT i2c0
#define SDA_PIN 12
#define SCL_PIN 13

// MCP23008 address with A0,A1,A2 tied to GND
#define MCP23008_ADDR 0x20

// MCP23008 Registers
#define IODIR 0x00
#define GPIO  0x09
#define OLAT  0x0A

// HEARTBEAT LED GPIO num
#define HEARTBEAT_LED 16

//Pre call funcs
void writeRegister(uint8_t reg, uint8_t value);
uint8_t readRegister(uint8_t reg);

int main()
{
    stdio_init_all();

    // Init "Heartbeat" LED
    gpio_init(HEARTBEAT_LED);
    gpio_set_dir(HEARTBEAT_LED, GPIO_OUT);

    // Initialize I2C
    i2c_init(I2C_PORT, 100 * 1000);

    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);

    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    //GP7 output, others input
    writeRegister(IODIR, 0x7F);

    //Start LED OFF
    writeRegister(OLAT, 0x00);

    while (true){
        // HEARTBEAT BLINK
        gpio_put(HEARTBEAT_LED, 1);
        sleep_ms(200);

        gpio_put(HEARTBEAT_LED,0);
        sleep_ms(100);

        //Button state
        uint8_t gpioState = readRegister(GPIO);

        // Check GP0
        bool buttonPressed = !(gpioState & 0x01);
        // If else button
        if (buttonPressed){
            writeRegister(OLAT, 0x80); // Turn GP7 ON
        }
        else{
            writeRegister(OLAT, 0x00); // Turn GP7 OFF

        }
        sleep_ms(100);
    }
}

// General Purpose Write Function
void writeRegister(uint8_t reg, uint8_t value){
    uint8_t buf[2];

    buf[0] = reg;
    buf[1] = value;

    i2c_write_blocking(I2C_PORT, MCP23008_ADDR, buf, 2, false);
}
// General Purpose Read Function
uint8_t readRegister(uint8_t reg){
    uint8_t value;

    // T    ell MCP23008 which register we want
    i2c_write_blocking(I2C_PORT, MCP23008_ADDR, &reg, 1, true);

    // Read the register value
    i2c_read_blocking(I2C_PORT, MCP23008_ADDR, &value, 1, false);
    return value;
}