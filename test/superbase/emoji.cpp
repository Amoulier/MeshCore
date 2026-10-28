#include <helpers/ui/EmojiText.h>
#include <array>
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace superbase::emoji;

static std::string utf8(uint32_t cp) {
  std::string out;
  if (cp < 0x80) out += char(cp);
  else if (cp < 0x800) { out += char(0xC0 | (cp>>6)); out += char(0x80 | (cp&63)); }
  else if (cp < 0x10000) {
    out += char(0xE0 | (cp>>12)); out += char(0x80 | ((cp>>6)&63)); out += char(0x80 | (cp&63));
  } else {
    out += char(0xF0 | (cp>>18)); out += char(0x80 | ((cp>>12)&63));
    out += char(0x80 | ((cp>>6)&63)); out += char(0x80 | (cp&63));
  }
  return out;
}

static size_t verifyFont(unsigned index, std::string text, std::string selectors) {
  const Node& node = nodes[index];
  assert(node.first + node.count <= nodeCount);
  size_t total = 0;
  if (node.glyph >= 0) {
    assert(size_t(node.glyph) < glyphCount);
    Token token = next(text.c_str());
    assert(token.glyph == node.glyph && token.bytes == text.size());
    token = next(selectors.c_str());
    assert(token.glyph == node.glyph && token.bytes == selectors.size());
    ++total;
  }
  for (unsigned i = 0; i < node.count; ++i) {
    const unsigned child = node.first + i;
    if (i) assert(nodes[child-1].cp < nodes[child].cp);
    const std::string cp = utf8(nodes[child].cp);
    total += verifyFont(child, text + cp, selectors + cp + "\xEF\xB8\x8F");
  }
  return total;
}

struct Surface {
  int x = 0, y = 0, scale = 1;
  std::array<unsigned char, 128*128> pixels = {};
  std::array<unsigned char, 1280> font = {};
  int getCursorX() { return x; }
  int getCursorY() { return y; }
  int width() { return 128; }
  void setCursor(int nx, int ny) { x = nx; y = ny; }
  void fillRect(int left, int top, int w, int h, uint16_t color) {
    for (int row = top; row < top+h; ++row) for (int col = left; col < left+w; ++col)
      if (row >= 0 && row < 128 && col >= 0 && col < 128) pixels[row*128+col] = color ? 255 : 0;
  }
  void write(uint8_t c) {
    for (int col = 0; col < 5; ++col) for (int row = 0; row < 8; ++row)
      if (font[c*5+col] & (1 << row)) fillRect(x + col*scale, y + row*scale, scale, scale, 1);
    x += 6*scale;
  }
};

int main(int argc, char** argv) {
  const char* joystick = "\xF0\x9F\x95\xB9\xEF\xB8\x8F";
  assert(next(joystick).bytes == 7 && next(joystick).glyph >= 0);
  assert(next("\xF0\x9F\x95\xB9").glyph == next(joystick).glyph);
  assert(width("MuziWorks \xF0\x9F\x95\xB9\xEF\xB8\x8F") == 71);
  assert(width("1") == 6 && width("1\xEF\xB8\x8F\xE2\x83\xA3") == 11);
  assert(next("\xC3\xB1").character == 164);
  const char* family = "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA7\xE2\x80\x8D\xF0\x9F\x91\xA6";
  assert(next(family).bytes == strlen(family) && width(family) == 11);
  assert(width("\xF0\x9F\x87\xA7\xF0\x9F\x87\xB4") == 11); // Bolivia flag.
  assert(width("\xF0\x9F\x91\x8D\xF0\x9F\x8F\xBD") == 11); // Skin tone sequence.
  char buffer[16];
  memset(buffer, '!', sizeof(buffer));
  copyUtf8(buffer, joystick, 0); assert(buffer[0] == '!');
  copyUtf8(buffer, joystick, 7); assert(buffer[0] == 0 && buffer[7] == '!');
  copyUtf8(buffer, joystick, 8); assert(strcmp(buffer, joystick) == 0 && buffer[8] == '!');
  const std::string mixed = std::string("AB") + joystick + "C";
  assert(fittedPrefix(mixed.c_str(), 12, 1, 255) == 2);
  assert(fittedPrefix(mixed.c_str(), 23, 1, 255) == 9);
  assert(fittedPrefix(mixed.c_str(), 100, 1, 8) == 2);
  for (const std::string& bad : {std::string("\xF0"), std::string("\xF0\x9F"), std::string("\xF0\x9F\x95"),
       std::string("\xED\xA0\x80"), std::string("\xF4\x90\x80\x80"), std::string("\xC0\x80")}) {
    // Exact-sized buffers make sanitizer checks catch any read past a short input.
    std::vector<char> input(bad.begin(), bad.end()); input.push_back(0);
    const char* cursor = input.data();
    while (*cursor) { Token token = next(cursor); assert(token.bytes > 0); cursor += token.bytes; }
    assert(cursor <= input.data() + bad.size());
  }
  assert(verifyFont(0, "", "") == sequenceCount);
  Surface surface;
  surface.x = 120;
  superbase::emoji::print(surface, joystick, 1, 1);
  assert(surface.x == 11 && surface.y == 10);
  surface.x = surface.y = 0; surface.scale = 2;
  superbase::emoji::print(surface, joystick, 2, 1);
  assert(surface.x == 22 && surface.y == 0);
  if (argc == 4 && std::string(argv[1]) == "--render") {
    Surface preview;
    std::ifstream font(argv[2], std::ios::binary);
    font.read(reinterpret_cast<char*>(preview.font.data()), preview.font.size());
    assert(font.gcount() == 1280);
    preview.setCursor(0,2);
    superbase::emoji::print(preview, "MuziWorks \xF0\x9F\x95\xB9\xEF\xB8\x8F",1,1);
    preview.setCursor(0,24);
    superbase::emoji::print(preview,"\xF0\x9F\x99\x82 \xF0\x9F\x9A\x80 \xE2\x9D\xA4\xEF\xB8\x8F \xF0\x9F\x94\x8B \xF0\x9F\x93\xA1",1,1);
    preview.setCursor(0,42);
    superbase::emoji::print(preview,family,1,1);
    superbase::emoji::print(preview," \xF0\x9F\x87\xA7\xF0\x9F\x87\xB4 \xF0\x9F\x91\x8D\xF0\x9F\x8F\xBD",1,1);
    preview.setCursor(0,64); preview.scale = 2;
    superbase::emoji::print(preview,joystick,2,1);
    std::ofstream image(argv[3], std::ios::binary);
    image << "P5\n128 128\n255\n";
    image.write(reinterpret_cast<char*>(preview.pixels.data()), preview.pixels.size());
  }
  std::cout << "PASS: " << sequenceCount << " emoji sequences, selectors, flags, modifiers, UTF-8 bounds, width and rendering\n";
}
