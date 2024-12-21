#include "Map.h"

Map::Map(LGFX *display, Character *c, unsigned char (*bmp)[2048]) {
  chara = c;
  tft = display;
  sprite = new LGFX_Sprite(tft);
  width = 240;
  height = 240;
  frame = 0;
  speed = 4;
  status = STATUS_STOP;
  move_diff = 1;
  offset_x = 0;
  image = bmp;
  slow_offset_x = 0;
  slow_scroll_from = 3;
  slow_scroll_to = 3;
  scroll_from = 4;
  scroll_to = 7;
  sprite->createSprite(240, 32 * ((scroll_to - scroll_from) + 1));
}

void Map::setSpeed(uint8_t s) {
  speed = s;
  move_diff = (16 >> (s - 1)) + 2 ;
}

void Map::setDirection(uint8_t d) {
  move_diff = move_diff * (d == MOVE_LEFT ? 1 : (d == MOVE_RIGHT ? -1 : 0));
}

void Map::setStatus(uint8_t d) {
  status = d;  
  if (status == STATUS_WAIT) {
    move_diff = 0;
  }
}

uint32_t Map::update() {
  uint32_t e = 0;
  if (status == STATUS_MOVE || status == STATUS_WAIT) {
    if (frame == 0) {
      offset_x += move_diff;
      offset_x = (offset_x < 0 ? width + offset_x : offset_x) % width;
      e = drawPartialMap();
//      slow_offset_x += move_diff / 2;
//      slow_offset_x = (slow_offset_x < 0 ? width + slow_offset_x : slow_offset_x) % width;
//      e += drawPartialMap(slow_offset_x, slow_scroll_from, slow_scroll_to);
    }
    frame++;
    frame %= speed;
  }
  return e;
}


uint32_t Map::drawEntireMap() {
  uint32_t startTime = millis();
  tft->startWrite();
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      drawBmp((unsigned char *)image[bgmap[i][j]], j * 32, i * 32, 32, 32);
    }
  }
  tft->endWrite();
  return millis() - startTime;
}

uint32_t Map::drawPartialMap() {
  uint32_t startTime = millis();
  for (int i = scroll_from ; i <= scroll_to; i++) {
    for (int j = 0; j < 8; j++) {
      drawBmpOnSprite((unsigned char *)image[bgmap[i][j]], offset_x + j * 32, (i - scroll_from) * 32, 32, 32);
    }
  }
  chara->drawBmpOnSprite(sprite, 0, scroll_from * 32);
  sprite->pushSprite(0, scroll_from * 32);
  return millis() - startTime;
}

uint32_t Map::drawPartialMap(int offset, int from, int to) {
  uint32_t startTime = millis();
  for (int i = from ; i <= to; i++) {
    for (int j = 0; j < 8; j++) {
      drawBmpOnSprite((unsigned char *)image[bgmap[i][j]], offset + j * 32, (i - from) * 32, 32, 32);
    }
  }
  chara->drawBmpOnSprite(sprite, 0, from * 32);
  sprite->pushSprite(0, from * 32);
  return millis() - startTime;
}

uint32_t Map::drawBmp(unsigned char *data, int16_t x, int16_t y, int16_t w,
                      int16_t h) {
  uint16_t row, col, buffidx = 0;

  for (col = 0; col < w; col++) {  // For each scanline...
    for (row = 0; row < h; row++) {
      uint16_t c = pgm_read_word(data + buffidx);
      c = ((c >> 8) & 0x00ff) | ((c << 8) & 0xff00);  // swap back and fore
      tft->drawPixel(col + x, row + y, c);
      buffidx += 2;
    }  // end pixel
  }
  return 0;
}

uint32_t Map::drawBmpOnSprite(unsigned char *data, int16_t x, int16_t y,
                              int16_t w, int16_t h) {
  uint16_t row, col, buffidx = 0;

  for (col = 0; col < w; col++) {  // For each scanline...
    for (row = 0; row < h; row++) {
      uint16_t c = pgm_read_word(data + buffidx);
      c = ((c >> 8) & 0x00ff) | ((c << 8) & 0xff00);  // swap back and fore
      sprite->drawPixel((col + x) % sprite->width(), (row + y), c);
      buffidx += 2;
    }  // end pixel
  }
  return 0;
}
