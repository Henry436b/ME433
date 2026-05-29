#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"

#define LED_PIN 16
#define I2C_SDA 12
#define I2C_SCL 13


int main(){
    stdio_init_all();

    i2c_init(i2c_default, 400*1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    //LED Set up
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    ssd1306_setup();

    int pixel_on = 0;

    while (true) {
        gpio_put(LED_PIN, pixel_on);

        ssd1306_clear();
        if(pixel_on){
            ssd1306_drawPixel(64, 16, 1);
        }
        ssd1306_update();

        pixel_on = !pixel_on;
        sleep_ms(500);
    }
}
