#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "lcd/fontx.hpp"

#ifndef __arm__
#include <SDL2/SDL.h>

#include <vector>
#else
#include <hardware/dma.h>
#include <hardware/pio.h>
#include <pico/stdlib.h>

#include "lcd_4bit.pio.h"
#endif

/* 何諧調表示にするか？ */
#define LCD_LAYER_COUNT 4

/* LCDの解像度定義 */
#define LCD_WIDTH 320
#define LCD_HEIGHT 200

/* シミュレータの表示倍率 */
#define LCD_SCALE 4

struct tLcd {
  uint8_t layer;
  uint8_t vram[LCD_WIDTH * LCD_HEIGHT / 8 * LCD_LAYER_COUNT];
  void (*vsync)();
#ifndef __arm__
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_Texture* texture;
#else
  PIO pio;
  uint sm;
  uint dma;
  bool mState;
  uint8_t line;
#endif
};

int LcdInit(void (*vsync)());
int LcdProcess(void);

extern struct tLcd gLcd;
