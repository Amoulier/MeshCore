#pragma once
#include <stddef.h>
#include <stdint.h>

namespace superbase { namespace emoji {
struct Node { uint32_t cp; uint16_t first, count; int16_t glyph; };
extern const Node nodes[];
extern const uint16_t glyphRows[][10];
extern const size_t nodeCount, glyphCount, sequenceCount;
struct Token { size_t bytes; int16_t glyph; uint8_t character; };
// One complete emoji sequence, one displayable text character, or an ignored selector.
Token next(const char* text);
void copyUtf8(char* dest, const char* src, size_t capacity);
uint16_t width(const char* text, uint8_t scale = 1);
size_t fittedPrefix(const char* text, int pixels, uint8_t scale, size_t capacity);
inline int advance(const Token& token, uint8_t scale) {
  return (token.glyph >= 0 ? 11 : token.character >= 32 ? 6 : 0) * scale;
}

template<class Surface>
void print(Surface& surface, const char* text, uint8_t scale, uint16_t color) {
  int x = surface.getCursorX(), y = surface.getCursorY();
  while (*text) {
    const Token token = next(text);
    text += token.bytes;
    if (token.character == '\r') continue;
    const int step = advance(token, scale);
    if (token.character == '\n' || (step && x + step > surface.width() && x > 0)) {
      x = 0; y += 10 * scale;
      if (token.character == '\n') continue;
    }
    if (token.glyph >= 0) {
      for (unsigned row = 0; row < 10; ++row)
        for (unsigned col = 0; col < 10; ++col)
          if (glyphRows[token.glyph][row] & (1U << (9-col)))
            surface.fillRect(x + int(col)*scale, y + int(row)*scale, scale, scale, color);
    } else if (token.character >= 32) {
      surface.setCursor(x, y);
      surface.write(token.character);
    }
    x += step;
  }
  surface.setCursor(x, y);
}
} }
