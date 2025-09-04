#include <SDL2/SDL.h>

#include <cstdio>

#include "gfx/fontx.hpp"
#include "lcd/lcd_4bit.hpp"
void vsync() {}

int main(int argc, char** argv) {
  const uint16_t text[] = {
      0x93fa, 0x967b, 0x8cea, 0x955c, 0x8ea6, 0x8365, 0x8358, 0x8367,
  };

  LcdInit(vsync);

  for (int l = 0; l < LCD_LAYER_COUNT; l++) {
    uint8_t* v = &gLcd.vram[l * 40 * 200];
    uint8_t buf[2 * 16];
    for (int c = 0; c < sizeof(text) / sizeof(text[0]); c++) {
      KanjiReadX(eFont_VLG, text[c], buf);
      for (int y = 0; y < 16; y++) {
        memcpy(&v[40 * y + 2 * c], &buf[2 * y], 2);
      }
    }
  }

  for (int l = 0; l < LCD_LAYER_COUNT; l++) {
    uint8_t* v = &gLcd.vram[l * 40 * 200];
    uint8_t buf[4 * 32];
    for (int c = 0; c < sizeof(text) / sizeof(text[0]); c++) {
      KanjiReadX(eFont_HSP, text[c], buf);
      for (int y = 0; y < 32; y++) {
        memcpy(&v[40 * (y + 16) + 4 * c], &buf[4 * y], 4);
      }
    }
  }
  while (LcdProcess()) {
  }
  return 0;
}