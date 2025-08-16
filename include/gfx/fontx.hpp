#pragma once

#include <cstdint>

#pragma once

#include <cstdint>

/* Import a binary file */
#define IMPORT_BIN(sect, file, sym)                        \
  asm(".section " #sect                                    \
      "\n"                 /* Change section */            \
      ".balign 4\n"        /* Word alignment */            \
      ".global " #sym "\n" /* Export the object address */ \
      #sym                                                 \
      ":\n" /* Define the object label */                  \
      ".incbin \"" file                                    \
      "\"\n" /* Import the file */                         \
      ".global _sizeof_" #sym                              \
      "\n" /* Export the object size */                    \
      ".set _sizeof_" #sym ", . - " #sym                   \
      "\n"                    /* Define the object size */ \
      ".balign 4\n"           /* Word alignment */         \
      ".section \".text\"\n") /* Restore section */

/// @brief Shift-JISで2バイト目が来るかどうか判別
/// @param c 判定する文字
/// @return 1:2バイト文字/0:1バイト文字
inline int SJISMultiCheck(unsigned char c) {
  if (((c >= 0x81) && (c <= 0x9f)) || ((c >= 0xe0) && (c <= 0xfc)))
    return 1;
  else
    return 0;
}

enum eFont {
  eFont_VLG, /* VLゴシック(16) */
  eFont_HSP, /* 創英プレゼンス(32) */
};

uint32_t KanjiReadX(eFont font, uint16_t code, uint8_t *pat);