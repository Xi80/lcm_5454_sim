#include "lcd/lcd_4bit.hpp"

struct tLcd gLcd;

#ifndef __arm__
const int VRAM_PLANE_SIZE_BYTES = LCD_WIDTH * LCD_HEIGHT / 8;
const int VRAM_TOTAL_SIZE_BYTES = VRAM_PLANE_SIZE_BYTES * 4;
const int FPS = 30;
const int FRAME_DELAY_MS = 1000 / FPS;
std::vector<uint32_t> pixels(LCD_WIDTH* LCD_HEIGHT);
static void convertVramToPixels(const uint8_t* vram,
                                std::vector<uint32_t>& pixels);
#else
const int VRAM_LINE_STRIDE = LCD_WIDTH / 8;

#define _LCD_XSCL 25
#define _LCD_DATA 26
#define _LCD_VSYNC 22
#define _LCD_HSYNC 24
#define _LCD_M 23

#define WAIT(x)                   \
  for (int z = 0; z < (x); z++) { \
    __asm__ volatile("nop");      \
  }

static void dmaHandler();
#endif

int LcdInit(void (*vsync)()) {
  gLcd.vsync = vsync;
  memset(gLcd.vram, 0x00, sizeof(gLcd.vram));
#ifndef __arm__
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    return -1;
  }

  gLcd.window = SDL_CreateWindow("LCM-5454 Simulator", SDL_WINDOWPOS_CENTERED,
                                 SDL_WINDOWPOS_CENTERED, LCD_WIDTH * LCD_SCALE,
                                 LCD_HEIGHT * LCD_SCALE, SDL_WINDOW_SHOWN);
  if (!gLcd.window) {
    SDL_Quit();
    return -2;
  }

  gLcd.renderer = SDL_CreateRenderer(gLcd.window, -1, SDL_RENDERER_ACCELERATED);

  if (!gLcd.renderer) {
    SDL_DestroyWindow(gLcd.window);
    SDL_Quit();
    return -3;
  }

  gLcd.texture =
      SDL_CreateTexture(gLcd.renderer, SDL_PIXELFORMAT_ARGB8888,
                        SDL_TEXTUREACCESS_STREAMING, LCD_WIDTH, LCD_HEIGHT);
  if (!gLcd.texture) {
    SDL_DestroyRenderer(gLcd.renderer);
    SDL_DestroyWindow(gLcd.window);
    SDL_Quit();
    return -4;
  }

  return 0;
#else
  /*同期信号*/
  gpio_init(_LCD_VSYNC);
  gpio_init(_LCD_HSYNC);
  gpio_init(_LCD_M);

  gpio_set_dir(_LCD_VSYNC, true);
  gpio_set_dir(_LCD_HSYNC, true);
  gpio_set_dir(_LCD_M, true);

  /*PIO初期化*/
  gLcd.pio = pio0;
  gLcd.sm = 0;
  uint offset = pio_add_program(gLcd.pio, &eg8504_lcd_program);
  eg8504_lcd_program_init(gLcd.pio, gLcd.sm, offset, _LCD_DATA, _LCD_XSCL,
                          15.f);
  gLcd.dma = dma_claim_unused_channel(true);

  /*DMA初期化*/
  dma_channel_config config = dma_channel_get_default_config(gLcd.dma);
  channel_config_set_transfer_data_size(&config, DMA_SIZE_8);
  channel_config_set_read_increment(&config, true);
  channel_config_set_write_increment(&config, false);
  channel_config_set_dreq(&config,
                          pio_get_dreq(gLcd.pio, gLcd.sm, true));  // pio sm, TX
  dma_channel_configure(gLcd.dma, &config, &pio0_hw->txf[gLcd.sm],
                        &gLcd.vram[0], 40, false);
  dma_channel_set_irq1_enabled(gLcd.dma, true);
  irq_set_exclusive_handler(DMA_IRQ_1, dmaHandler);
  irq_set_enabled(DMA_IRQ_1, true);

  dmaHandler();
  return 0;
#endif
}

int LcdProcess(void) {
  if (gLcd.vsync != nullptr) gLcd.vsync();
#ifndef __arm__
  int ret = 1;
  Uint32 frameStart = SDL_GetTicks();
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      ret = 0;
      SDL_DestroyTexture(gLcd.texture);
      SDL_DestroyRenderer(gLcd.renderer);
      SDL_DestroyWindow(gLcd.window);
      SDL_Quit();
    }
  }

  /* VRAM -> Texture */
  convertVramToPixels(gLcd.vram, pixels);

  // テクスチャを更新して画面に描画
  SDL_UpdateTexture(gLcd.texture, NULL, pixels.data(),
                    LCD_WIDTH * sizeof(uint32_t));
  SDL_RenderClear(gLcd.renderer);
  SDL_RenderCopy(gLcd.renderer, gLcd.texture, NULL, NULL);
  SDL_RenderPresent(gLcd.renderer);
  // 3. 処理にかかった時間を計算
  int frameTime = SDL_GetTicks() - frameStart;

  // 4. 目標時間より処理が早く終わったら、その差分だけ待機
  if (frameTime < FRAME_DELAY_MS) {
    SDL_Delay(FRAME_DELAY_MS - frameTime);
  }
  return ret;
#else
  return 1;
#endif
}

#ifndef __arm__

static void convertVramToPixels(const uint8_t* vram,
                                std::vector<uint32_t>& pixels) {
  for (int y = 0; y < LCD_HEIGHT; ++y) {
    for (int x = 0; x < LCD_WIDTH; ++x) {
      int pixel_index = y * LCD_WIDTH + x;
      int byte_index = pixel_index / 8;
      uint8_t bit_mask = 1 << (7 - (pixel_index % 8));
      uint8_t sum = 0;
      for (int p = 0; p < 4; ++p) {
        int plane_offset = p * VRAM_PLANE_SIZE_BYTES;
        uint8_t byte_data = vram[plane_offset + byte_index];
        if (byte_data & bit_mask) {
          sum++;
        }
      }
      uint8_t gray_value = sum * 63;
      uint32_t color =
          (0xFF << 24) | (gray_value << 16) | (gray_value << 8) | gray_value;
      pixels[pixel_index] = color;
    }
  }
}

#else

static void dmaHandler() {
  while (!pio_sm_is_tx_fifo_empty(gLcd.pio, gLcd.sm));

  WAIT(48);

  /*HSYNC*/
  gpio_put(_LCD_HSYNC, true);

  gLcd.line = (gLcd.line + 1) % LCD_HEIGHT;
  if (gLcd.line == 0) {
    if (gLcd.vsync != nullptr) gLcd.vsync();

#if LCD_LAYER_COUNT > 1
    gLcd.layer = (gLcd.layer + 1) % 4;
#endif
    gLcd.mState = !gLcd.mState;
    gpio_put(_LCD_M, gLcd.mState);
  }
  gpio_put(_LCD_HSYNC, false);

  WAIT(32);

  gpio_put(_LCD_VSYNC, (gLcd.line == 0) ? true : false);

  WAIT(16);

  dma_channel_set_read_addr(
      gLcd.dma,
      &gLcd.vram[VRAM_LINE_STRIDE * LCD_HEIGHT * gLcd.layer +
                 gLcd.line * VRAM_LINE_STRIDE],
      true);

  /*IRQをクリア*/
  dma_hw->ints1 = 1u << gLcd.dma;
}

#endif