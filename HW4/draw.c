#include "draw.h"
#include "font.h"
#include "ssd1306.h"

void drawChar(int x, int y, char ch) {
    int idx = ch - 0x20;  // font table starts at space (0x20)
    for (int col = 0; col < 5; col++) {
        unsigned char column_bits = ASCII[idx][col];
        for (int row = 0; row < 8; row++) {
            ssd1306_drawPixel(x + col, y + row, (column_bits >> row) & 1);
        }
    }
}


void drawMessage(int x, int y, char* str) {
    int i = 0;
    while (str[i] != '\0') {
        drawChar(x + i * 6, y, str[i]);
        i++;
    }
}