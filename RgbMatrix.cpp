// Copyright (c) 2013 Matt Hill
// Use of this source code is governed by The MIT License
// that can be found in the LICENSE file.

// This class is for controlling a 32x32 RGB LED Matrix panel using
// the Raspberry Pi GPIO.
//
// This code is based on a cool example found at:
//   https://github.com/hzeller/rpi-rgb-led-matrix

#include "RgbMatrix.h"

#include "font3x5.h"
#include "font5x7.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <algorithm>

#define _USE_MATH_DEFINES

#define pgm_read_byte(addr) (*(const unsigned char *)(addr))


// Clocking in a row takes about 3.4usec (TODO: per board)
// Because clocking the data in is part of the 'wait time', we need to
// substract that from the row sleep time.
static const int RowClockTime = 3400;

const long row_sleep_nanos[8] = {   // Only using the first PwmResolution elements.
   (1 * RowClockTime) - RowClockTime,
   (2 * RowClockTime) - RowClockTime,
   (4 * RowClockTime) - RowClockTime,
   (8 * RowClockTime) - RowClockTime,
  (16 * RowClockTime) - RowClockTime,
  (32 * RowClockTime) - RowClockTime,
  (64 * RowClockTime) - RowClockTime,
  // Too much flicker with 8 bits. We should have a separate screen pass
  // with this bit plane. Or interlace. Or trick with -OE switch on in the
  // middle of row-clocking, thus have RowClockTime / 2
  (128 * RowClockTime) - RowClockTime, // too much flicker.
};

static void sleep_nanos(long nanos)
{
  // For sleep times above 20usec, nanosleep seems to be fine, but it has
  // an offset of about 20usec (on the RPi distribution I was testing it on).
  // That means, we need to give it 80us to get 100us.
  // For values lower than roughly 30us, this is not accurate anymore and we
  // need to switch to busy wait.
  // TODO: compile Linux kernel realtime extensions and watch if the offset-time
  // changes and hope for less jitter.
  if (nanos > 28000)
  {
    struct timespec sleep_time = { 0, nanos - 20000 };
    nanosleep(&sleep_time, NULL);
  }
  else
  {
    // The following loop is determined empirically on a 700Mhz RPi
    for (int i = nanos >> 2; i != 0; --i)
    {
      asm("");   // force GCC not to optimize this away.
    }
  }
}



RgbMatrix::RgbMatrix(GpioProxy *io) : _gpio(io)
{
  // Tell GPIO about the pins we will use.
  GpioPins b;

  b.raw = 0;

  b.bits.outputEnabled = b.bits.clock = b.bits.latch = 1;
  b.bits.r1 = b.bits.g1 = b.bits.b1 = 1;
  b.bits.r2 = b.bits.g2 = b.bits.b2 = 1;
  b.bits.rowAddress = 0xf; //binary: 1111

  // Initialize outputs, make sure that all of these are supported bits.
  const uint32_t result = _gpio->setupOutputBits(b.raw);

  assert(result == b.raw);
  assert(PwmResolution < 8);    // only up to 7 makes sense.

  clearDisplay();
}


void RgbMatrix::clearDisplay()
{
  memset(&_plane, 0, sizeof(_plane));
}


//TODO: Notes...
void RgbMatrix::updateDisplay()
{
  GpioPins serial_mask;   // Mask of bits we need to set while clocking in.
  serial_mask.bits.r1 = serial_mask.bits.g1 = serial_mask.bits.b1 = 1;
  serial_mask.bits.r2 = serial_mask.bits.g2 = serial_mask.bits.b2 = 1;
  serial_mask.bits.clock = 1;

  GpioPins row_mask;
  row_mask.bits.rowAddress = 0xf;

  //TODO: Why do I need a whole "GpioPins" for these bits?
  GpioPins clock, output_enable, latch;

  clock.bits.clock = 1;
  output_enable.bits.outputEnabled = 1;
  latch.bits.latch = 1;

  GpioPins row_bits;

  for (uint8_t row = 0; row < RowsPerSubPanel; ++row)
  {
    // Rows can't be switched very quickly without ghosting, so we do the
    // full PWM of one row before switching rows.
    for (int b = 0; b < PwmResolution; ++b)
    {
      const TwoRows &rowdata = _plane[b].row[row];

      // Clock in the row. The time this takes is the smalles time we can
      // leave the LEDs on, thus the smallest time-constant we can use for
      // PWM (doubling the sleep time with each bit).
      // So this is the critical path; I'd love to know if we can employ some
      // DMA techniques to speed this up.
      // (With this code, one row roughly takes 3.0 - 3.4usec to clock in).
      //
      // However, in particular for longer chaining, it seems we need some more
      // wait time to settle.
      const long kIOStabilizeWaitNanos = 0; //TODO: mateo was 256

      for (uint8_t col = 0; col < ColumnCnt; ++col)
      {
        const GpioPins &out = rowdata.column[col];
        _gpio->clearBits(~out.raw & serial_mask.raw);  // also: resets clock.
        sleep_nanos(kIOStabilizeWaitNanos);
        _gpio->setBits(out.raw & serial_mask.raw);
        sleep_nanos(kIOStabilizeWaitNanos);
        _gpio->setBits(clock.raw);
        sleep_nanos(kIOStabilizeWaitNanos);
      }

      _gpio->setBits(output_enable.raw);  // switch off while strobe (latch).

      row_bits.bits.rowAddress = row;
      _gpio->setBits(row_bits.raw & row_mask.raw);
      _gpio->clearBits(~row_bits.raw & row_mask.raw);

      _gpio->setBits(latch.raw);   // strobe
      _gpio->clearBits(latch.raw);

      // Now switch on for the given sleep time.
      _gpio->clearBits(output_enable.raw);
      // If we use less bits, then use the upper areas which leaves us more
      // CPU time to do other stuff.
      sleep_nanos(row_sleep_nanos[b + (7 - PwmResolution)]);
    }
  }
}


void RgbMatrix::drawPixel(uint8_t x, uint8_t y,
                          uint8_t red, uint8_t green, uint8_t blue)
{
  if (x >= Width || y >= Height) return;

  // My setup: having four panels connected  [>] [>]
  //                                         [<] [<]
  // So we have up to column 64 one direction, then folding around. Lets map
  // that backward
  if (y > 31) 
  {
    x = 127 - x;
    y = 63 - y;
  }
  
  // TODO: re-map values to be luminance corrected (sometimes called 'gamma').
  // Ideally, we had like 10PWM bits for this, but we're too slow for that :/
  
  // Scale to the number of bit planes we actually have, so that MSB matches
  // MSB of PWM.
  red   >>= 8 - PwmResolution;
  green >>= 8 - PwmResolution;
  blue  >>= 8 - PwmResolution;

  for (int b = 0; b < PwmResolution; ++b)
  {
    uint8_t mask = 1 << b;
    GpioPins *bits = &_plane[b].row[y & 0xf].column[x];

    if (y < 16)
    {
      // Upper sub-panel
      bits->bits.r1 = (red & mask) == mask;
      //TODO: Figure out why these are reversed. Did I wire backwards??
      //bits->bits.g1 = (green & mask) == mask;
      //bits->bits.b1 = (blue & mask) == mask;
      bits->bits.b1 = (green & mask) == mask;
      bits->bits.g1 = (blue & mask) == mask;
 
    }
    else
    {
      // Lower sub-panel
      bits->bits.r2 = (red & mask) == mask;
      //TODO: Figure out why these are reversed. Did I wire backwards??
      //bits->bits.g2 = (green & mask) == mask;
      //bits->bits.b2 = (blue & mask) == mask;
      bits->bits.b2 = (green & mask) == mask;
      bits->bits.g2 = (blue & mask) == mask;
    }
  }
}


// Bresenham's Line Algorithm
void RgbMatrix::drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,
                         uint8_t red, uint8_t green, uint8_t blue)
{
  int16_t steep = abs(y1 - y0) > abs(x1 - x0);

  if (steep)
  {
    std::swap(x0, y0);
    std::swap(x1, y1);
  }

  if (x0 > x1)
  {
    std::swap(x0, x1);
    std::swap(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int16_t err = dx / 2;
  int16_t ystep;

  if (y0 < y1)
  {
    ystep = 1;
  }
  else
  {
    ystep = -1;
  }

  for (; x0 <= x1; x0++)
  {
    if (steep)
    {
      drawPixel(y0, x0, red, green, blue);
    }
    else
    {
      drawPixel(x0, y0, red, green, blue);
    }

    err -= dy;

    if (err < 0)
    {
      y0 += ystep;
      err += dx;
    }
  }
}


// Draw a vertical line
void RgbMatrix::drawVLine(uint8_t x, uint8_t y, uint8_t h,
                          uint8_t red, uint8_t green, uint8_t blue)
{
  drawLine(x, y, x, y + h - 1, red, green, blue);
}


// Draw a horizontal line
void RgbMatrix::drawHLine(uint8_t x, uint8_t y, uint8_t w,
                          uint8_t red, uint8_t green, uint8_t blue)
{
  drawLine(x, y, x + w - 1, y, red, green, blue);
}


// Draw the outline of a rectangle (no fill)
void RgbMatrix::drawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                         uint8_t red, uint8_t green, uint8_t blue)
{
  drawHLine(x, y, w, red, green, blue);
  drawHLine(x, y + h - 1, w, red, green, blue);
  drawVLine(x, y, h, red, green, blue);
  drawVLine(x + w - 1, y, h, red, green, blue);
}


void RgbMatrix::fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                         uint8_t red, uint8_t green, uint8_t blue)
{
  for (uint8_t i = x; i < x + w; i++)
  {
    drawVLine(i, y, h, red, green, blue);
  }
}


void RgbMatrix::fillScreen(uint8_t red, uint8_t green, uint8_t blue)
{
  fillRect(0, 0, Width, Height, red, green, blue);
}


// Draw a rounded rectangle with radius r.
void RgbMatrix::drawRoundRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t r,
                              uint8_t red, uint8_t green, uint8_t blue)
{
  drawHLine(x + r    , y        , w - 2 * r, red, green, blue);
  drawHLine(x + r    , y + h - 1, w - 2 * r, red, green, blue);
  drawVLine(x        , y + r    , h - 2 * r, red, green, blue);
  drawVLine(x + w - 1, y + r    , h - 2 * r, red, green, blue);

  drawCircleQuadrant(x + r        , y + r        , r, 1, red, green, blue);
  drawCircleQuadrant(x + w - r - 1, y + r        , r, 2, red, green, blue);
  drawCircleQuadrant(x + w - r - 1, y + h - r - 1, r, 4, red, green, blue);
  drawCircleQuadrant(x + r        , y + h - r - 1, r, 8, red, green, blue);
}


void RgbMatrix::fillRoundRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t r,
                              uint8_t red, uint8_t green, uint8_t blue)
{
  fillRect(x + r, y, w - 2 * r, h, red, green, blue);

  fillCircleHalf(x + r        , y + r, r, 1, h - 2 * r - 1, red, green, blue);
  fillCircleHalf(x + w - r - 1, y + r, r, 2, h - 2 * r - 1, red, green, blue);
}


// Draw the outline of a cirle (no fill) - Midpoint Circle Algorithm
void RgbMatrix::drawCircle(uint8_t x, uint8_t y, uint8_t r,
                           uint8_t red, uint8_t green, uint8_t blue)
{
  int16_t f = 1 - r;
  int16_t ddFx = 1;
  int16_t ddFy = -2 * r;
  int16_t x1 = 0;
  int16_t y1 = r;

  drawPixel(x, y + r, red, green, blue);
  drawPixel(x, y - r, red, green, blue);
  drawPixel(x + r, y, red, green, blue);
  drawPixel(x - r, y, red, green, blue);

  while (x1 < y1)
  {
    if (f >= 0)
    {
      y1--;
      ddFy += 2;
      f += ddFy;
    }

    x1++;
    ddFx += 2;
    f += ddFx;

    drawPixel(x + x1, y + y1, red, green, blue);
    drawPixel(x - x1, y + y1, red, green, blue);
    drawPixel(x + x1, y - y1, red, green, blue);
    drawPixel(x - x1, y - y1, red, green, blue);
    drawPixel(x + y1, y + x1, red, green, blue);
    drawPixel(x - y1, y + x1, red, green, blue);
    drawPixel(x + y1, y - x1, red, green, blue);
    drawPixel(x - y1, y - x1, red, green, blue);
  }
}

// Draw one of the four quadrants of a circle.
void RgbMatrix::drawCircleQuadrant(uint8_t x, uint8_t y, uint8_t r, uint8_t quadrant,
                                   uint8_t red, uint8_t green, uint8_t blue)
{
  int16_t f = 1 - r;
  int16_t ddFx = 1;
  int16_t ddFy = -2 * r;
  int16_t x1 = 0;
  int16_t y1 = r;

  while (x1 < y1)
  {
    if (f >= 0)
    {
      y1--;
      ddFy += 2;
      f += ddFy;
    }

    x1++;
    ddFx += 2;
    f += ddFx;

    //Upper Left
    if (quadrant & 0x1)
    {
      drawPixel(x - y1, y - x1, red, green, blue);
      drawPixel(x - x1, y - y1, red, green, blue);
    }

    //Upper Right
    if (quadrant & 0x2)
    {
      drawPixel(x + x1, y - y1, red, green, blue);
      drawPixel(x + y1, y - x1, red, green, blue);
    }

    //Lower Right
    if (quadrant & 0x4)
    {
      drawPixel(x + x1, y + y1, red, green, blue);
      drawPixel(x + y1, y + x1, red, green, blue);
    }

    //Lower Left
    if (quadrant & 0x8)
    {
      drawPixel(x - y1, y + x1, red, green, blue);
      drawPixel(x - x1, y + y1, red, green, blue);
    } 
  }
}


void RgbMatrix::fillCircle(uint8_t x, uint8_t y, uint8_t r,
                           uint8_t red, uint8_t green, uint8_t blue)
{
  drawVLine(x, y - r, 2 * r + 1, red, green, blue);
  fillCircleHalf(x, y, r, 3, 0, red, green, blue);
}


void RgbMatrix::fillCircleHalf(uint8_t x, uint8_t y, uint8_t r,
                               uint8_t half, uint8_t stretch,
                               uint8_t red, uint8_t green, uint8_t blue)
{
  int16_t f = 1 - r;
  int16_t ddFx = 1;
  int16_t ddFy = -2 * r;
  int16_t x1 = 0;
  int16_t y1 = r;

  while (x1 < y1)
  {
    if (f >= 0)
    {
      y1--;
      ddFy += 2;
      f += ddFy;
    }

    x1++;
    ddFx += 2;
    f += ddFx;

    //Left
    if (half & 0x1)
    {
      drawVLine(x - x1, y - y1, 2 * y1 + 1 + stretch, red, green, blue);
      drawVLine(x - y1, y - x1, 2 * x1 + 1 + stretch, red, green, blue);
    }

    //Right
    if (half & 0x2)
    {
      drawVLine(x + x1, y - y1, 2 * y1 + 1 + stretch, red, green, blue);
      drawVLine(x + y1, y - x1, 2 * x1 + 1 + stretch, red, green, blue);
   }
  }
}

// Draw an Arc
void RgbMatrix::drawArc(uint8_t x, uint8_t y, uint8_t r,
                        float startAngle, float endAngle,
                        uint8_t red, uint8_t green, uint8_t blue)
{
  // Convert degrees to radians
  float degreesPerRadian = M_PI / 180;

  startAngle *= degreesPerRadian;
  endAngle *= degreesPerRadian;
  float step = 1 * degreesPerRadian; //number of degrees per point on the arc

  float prevX = x + r * cos(startAngle);
  float prevY = y + r * sin(startAngle);
 
  // Draw the arc
  for (float theta = startAngle; theta < endAngle; theta += std::min(step, endAngle - theta))
  {
    drawLine(prevX, prevY, x + r * cos(theta), y + r * sin(theta), red, green, blue);

    prevX = x + r * cos(theta);
    prevY = y + r * sin(theta);
  }

  drawLine(prevX, prevY, x + r * cos(endAngle), y + r * sin(endAngle), red, green, blue);
}
 

// Draw the outline of a wedge.
void RgbMatrix::drawWedge(uint8_t x, uint8_t y, uint8_t r,  //TODO: add inner radius
                          float startAngle, float endAngle,
                          uint8_t red, uint8_t green, uint8_t blue)
{
  // Convert degrees to radians
  float degreesPerRadian = M_PI / 180;

  float startAngleDeg = startAngle * degreesPerRadian;
  float endAngleDeg = endAngle * degreesPerRadian;

  uint8_t prevX = x + r * cos(startAngleDeg);
  uint8_t prevY = y + r * sin(startAngleDeg);

  drawLine(x, y, prevX, prevY, red, green, blue);

  drawArc(x, y, r, startAngle, endAngle, red, green, blue);

  drawLine(x + r * cos(endAngleDeg), y + r * sin(endAngleDeg), x, y, red, green, blue);
}


//TODO: more shapes...



// Draw a character.
//TODO: Add size: uint8_t size (0x1, 0x2...) (Small, Large)
void RgbMatrix::drawChar(uint8_t x, uint8_t y, unsigned char c, uint8_t size,
                         uint8_t red, uint8_t green, uint8_t blue)
{
  unsigned char font[] = Font5x7;
  uint8_t fontWidth = 5;
  uint8_t fontHeight = 7;

  if (size & 0x1) //small (3x5)
  {
    font = Font3x5;
    fontWidth = 3;
    fontHeight = 5;
  }
  else if (size & 0x2) //large (5x7)
  {
    ; //already set as default.
  }

  for (int i=0; i < fontWidth+1; i++)
  {
    uint8_t line;

    if (i == fontWidth)
    {
      line = 0x0;
    }
    else
    {
      line = pgm_read_byte(font + ((c - 0x20) * 5) + i);
    }

    for (int j=0; j < fontHeight+1; j++)
    {
      if (line & 0x1)
      {
        drawPixel(x+i, y+j, red, green, blue);
      }

      line >>= 1;
    }
  }
}


 
