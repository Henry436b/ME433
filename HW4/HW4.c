#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "ssd1306.h"
#include "draw.h"

#define LED_PIN 16
#define I2C_SDA 12
#define I2C_SCL 13

int main(){
    stdio_init_all();

    i2c_init(i2c0, 400*1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    adc_init();
    adc_gpio_init(26);
    adc_select_input(0);

    ssd1306_setup();

    int led = 0;
    unsigned int t = to_us_since_boot(get_absolute_time());  

    while (true) {
        uint16_t raw = adc_read();
        float volts = raw * 3.3f / 4095.0f;

        unsigned int t2 = to_us_since_boot(get_absolute_time());
        float fps = 1000000.0f / (t2 - t);
        t = t2;

        if (led == 0){
            led = 1;
        }
        else{
            led = 0;
        }
        gpio_put(LED_PIN, led);
        char adc_msg[50];
        char fps_msg[50];
        sprintf(adc_msg, "ADC0: %.2f V", volts);
        sprintf(fps_msg, "FPS: %.1f", fps);

        ssd1306_clear();
        drawMessage(0, 0, adc_msg);
        drawMessage(0, 24, fps_msg);
        ssd1306_update();
    }
}