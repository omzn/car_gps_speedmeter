#ifndef MAP_H
#define MAP_H

#include "Arduino.h"
#include "bitmaps.h"
//#include "util.h"
#include <pgmspace.h>
//#include <Adafruit_GFX.h>
//#include <Arduino_ST7789.h>
#include "LGFX_ST7789_ESP32.hpp"
#include "character.h"

//#include "DSEG7Classic-Bold18pt7b.h"
//#include <Fonts/FreeSansBold12pt7b.h>
//#include <Fonts/FreeSansBold18pt7b.h>

// 20 frames per second
#define FPS 24

#define BLACK TFT_BLACK
#define WHITE TFT_WHITE
#define SCREEN_BGCOLOR TFT_NAVY

class Map {
 public:
  Map(LGFX *display, Character *c, unsigned char (*bmp)[2048]);
  uint32_t drawBmp(unsigned char *data, int16_t x, int16_t y, int16_t w, int16_t h);
  uint32_t drawBmpOnSprite(unsigned char *data, int16_t x, int16_t y, int16_t w, int16_t h);
  uint32_t drawAquatanOnSprite(unsigned char *data, int16_t x, int16_t y, int16_t w, int16_t h);
  uint32_t setScrollRange(uint8_t from, uint8_t to);
  uint32_t drawEntireMap();
  uint32_t drawPartialMap();
  uint32_t drawPartialMap(int offset, int from, int to);
  void setSpeed(uint8_t s);
  void setDirection(uint8_t d);
  void setStatus(uint8_t d);
  uint32_t update();
 private:
  LGFX *tft;
  LGFX_Sprite *sprite;
  Character *chara;
  unsigned char (*image)[2048];
  int16_t pos_x, pos_y, width, height, target_x, target_y;

  int8_t slow_scroll_from;
  int8_t slow_scroll_to;
  uint8_t scroll_from;
  uint8_t scroll_to;
  int16_t offset_x;
  int16_t slow_offset_x;
  uint8_t status;
  uint8_t speed;    // speed (1-) drawing interval (5 is slow, 1 is fast)
  uint8_t pattern;  // character pattern (0-4)
  uint8_t frame;
  uint8_t direction;
  uint8_t move_diff = 1;
};

#endif