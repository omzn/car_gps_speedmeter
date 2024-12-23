#include <SPI.h>
#include "LGFX_ST7789_ESP32.hpp"  // Hardware-specific library
#include <TinyGPSPlus.h>

#include <Dusk2Dawn.h>
//#include "DSEG7Classic-BoldItalic26pt7b.h"
//#include "DSEG7Classic-BoldItalic28pt7b.h"
//#include "FreeSansBold12pt7b.h"
#include "MazdaTypeBold32pt.h"
#include "MazdaTypeRegular10pt.h"
#include "Map.h"
#include "bitmaps.h"
#include "character.h"
#include "debugmacros.h"

#define PIN_TOUCH GPIO_NUM_27
#define PIN_GPS_TX GPIO_NUM_17
#define PIN_GPS_RX GPIO_NUM_16

//#define FONT_7SEG28 tft.setFreeFont(&DSEG7Classic_BoldItalic28pt7b)
#define FONT_7SEG_IMG img.setFont(&Mazda_Type_Bold32pt7b)
//#define FONT_SANS_IMG img.setFont(&fonts::FreeSansBold12pt7b)
#define FONT_SANS_IMG img.setFont(&Mazda_Type_Regular10pt7b)

#define TFT_SKY_FINE 0x43fc
#define TFT_SKY_SUNSET 0xfc43
#define TFT_SKY_NIGHT TFT_NAVY

#define TIMEZONE (9)
#define NUM_OF_VIEWS (2)

uint8_t view = 0;
uint32_t last_touched = 0;
uint8_t btnint = 0;

LGFX tft = LGFX();  // Invoke custom library with default width and height
LGFX_Sprite img(&tft);

Character aquatan = Character(&tft, (unsigned char (*)[4][2048])aqua_bmp);
Map background = Map(&tft, &aquatan, (unsigned char (*)[2048])bgimg);

TinyGPSPlus gps;
HardwareSerial hs(2);

Dusk2Dawn  *currentplace = NULL; // losAngeles(34.0522, -118.2437, -8);
int sunrise;
int sunset;

bool debug = true;

int aquatan_speed = 8;
int gps_speed = 0;
double gps_course;
int day_minutes = 0;

int stop_begin_time = 0;
int stop_timer = 0;
int isStopped = 0;

String format_digit(float f, int digits, int decimal = 0) {
  int divnum = pow(10, digits - 1 + decimal);
  int zeroflag = 1;
  int negativeflag = 0;
  String s = "";
  int num = (int)(f * pow(10, decimal));
  if (num < 0) {
    num = -num;
    negativeflag = 1;
  }
  //  Serial.print("num=");
  //  Serial.println(num);
  for (int i = 0; i < digits + decimal; i++) {
    if (num / divnum == 0) {
      if (zeroflag == 1) {
        if (i == digits - 1) {
          zeroflag = 0;
          s += "0";
          if (decimal > 0) {
            s += ".";
          }
        } else {
          s += " ";
        }
      } else {
        s += "0";
        if (i == digits - 1 && decimal > 0) {
          s += ".";
        }
      }
    } else {
      s += String(num / divnum);
      if (i == digits - 1 && decimal > 0) {
        s += ".";
      }
      zeroflag = 0;
    }
    num %= divnum;
    divnum /= 10;
    //    Serial.println(s);
  }
  if (negativeflag) {
    int i = s.lastIndexOf('!');
    if (i >= 0) {
      s.setCharAt(i, '-');
    } else {
      s = '-' + s;
    }
  }
  return s;
}

void handleTouch() {
  if (millis() - last_touched > 500) {
    btnint = 1;
    last_touched = millis();
  }
}

void drawBmpOnSprite(LGFX_Sprite *sp, unsigned char *data, int16_t x, int16_t y,
                     int16_t w, int16_t h) {
  uint16_t row, col, buffidx = 0;
  for (col = 0; col < w; col++) {  // For each scanline...
    for (row = 0; row < h; row++) {
      uint16_t c = pgm_read_word(data + buffidx);
      c = ((c >> 8) & 0x00ff) | ((c << 8) & 0xff00);  // swap back and fore
      sp->drawPixel(col + x, row + y, c);
      buffidx += 2;
    }  // end pixel
  }
}

void drawBG() {
  if (day_minutes >= sunset - 60  && day_minutes < sunset + 25) {
    img.fillSprite(day_minutes < sunset ? TFT_SKY_FINE : TFT_SKY_NIGHT);
    int ypos = (sunset - day_minutes + 25) * 3 / 2; //  0 <= ypos <= 120
    int ygrad = (day_minutes >=sunset) ?  8 + (day_minutes - sunset + 25) * (3 / 2) * (3 / 4) : 8 + (ypos * 3 / 4);
    for (int i=0;i<240;i++) {
      img.drawGradientVLine(i, ygrad, 128, day_minutes < sunset ? TFT_SKY_FINE : TFT_SKY_NIGHT,TFT_SKY_SUNSET);
    }
    img.fillCircle(10,128+25 - ypos/3,25,TFT_YELLOW);
  } else if (day_minutes >= sunrise - 25  && day_minutes < sunrise + 60) {
    img.fillSprite(day_minutes > sunrise ? TFT_SKY_FINE : TFT_SKY_NIGHT);
    int ypos = (day_minutes - sunrise + 25) * 3 / 2; //  0 <= ypos <= 120
    for (int i=0;i<240;i++) {
      img.drawGradientVLine(i, 8 + ypos * 3 / 4, 128, day_minutes > sunrise ? TFT_SKY_FINE : TFT_SKY_NIGHT, TFT_SKY_SUNSET);
    }
    img.fillCircle(10,128+25 - ypos/3,25,TFT_YELLOW);
    // y: ypos = 0 -> 153, ypos = 120 -> 113
  } else if (day_minutes >= sunset + 20 || day_minutes < sunrise ) {
    img.fillSprite(TFT_SKY_NIGHT);
    img.fillCircle(30,30,25,TFT_YELLOW);
    img.fillCircle(25,25,25,TFT_SKY_NIGHT);
  } else { // y = 13 - 133 で動かせる
    int noon = (sunset - sunrise) / 2 + sunrise;
    int ypos = 13 + abs(noon - day_minutes) * 120 / ((sunset - sunrise) / 2);
    img.fillSprite(TFT_SKY_FINE);
    img.fillCircle(10,ypos,25,TFT_YELLOW);
    img.fillArc(10,ypos,28,40,0,10,TFT_YELLOW);
    img.fillArc(10,ypos,28,40,30,40,TFT_YELLOW);
    img.fillArc(10,ypos,28,40,60,70,TFT_YELLOW);
    img.fillArc(10,ypos,28,40,90,100,TFT_YELLOW);
    img.fillArc(10,ypos,28,40,210,220,TFT_YELLOW);
    img.fillArc(10,ypos,28,40,240,250,TFT_YELLOW);
    img.fillArc(10,ypos,28,40,270,280,TFT_YELLOW);
    img.fillArc(10,ypos,28,40,300,310,TFT_YELLOW);
    img.fillArc(10,ypos,28,40,330,340,TFT_YELLOW);
  }
}

void speedBox(int x, int y) {
  drawBG();
  
  FONT_7SEG_IMG;
  img.setTextColor(TFT_WHITE);
  String str = format_digit(gps_speed, 3, 0);
  img.drawString(str, img.width() / 2 + 40 - img.textWidth(str), 16);

  FONT_SANS_IMG;
  str = "km/h";
  img.drawString(str, img.width() / 2 + 110 - img.textWidth(str), 50);
  str = TinyGPSPlus::cardinal(gps_course);
  img.drawString(str, img.width() / 2 + 110 - img.textWidth(str), 16);

  //  img.drawString(String(speed), 66, 16);
  img.pushSprite(x, y, TFT_TRANSPARENT);
}

void timerBox(int x, int y) {
  drawBG();
  
  FONT_7SEG_IMG;
  img.setTextColor(TFT_WHITE);
  String str = format_digit(stop_timer / 1000, 3, 0);
  img.drawString(str, img.width() / 2 + 40 - img.textWidth(str), 16);

  FONT_SANS_IMG;
  str = "sec";
  img.drawString(str, img.width() / 2 + 110 - img.textWidth(str), 50);
  //str = TinyGPSPlus::cardinal(gps_course);
  //img.drawString(str, img.width() / 2 + 110 - img.textWidth(str), 16);

  //  img.drawString(String(speed), 66, 16);
  img.pushSprite(x, y, TFT_TRANSPARENT);
}


void messageBox(int x, int y, String message) {
  img.fillSprite(TFT_NAVY);

  img.drawRoundRect(0, 0, 240, 80, 5, TFT_WHITE);
  img.drawRoundRect(1, 1, 238, 78, 5, TFT_WHITE);
  FONT_SANS_IMG;
  img.setTextColor(TFT_WHITE);

  img.drawString(message, img.width() / 2 - img.textWidth(message) / 2, 30);

  //  img.drawString(message, 10, 20);

  img.pushSprite(x, y, TFT_TRANSPARENT);
}

void drawView(int x, int y, int v) {
  if (v == 0) {
    speedBox(x, y);
  } else if (v == 1) {
//    speedBox(x, y);
    timerBox(x, y);
  }
}

void drawScreen() {
  drawView(0, 0, isStopped);
}

void setup() {
  pinMode(PIN_TOUCH, INPUT);
  tft.init();
  delay(10);
  // tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  img.createSprite(240,128);
  //  drawScreen();
  //  String msg = "Seeking satellites";
  //  messageBox(0, 0, msg);

  Serial.begin(115200);
  hs.begin(9600);
  //  Serial.print("\r\nscan start.\r\n");

  /*  BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setActiveScan(false);
  */
  attachInterrupt(PIN_TOUCH, handleTouch, RISING);
  aquatan_speed = 1;

  background.drawEntireMap();

  background.setDirection(MOVE_LEFT);
  background.setStatus(STATUS_WAIT);
  background.setSpeed(aquatan_speed);

  aquatan.start(ORIENT_FRONT);
  background.setStatus(STATUS_WAIT);
  aquatan.setSpeed(aquatan_speed);
}


void loop() {
  static bool gps_wasvalid = true;
  static uint32_t debug_timer = millis();
  static int speed_diff = 10;

  if (btnint) {
    view++;
    view %= NUM_OF_VIEWS;
    btnint = 0;
  }

  while (hs.available() > 0) {
    gps.encode(hs.read());
  }

  if (millis() > 5000 && gps.charsProcessed() < 10) {
    messageBox(20, 10, "GPS not connected.");
    while (true)
      ;
  }

  if (!gps.location.isValid() && !debug) {
    if (gps_wasvalid) {
      gps_wasvalid = false;
      drawScreen();
      messageBox(0, 0, "Seeking satellites:" + String(gps.satellites.value()));
      background.setStatus(STATUS_WAIT);
      aquatan.setStatus(STATUS_WAIT);
      aquatan.setOrient(ORIENT_SLEEP);
      aquatan.setSpeed(4);
    }
  } else {
    if (debug) {
      /*
      DEBUG MODE
      */
      if (millis() - debug_timer > 1000) {
        debug_timer = millis();
        gps_speed = gps_speed + speed_diff;
        speed_diff = gps_speed == 100 ? -10 : (gps_speed == 0 ? 10 : speed_diff);
        day_minutes += 10;
        day_minutes %= 60*8;
        sunrise = 2 * 60 + 0;
        sunset =  6 * 60 + 0;
        DPRINTF("day_minutes %d sunrise %d sunset %d\n",day_minutes,sunrise,sunset);
      }
      gps_course = 0.0;
    } else {
      gps_speed = gps.speed.kmph();
      gps_course = gps.course.deg();
      day_minutes = ((gps.time.hour() + TIMEZONE) % 24) * 60 + gps.time.minute();
      if (currentplace == NULL) {
        DPRINTF("lat %f  lng %f\n",gps.location.lat(),gps.location.lng());
        currentplace = new Dusk2Dawn(gps.location.lat(),gps.location.lng(),TIMEZONE);
        sunrise = currentplace->sunrise(gps.date.year(),gps.date.month(),gps.date.day(),false);
        sunset = currentplace->sunset(gps.date.year(),gps.date.month(),gps.date.day(),false);
        char sunrise_str[] = "00:00";
        Dusk2Dawn::min2str(sunrise_str,sunrise);
        char sunset_str[] = "00:00";
        Dusk2Dawn::min2str(sunset_str,sunset);
        DPRINTF("SUNRISE %s  SUNSET %s\n",sunrise_str,sunset_str);
      }
    }

    gps_wasvalid = true;
    drawScreen();
    /*
        gps_speed    60 40 20  0
        aquatan_speed 1  2  3  4      aquatan_speed = map(constrain(gps_speed,0,60) / 20, 1, 4, 4, 1)
        bg.move_diff 16  8  4  2      move_diff = 8 / 2 ^ (aquatan_speed - 1) = 8 >> (aquatan_speed - 1)
    */

    if (gps_speed > 5) {
      aquatan_speed = map(constrain(gps_speed, 20, 80) / 20, 1, 4, 4, 1);
      background.setSpeed(aquatan_speed);
      background.setStatus(STATUS_MOVE);
      background.setDirection(MOVE_LEFT);
      aquatan.setSpeed(aquatan_speed);
      if (gps_speed >= 40) {
        aquatan.setOrient(ORIENT_CAR);
      } else {
        aquatan.setOrient(ORIENT_LEFT);
      }
      aquatan.setStatus(STATUS_WAIT);
      isStopped = 0;
    } else {
      aquatan_speed = 4;
      // background.setSpeed(aquatan_speed);
      background.setStatus(STATUS_WAIT);
      aquatan.setSpeed(aquatan_speed);
      aquatan.setOrient(ORIENT_FRONT);
      aquatan.setStatus(STATUS_WAIT);
      if (isStopped == 0) {
        stop_begin_time = millis();
        isStopped = 1;
      }
      stop_timer = millis() - stop_begin_time;      
    }
  }

  int d1 = 0;
  d1 += aquatan.update();
  d1 += background.update();
  int dd = (1000 / FPS) - d1 > 0 ? (1000 / FPS) - d1 : 1;
  delay(dd);
}
