#include <SDL2/SDL.h>

#include <cstdio>

#include "lcd/lcd_4bit.hpp"

const int FPS = 30;
const int FRAME_DELAY_MS = 1000 / FPS;

bool timerFlag;
bool wroteFlag;
uint8_t v[4 * 40 * 200];

Uint32 fpsCallback(Uint32 interval, void* param) {
  timerFlag = true;
  return interval;
}

void vsync() {
  if (!wroteFlag) {
    memcpy(gLcd.vram, v, sizeof(gLcd.vram));
    wroteFlag = true;
  }
}

int main(int argc, char** argv) {
  FILE* fp;
  fp = fopen("mov.bin", "rb");

  LcdInit(vsync);
  SDL_TimerID timerID = SDL_AddTimer(FRAME_DELAY_MS, fpsCallback, NULL);
  while (LcdProcess()) {
    if (timerFlag && wroteFlag) {
      int r = fread(v, sizeof(v[0]), sizeof(v) / sizeof(v[0]), fp);

      if (r == 0) {
        fseek(fp, SEEK_SET, 0);
        r = fread(v, sizeof(v[0]), sizeof(v) / sizeof(v[0]), fp);
      }

      timerFlag = wroteFlag = false;
    }
  }
  SDL_RemoveTimer(timerID);
  fclose(fp);
  return 0;
}