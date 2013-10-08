// Copyright (c) 2013 Matt Hill
// Use of this source code is governed by The MIT License
// that can be found in the LICENSE file.
//
// This class is for controlling a 32x32 RGB LED Matrix panel using
// the Raspberry Pi GPIO.
//
// Buy a 32x32 RGB LED Matrix from Adafruit!
//   http://www.adafruit.com/products/607
//
// For different sizes of RGB LED Matrix, change the constants in this file.
//
// The 32x32 panels can also be chained together to make larger panels.
// When daisy-chaining multiple boards in a square (like four 32x32 boards
// for a 64x64 matrix), columns 1:64 (rows 1:32) are Left to Right across
// the top two boards, but columns 65:128 (rows 33:64) are backwards Right
// to Left across the bottom two boards. (Referenced by: ColumnCnt)
 
#ifndef RPI_RGBMATRIX_H
#define RPI_RGBMATRIX_H

#include <stdint.h>

#include "GpioProxy.h"


struct Color {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};


class RgbMatrix
{
public:

  // Width and Height of the RBG Matrix.
  // If chaining multiple boards together, this is the overall Width x Height.
  static const int Width = 32;
  static const int Height = 32;

  // The 32x32 RGB Matrix is broken into two 16x32 sub-panels. 
  static const int RowsPerSubPanel = 16;
  static const int ColsPerSubPanel = 32;

  // Number of Daisy-Chained Boards
  static const int ChainedBoardsCnt = 1;

  // Number of Columns
  static const int ColumnCnt = ChainedBoardsCnt * ColsPerSubPanel;

  // Pulse Width Modulation (PWM) Resolution 
  static const int PwmBits = 7; //max is 7


  RgbMatrix(GpioProxy *io);

  // Call this in a loop to keep the matrix updated.
  void updateDisplay();

  // Clear the entire display
  void clearDisplay();

  // Clear the inside of the given rectangle.
  void clearRect(uint8_t fx, uint8_t fy, uint8_t fw, uint8_t fh);

  // Fade all pixels on the display to black.
  void fadeDisplay();

  // Fade pixels inside the given rectangle to black.
  void fadeRect(uint8_t fx, uint8_t fy, uint8_t fw, uint8_t fh);

  // Call this after drawing on the display and before calling fadeIn().
  // Before drawing on the display, it's best to first suspend the thread
  // that is calling updateDisplay(). After calling setupFadeIn(), resume
  // the suspended thread and call fadeIn().
  void setupFadeIn();

  // Fade In what has been drawn on the display.
  void fadeIn();

  // Wipe all pixels down off the screen
  void wipeDown();

  //Drawing functions
  void drawPixel(uint8_t x, uint8_t y, Color color);

  void drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, Color color);

  void drawVLine(uint8_t x, uint8_t y, uint8_t h, Color color);

  void drawHLine(uint8_t x, uint8_t y, uint8_t w, Color color);

  void drawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, Color color);

  void fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, Color color);

  void fillScreen(Color color);

  void drawRoundRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t r,
                     Color color);

  void fillRoundRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t r,
                     Color color);
  
  void drawCircle(uint8_t x, uint8_t y, uint8_t r, Color color);

  // Draw one of the four quadrants of a cirle.
  //   quadrant = 1 : Upper Left
  //            = 2 : Upper Right
  //            = 4 : Lower Right
  //            = 8 : Lower Left
  void drawCircleQuadrant(uint8_t x, uint8_t y, uint8_t r, uint8_t quadrant,
                          Color color);

  void fillCircle(uint8_t x, uint8_t y, uint8_t r, Color color);

  // Fill one half of a cirle.
  //   half = 1 : Left
  //        = 2 : Right
  //        = 3 : Both
  //   stretch = number of pixels to stretch the circle vertically.
  void fillCircleHalf(uint8_t x, uint8_t y, uint8_t r,
                      uint8_t half, uint8_t stretch,
                      Color color);

  // Draw an arc.
  //   x : Segment origin
  //   y : Segment origin
  //   r : Segment radius
  //   startAngle : starting angle in degrees  (East == 0)
  //   endAngle : ending angle in degrees
  void drawArc(uint8_t x, uint8_t y, uint8_t r,
               float startAngle, float endAngle,
               Color color);

  // Draw the outline of a wedge.
  //   x : Segment origin
  //   y : Segment origin
  //   r : Segment radius
  //   startAngle : starting angle in degrees  (East == 0)
  //   endAngle : ending angle in degrees
  void drawWedge(uint8_t x, uint8_t y, uint8_t r,
                 float startAngle, float endAngle,
                 Color color);

  void drawTriangle(uint8_t x1, uint8_t y1,
                    uint8_t x2, uint8_t y2,
                    uint8_t x3, uint8_t y3,
                    Color color);

  void fillTriangle(uint8_t x1, uint8_t y1,
                    uint8_t x2, uint8_t y2,
                    uint8_t x3, uint8_t y3,
                    Color color);


  // Special method to create a color wheel on the display.
  // Only works on displays where Height == Width.
  void drawColorWheel();


  // When using writeChar(), the cursor is the location where to start. 
  void setTextCursor(uint8_t x, uint8_t y);

  void setFontColor(Color color);

  // Three sizes are currently available:
  //   size = 1 : Small  (3x5)
  //        = 2 : Medium (4x6)
  //        = 3 : Large  (5x7)
  void setFontSize(uint8_t size);

  void setWordWrap(bool wrap);

  // Write a character using the text cursor and stored Font settings.
  void writeChar(unsigned char c);

  // Put a single character on the display.
  //   x : X for top left origin
  //   y : Y for top left origin
  //   c : the character to draw
  //   size = 1 : Small  (3x5)
  //        = 2 : Medium (4x6)
  //        = 2 : Large  (5x7)
  void putChar(uint8_t x, uint8_t y, unsigned char c, uint8_t size, Color color);


  // Convert an HSV color to an RGB color.
  Color colorHSV(long hue, uint8_t sat, uint8_t val);

 
private:

  GpioProxy *const _gpio;

  // The following data structure represents the pins on the Raspberry Pi GPIO.
  // Each RGB LED Panel requires writing to 2 LED's at a time, so the data
  // structure represents 2 pixels on an RGB LED matrix. The data structure maps
  // the GPIO pins to the LED matrix contols.
  //
  // The GPIO pins are mapped to the LED Matrix as follows:
  //
  //   GPIO 2 (SDA)       -->  OE (Output Enabled)
  //   GPIO 3 (SCL)       -->  CLK (Serial Clock)
  //   GPIO 4 (GPCLK0)    -->  LAT (Data Latch)
  //   GPIO 7 (CE1)       -->  A  --|
  //   GPIO 8 (CE0)       -->  B    |   Row
  //   GPIO 9 (MISO)      -->  C    | Address
  //   GPIO 10 (MOSI)     -->  D  --|
  //   GPIO 17            -->  R1 (LED 1: Red)
  //   GPIO 18 (PCM_CLK)  -->  B1 (LED 1: Blue)
  //   GPIO 22            -->  G1 (LED 1: Green)
  //   GPIO 23            -->  R2 (LED 2: Red)
  //   GPIO 24            -->  B2 (LED 2: Blue)
  //   GPIO 25            -->  G2 (LED 2: Green)
  //
  // ***************************************************************************
  // NOTE: My LED panel has the Green and Blue pins reversed whem compared to
  //       the original mapping I got from Adafruit's website:
  // ***************************************************************************
  //   GPIO 17            -->  R1 (LED 1: Red)
  //   GPIO 18 (PCM_CLK)  -->  G1 (LED 1: Green)
  //   GPIO 22            -->  B1 (LED 1: Blue)
  //   GPIO 23            -->  R2 (LED 2: Red)
  //   GPIO 24            -->  G2 (LED 2: Green)
  //   GPIO 25            -->  B2 (LED 2: Blue)
 
  union GpioPins {
    struct {
      unsigned int ignoredPins1:2;   // 0,1
      unsigned int outputEnabled:1;  // 2
      unsigned int clock:1;          // 3
      unsigned int latch:1;          // 4
      unsigned int ingoredPins2:2;   // 5,6
      unsigned int rowAddress:4;     // 7-10
      unsigned int ignoredPins3:6;   // 11-16
      unsigned int r1:1;             // 17
      unsigned int b1:1;             // 18
      unsigned int ignoredPins4:3;   // 19-21
      unsigned int g1:1;             // 22
      unsigned int r2:1;             // 23
      unsigned int b2:1;             // 24
      unsigned int g2:1;             // 25
    } bits;
    uint32_t raw;
    GpioPins() : raw(0) {}
  };


  // Because a 32x32 Panel is composed of two 16x32 sub-panels, and each
  // 32x32 Panel requires writing an LED from each sub-panel at a time, the
  // following data structure represents two rows: n and n+16.
  struct TwoRows {
    GpioPins column[ColumnCnt];  //TODO: Does this only use color bits?
  };

  struct Display {
    TwoRows row[RowsPerSubPanel];
  };

  Display _plane[PwmBits];
  Display _fadeInPlane[PwmBits]; //2nd plane for hadling fadeIn

  // Members for writing text
  uint8_t _textCursorX, _textCursorY;
  Color _fontColor;
  uint8_t _fontSize;
  uint8_t _fontWidth;
  uint8_t _fontHeight;
  bool _wordWrap;
};

#endif
