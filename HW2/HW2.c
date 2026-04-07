#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

//Using GP14
#define Servo_Pin 14

//Servo min and max "Magic numbers"
#define Servo_min 1950
#define Servo_max 4900

void servo_init(){
    gpio_set_function(Servo_Pin, GPIO_FUNC_PWM);
    
    uint slice = pwm_gpio_to_slice_num(Servo_Pin);
    pwm_set_clkdiv(slice, 64.0f);
    pwm_set_wrap(slice, 39062);
    pwm_set_enabled(slice, true);
}

void servo_angle(uint pin, uint level){
    pwm_set_gpio_level(pin, level);
}


int main()
{
    stdio_init_all();
    servo_init();

    while (true) {
        servo_angle(Servo_Pin, Servo_min);
        sleep_ms(1000);
        servo_angle(Servo_Pin, Servo_max);
        sleep_ms(1000);
    }
}
