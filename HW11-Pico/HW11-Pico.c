#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

int main()
{
    stdio_init_all();

    uart_init(uart0, 115200);
    gpio_set_function(0, GPIO_FUNC_UART);
    gpio_set_function(1, GPIO_FUNC_UART);

    while (true)
    {
        if (uart_is_readable(uart0)){
            putchar(uart_getc(uart0));
        }
    }
}