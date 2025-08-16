#include "gfx/fontx.hpp"

/* フォントファイル名 */
#define FONT_HANKAKU "VLG16H.FNT"
#define FONT_ZENKAKU "VLG16Z.FNT"

/* フォントをROMに埋め込むための魔法 */
IMPORT_BIN(".rodata", "VLG16H.FNT", FontVlgH);
IMPORT_BIN(".rodata", "VLG16Z.FNT", FontVlgZ);

IMPORT_BIN(".rodata", "HSP32H.FNT", FontHspH);
IMPORT_BIN(".rodata", "HSP32Z.FNT", FontHspZ);

extern const uint8_t FontVlgH[], _sizeof_FontVlgH[];
extern const uint8_t FontVlgZ[], _sizeof_FontVlgZ[];
extern const uint8_t FontHspH[], _sizeof_FontHspH[];
extern const uint8_t FontHspZ[], _sizeof_FontHspZ[];

/// @brief Flashに格納されているフォントから指定された文字のパターンを取得
/// @param font フォントの種類
/// @param code パターンを取得したい文字コード(Shift-JIS)
/// @param pat パターンの格納先
/// @return 諸々の情報(あまり使わない)
uint32_t KanjiReadX(eFont font, uint16_t code, uint8_t *pat) {
  uint32_t ccc = 0, ofset = 0;
  uint8_t *fontx_data;
  uint32_t index = 0;
  if (code < 0x100) {
    /* 半角 */
    switch (font) {
      case eFont_HSP:
        fontx_data = (uint8_t *)FontHspH;
        break;
      case eFont_VLG:
      default:
        fontx_data = (uint8_t *)FontVlgH;
    }
  } else {
    /* 全角 */
    switch (font) {
      case eFont_HSP:
        fontx_data = (uint8_t *)FontHspZ;
        break;
      case eFont_VLG:
      default:
        fontx_data = (uint8_t *)FontVlgZ;
    }
  }
  index = 14;
  uint8_t Fw = fontx_data[index++];
  uint8_t Fh = fontx_data[index++];
  uint8_t Ft = fontx_data[index++];
  uint8_t Fb, Fbw;
  uint32_t Fsize = ((Fw + 7) / 8) * Fh;
  if (Ft == 0x00) {
    if (code < 0x100) ofset = 17 + code * Fsize;
  } else {
    Fb = Fbw = fontx_data[index++];
    while (Fbw--) {
      uint16_t Cbs = fontx_data[index++] | fontx_data[index++] << 8,
               Cbe = fontx_data[index++] | fontx_data[index++] << 8;
      if (code >= Cbs && code <= Cbe) {
        ccc += code - Cbs;
        ofset = 18 + 4 * Fb + ccc * Fsize;
        break;
      }
      ccc += Cbe - Cbs + 1;
    }
  }
  uint16_t i = Fsize;
  if (ofset) {
    index = ofset;
    while (i--) *(pat++) = fontx_data[index++];
    return ((unsigned long)Fw << 24) | ((unsigned long)Fh << 16) | Fsize;
  }
  return 0;
}