// Variadic macros used to print information in de-bugging mode
// from LarryD, Arduino forum

#pragma once
// un-comment this line to print the debugging statements
#define DEBUG

#ifdef DEBUG
#define DPRINT(...)    Serial.print(__VA_ARGS__)
#define DPRINTLN(...)  Serial.println(__VA_ARGS__)
#define DPRINTF(...)  Serial.printf(__VA_ARGS__)
//#define DPRINT(...)    M5.Lcd.print(__VA_ARGS__)
//#define DPRINTLN(...)  M5.Lcd.println(__VA_ARGS__)
#else
  // define blank line
  #define DPRINT(...)
  #define DPRINTLN(...)
  #define DPRINTF(...)
#endif
