#ifdef MUZI_BASE_SUPERIO
#include "EmojiText.h"
#include <string.h>
#include <limits.h>

namespace superbase { namespace emoji {
#include "emoji/OpenMojiData.inc"

static uint32_t decode(const char* text, size_t& size) {
  const uint8_t first = uint8_t(text[0]);
  size = first ? 1 : 0;
  if (first < 0x80) return first;
  const unsigned count = first >= 0xF0 && first <= 0xF4 ? 4 :
    first >= 0xE0 && first <= 0xEF ? 3 : first >= 0xC2 && first <= 0xDF ? 2 : 0;
  if (!count) return 0xFFFD;
  uint32_t cp = first & (0x7F >> count);
  for (unsigned i = 1; i < count; ++i) {
    const uint8_t value = uint8_t(text[i]);
    if (!value || (value & 0xC0) != 0x80) return 0xFFFD;
    cp = (cp << 6) | (value & 0x3F);
  }
  if ((count == 2 && cp < 0x80) || (count == 3 && cp < 0x800) ||
      (count == 4 && cp < 0x10000) || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) return 0xFFFD;
  size = count;
  return cp;
}

Token next(const char* text) {
  if (!*text) return {0, -1, 0};
  size_t first_size;
  const uint32_t first_cp = decode(text, first_size);
  if (first_cp == 0xFE0E || first_cp == 0xFE0F || first_cp == 0x200D)
    return {first_size, -1, 0};
  uint16_t index = 0;
  const char* cursor = text;
  Token best = {0, -1, 0};
  while (*cursor) {
    size_t size;
    uint32_t cp = decode(cursor, size);
    if (cp == 0xFE0E || cp == 0xFE0F) {
      cursor += size;
      if (best.bytes == size_t(cursor-text)-size) best.bytes += size;
      continue;
    }
    const Node& parent = nodes[index];
    unsigned low = parent.first, high = parent.first + parent.count;
    while (low < high) {
      const unsigned middle = low + (high-low)/2;
      if (nodes[middle].cp < cp) low = middle + 1; else high = middle;
    }
    if (low == parent.first + parent.count || nodes[low].cp != cp) break;
    index = low;
    cursor += size;
    if (nodes[index].glyph >= 0) best = {size_t(cursor-text), nodes[index].glyph, 0};
  }
  if (best.glyph >= 0) return best;
  uint8_t character = first_cp < 128 ? first_cp : '?';
  // The existing CP437 text font can also represent common Spanish letters.
  static const uint16_t latin[] = {0x00E1,0x00E9,0x00ED,0x00F3,0x00FA,0x00F1,0x00D1,0x00FC,0x00DC,0x00BF,0x00A1};
  static const uint8_t cp437[] = {160,130,161,162,163,164,165,129,154,168,173};
  for (unsigned i = 0; i < sizeof(latin)/sizeof(latin[0]); ++i)
    if (first_cp == latin[i]) character = cp437[i];
  if (character < 32 && character != '\r' && character != '\n') character = 0;
  return {first_size, -1, character};
}

void copyUtf8(char* dest, const char* src, size_t capacity) {
  if (!capacity) return;
  size_t used = 0;
  while (*src) {
    const Token token = next(src);
    if (token.bytes > capacity - 1 - used) break;
    memcpy(dest + used, src, token.bytes);
    used += token.bytes; src += token.bytes;
  }
  dest[used] = 0;
}

uint16_t width(const char* text, uint8_t scale) {
  unsigned line = 0, maximum = 0;
  while (*text) {
    const Token token = next(text);
    text += token.bytes;
    if (token.character == '\n') { if (line > maximum) maximum = line; line = 0; }
    else { line += advance(token, scale); if (line > UINT16_MAX) return UINT16_MAX; }
  }
  return line > maximum ? line : maximum;
}

size_t fittedPrefix(const char* text, int pixels, uint8_t scale, size_t capacity) {
  size_t bytes = 0;
  int used = 0;
  while (text[bytes]) {
    const Token token = next(text + bytes);
    const int step = advance(token, scale);
    if (used + step > pixels || token.bytes > capacity - bytes) break;
    bytes += token.bytes; used += step;
  }
  return bytes;
}
} }
#endif
