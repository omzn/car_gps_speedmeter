#include <SPI.h>
#include "LGFX_ST7789_ESP32.hpp"  // Hardware-specific library
#include <TinyGPSPlus.h>

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
#define FONT_SANS18 tft.setFont(&fonts::FreeSansBold12pt7b)
//#define FONT_SANS_IMG img.setFont(&fonts::FreeSansBold12pt7b)
#define FONT_SANS_IMG img.setFont(&Mazda_Type_Regular10pt7b)

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

void speedBox(int x, int y) {
  img.fillSprite(0x43fc);

//  img.fillRect(0, 0, 240, 80, 0xfc43);
//  img.drawRoundRect(0, 0, 240, 80, 5, TFT_WHITE);
//  img.drawRoundRect(1, 1, 238, 78, 5, TFT_WHITE);

  FONT_7SEG_IMG;
  img.setTextColor(TFT_WHITE);
  String str = format_digit(gps.speed.kmph(), 3, 0);
  img.drawString(str, img.width() / 2 + 40 - img.textWidth(str), 16);

  FONT_SANS_IMG;
  str = "km/h";
  img.drawString(str, img.width() / 2 + 110 - img.textWidth(str), 50);
  str = TinyGPSPlus::cardinal(gps.course.deg());
  img.drawString(str, img.width() / 2 + 110 - img.textWidth(str), 16);

  //  img.drawString(String(speed), 66, 16);
  img.pushSprite(x, y, TFT_TRANSPARENT);
}

void clockBox(int x, int y) {
  img.fillSprite(TFT_TRANSPARENT);

  img.fillRoundRect(0, 0, 240, 80, 5, TFT_VIOLET);
  img.drawRoundRect(0, 0, 240, 80, 5, TFT_WHITE);
  img.drawRoundRect(1, 1, 238, 78, 5, TFT_WHITE);
  FONT_7SEG_IMG;
  img.setTextColor(TFT_WHITE);

  int hour = (gps.time.hour() + TIMEZONE) % 24;
  int minute = gps.time.minute();

  String str = (hour < 10 ? "!" : "") + String(hour) + ":" + (minute < 10 ? "0" : "") + String(minute);
  img.drawString(str, img.width() / 2 + 40 - img.textWidth(str), 16);

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
    clockBox(x, y);
  }
}

void drawScreen() {
  drawView(0, 0, view);
}

void displayInfo() {
  Serial.print(F("Location: "));
  if (gps.location.isValid()) {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
  } else {
    Serial.print(F("INVALID"));
  }

  Serial.print(F("  Date/Time: "));
  if (gps.date.isValid()) {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
  } else {
    Serial.print(F("INVALID"));
  }

  Serial.print(F(" "));
  if (gps.time.isValid()) {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(F("."));
    if (gps.time.centisecond() < 10) Serial.print(F("0"));
    Serial.print(gps.time.centisecond());
  } else {
    Serial.print(F("INVALID"));
  }

  Serial.println();
}

int aquatan_speed = 8;

void setup() {
  pinMode(PIN_TOUCH, INPUT);
  tft.init();
  delay(10);
  // tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  img.createSprite(240,80);
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

  if (!gps.location.isValid()) {
    if (gps_wasvalid) {
      gps_wasvalid = false;
      drawScreen();
      String msg = "Seeking satellites";
      messageBox(0, 0, msg);
      background.setStatus(STATUS_WAIT);
      aquatan.setStatus(STATUS_WAIT);
      aquatan.setOrient(ORIENT_SLEEP);
      aquatan.setSpeed(4);
    }
  } else {
    gps_wasvalid = true;
    drawScreen();
    /*
        gps_speed    60 40 20  0
        aquatan_speed 1  2  3  4      aquatan_speed = map(constrain(gps_speed,0,60) / 20, 1, 4, 4, 1)
        bg.move_diff 16  8  4  2      move_diff = 8 / 2 ^ (aquatan_speed - 1) = 8 >> (aquatan_speed - 1)
    */

    if (gps.speed.kmph() > 5) {
      aquatan_speed = map(constrain(gps.speed.kmph(), 20, 80) / 20, 1, 4, 4, 1);
      background.setSpeed(aquatan_speed);
      background.setStatus(STATUS_MOVE);
      background.setDirection(MOVE_LEFT);
      aquatan.setSpeed(aquatan_speed);
      aquatan.setOrient(ORIENT_LEFT);
      aquatan.setStatus(STATUS_WAIT);
    } else {
      aquatan_speed = 4;
      // background.setSpeed(aquatan_speed);
      background.setStatus(STATUS_WAIT);
      aquatan.setSpeed(aquatan_speed);
      aquatan.setOrient(ORIENT_FRONT);
      aquatan.setStatus(STATUS_WAIT);
    }
  }

  int d1 = 0;
  d1 += aquatan.update();
  d1 += background.update();
  int dd = (1000 / FPS) - d1 > 0 ? (1000 / FPS) - d1 : 1;
  delay(dd);
  /*
Serial.println(gps.location.lat(), 6); // Latitude in degrees (double)
Serial.println(gps.location.lng(), 6); // Longitude in degrees (double)
Serial.print(gps.location.rawLat().negative ? "-" : "+");
Serial.println(gps.location.rawLat().deg); // Raw latitude in whole degrees
Serial.println(gps.location.rawLat().billionths);// ... and billionths (u16/u32)
Serial.print(gps.location.rawLng().negative ? "-" : "+");
Serial.println(gps.location.rawLng().deg); // Raw longitude in whole degrees
Serial.println(gps.location.rawLng().billionths);// ... and billionths (u16/u32)
Serial.println(gps.date.value()); // Raw date in DDMMYY format (u32)
Serial.println(gps.date.year()); // Year (2000+) (u16)
Serial.println(gps.date.month()); // Month (1-12) (u8)
Serial.println(gps.date.day()); // Day (1-31) (u8)
Serial.println(gps.time.value()); // Raw time in HHMMSSCC format (u32)
Serial.println(gps.time.hour()); // Hour (0-23) (u8)
Serial.println(gps.time.minute()); // Minute (0-59) (u8)
Serial.println(gps.time.second()); // Second (0-59) (u8)
Serial.println(gps.time.centisecond()); // 100ths of a second (0-99) (u8)
Serial.println(gps.speed.value()); // Raw speed in 100ths of a knot (i32)
Serial.println(gps.speed.knots()); // Speed in knots (double)
Serial.println(gps.speed.mph()); // Speed in miles per hour (double)
Serial.println(gps.speed.mps()); // Speed in meters per second (double)
Serial.println(gps.speed.kmph()); // Speed in kilometers per hour (double)
Serial.println(gps.course.value()); // Raw course in 100ths of a degree (i32)
Serial.println(gps.course.deg()); // Course in degrees (double)
Serial.println(gps.altitude.value()); // Raw altitude in centimeters (i32)
Serial.println(gps.altitude.meters()); // Altitude in meters (double)
Serial.println(gps.altitude.miles()); // Altitude in miles (double)
Serial.println(gps.altitude.kilometers()); // Altitude in kilometers (double)
Serial.println(gps.altitude.feet()); // Altitude in feet (double)
Serial.println(gps.satellites.value()); // Number of satellites in use (u32)
Serial.println(gps.hdop.value()); // Horizontal Dim. of Precision (100ths-i32)
  */

  /*  BLEScanResults foundDevices = pBLEScan->start(1);
    int count = foundDevices.getCount();
    for (int i = 0; i < count; i++) {
      BLEAdvertisedDevice d = foundDevices.getDevice(i);
      if (d.haveManufacturerData()) {
        std::string data = d.getManufacturerData();
        int manu = data[1] << 8 | data[0];
        if (manu == MyManufacturerId && seq != data[2]) {
          found = true;
          seq = data[2];
          if (seq > 5) {
            stable = 1;
          }
          temp = (float)(data[4] << 8 | data[3]) / 100.0;
          humid = (float)(data[6] << 8 | data[5]) / 100.0;
          press = (float)(data[8] << 8 | data[7]) / 10.0;
          co2 = (float)(data[10] << 8 | data[9]) / 10.0;
          Serial.printf(">>> seq: %d, t: %.1f, h: %.1f, p: %.1f c: %.1f\r\n", seq, temp, humid, press, co2);
          if (humid == 0 || press < 900 || co2 == 0 || co2 > 5000) {
            valid_data = false;
            break;
          }
          if (stable) {
            temperature_hist[temperature_hist_p++] = temp;
            humidity_hist[humidity_hist_p++] = humid;
            pressure_hist[pressure_hist_p++] = press;
            co2_hist[co2_hist_p++] = co2;
            temperature_hist_p %= SENSOR_HIST;
            humidity_hist_p %= SENSOR_HIST;
            pressure_hist_p %= SENSOR_HIST;
            co2_hist_p %= SENSOR_HIST;
          }
        }
      }
    }
  */
  /*  if (btnint) {
      DPRINTLN("touched");
      btnint = 0;
      int p_view = view;
      view++;
      view %= NUM_OF_VIEWS;
      //    drawScreen(temp, humid, press, co2);
      for (int i = 0; i < 12; i++) {
  //      drawView(0 - i * 20, 0, p_view, temp, humid, press, co2);
  //      drawView(240 - i * 20, 0, view, temp, humid, press, co2);
  //      delay(6);
      }
      drawScreen(gps_speed);
    }
  */
}
