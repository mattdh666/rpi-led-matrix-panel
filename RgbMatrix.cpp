// Copyright (c) 2013 Matt Hill
// Use of this source code is governed by The MIT License
// that can be found in the LICENSE file.
//
// This class is for controlling a 32x32 RGB LED Matrix panel using
// the Raspberry Pi GPIO.
//
// This code is based on a cool example found at:
//   https://github.com/hzeller/rpi-rgb-led-matrix

#include "RgbMatrix.h"

#include "Font3x5.h"
#include "Font4x6.h"
#include "Font5x7.h"

//#include "Gamma.h"

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

const long RowSleepNanos[8] = {   // Only using the first PwmBits elements.
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

static void sleepNanos(long nanos)
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
      asm("");  // Force GCC not to optimize this away.
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
  assert(PwmBits < 8);  // only up to 7 makes sense.

  //Initialize text members
  _textCursorX = 0;
  _textCursorY = 0;
  Color white;
  white.red = 255;
  white.green = 255;
  white.blue = 255;
  _fontColor = white;
  _fontSize = 1;
  _fontWidth = 3;
  _fontHeight = 5;
  _wordWrap = true;

  clearDisplay();
}


// Write pixels to the LED panel.
void RgbMatrix::updateDisplay()
{
  GpioPins serialMask;   // Mask of bits we need to set while clocking in.
  serialMask.bits.r1 = serialMask.bits.g1 = serialMask.bits.b1 = 1;
  serialMask.bits.r2 = serialMask.bits.g2 = serialMask.bits.b2 = 1;
  serialMask.bits.clock = 1;

  GpioPins rowMask;
  rowMask.bits.rowAddress = 0xf;

  GpioPins clock, outputEnable, latch;

  clock.bits.clock = 1;
  outputEnable.bits.outputEnabled = 1;
  latch.bits.latch = 1;

  GpioPins rowBits;

  for (uint8_t row = 0; row < RowsPerSubPanel; ++row)
  {
    // Rows can't be switched very quickly without ghosting, so we do the
    // full PWM of one row before switching rows.
    for (int b = 0; b < PwmBits; b++)
    {
      const TwoRows &rowData = _plane[b].row[row];

      // Clock in the row. The time this takes is the smallest time we can
      // leave the LEDs on, thus the smallest time-constant we can use for
      // PWM (doubling the sleep time with each bit).
      // So this is the critical path; I'd love to know if we can employ some
      // DMA techniques to speed this up.
      // (With this code, one row roughly takes 3.0 - 3.4usec to clock in).
      //
      // However, in particular for longer chaining, it seems we need some more
      // wait time to settle.
      const long StabilizeWaitNanos = 256; //TODO: mateo was 256

      for (uint8_t col = 0; col < ColumnCnt; ++col)
      {
        const GpioPins &out = rowData.column[col];
        _gpio->clearBits(~out.raw & serialMask.raw);  // also: resets clock.
        sleepNanos(StabilizeWaitNanos);
        _gpio->setBits(out.raw & serialMask.raw);
        sleepNanos(StabilizeWaitNanos);
        _gpio->setBits(clock.raw);
        sleepNanos(StabilizeWaitNanos);
      }

      _gpio->setBits(outputEnable.raw);  // switch off while strobe (latch).

      rowBits.bits.rowAddress = row;
      _gpio->setBits(rowBits.raw & rowMask.raw);
      _gpio->clearBits(~rowBits.raw & rowMask.raw);

      _gpio->setBits(latch.raw);   // strobe - on and off
      _gpio->clearBits(latch.raw);

      // Now switch on for the given sleep time.
      _gpio->clearBits(outputEnable.raw);

      // If we use less bits, then use the upper areas which leaves us more
      // CPU time to do other stuff.
      sleepNanos(RowSleepNanos[b + (7 - PwmBits)]);
    }
  }
}


// Clear the entire display
void RgbMatrix::clearDisplay()
{
  memset(&_plane, 0, sizeof(_plane));
}


// Clear the inside of the given Rectangle. 
void RgbMatrix::clearRect(uint8_t fx, uint8_t fy, uint8_t fw, uint8_t fh)
{
  uint8_t maxX, maxY;
  maxX = (fx + fw) > Width ? Width : (fx + fw);
  maxY = (fy + fh) > Height ? Height : (fy + fh);

  for (int b = PwmBits - 1; b >= 0; b--)
  {
    for (int x = fx; x < maxX; x++)
    {
      for (int y = fy; y < maxY; y++)
      {
        GpioPins *bits = &_plane[b].row[y & 0xf].column[x];

        if (y < 16)
        {
          // Upper sub-panel
          bits->bits.r1 = 0;
          bits->bits.g1 = 0;
          bits->bits.b1 = 0;
        }
        else
        {
          // Lower sub-panel
          bits->bits.r2 = 0;
          bits->bits.g2 = 0;
          bits->bits.b2 = 0;
        }
      }
    }
  }
}


// Fade whatever is on the display to black.
void RgbMatrix::fadeDisplay()
{
  for (int b = PwmBits - 1; b >= 0; b--)
  {
    for (int x = 0; x < Width; x++)
    {
      for (int y = 0; y < Height; y++)
      {
        GpioPins *bits = &_plane[b].row[y & 0xf].column[x];

        if (y < 16)
        {
          // Upper sub-panel
	  bits->bits.r1 >>= 1;
          bits->bits.g1 >>= 1;
          bits->bits.b1 >>= 1;
        }
        else
        {
          // Lower sub-panel
          bits->bits.r2 >>= 1;
          bits->bits.g2 >>= 1;
          bits->bits.b2 >>= 1;
        }
      }
    }
    //TODO: make this dependent on PwmBits (longer sleep for fewer PwmBits).
    usleep(100000); // 1/10 second
  }
}

 
// Fade whatever is shown inside the given Rectangle. 
void RgbMatrix::fadeRect(uint8_t fx, uint8_t fy, uint8_t fw, uint8_t fh)
{
  uint8_t maxX, maxY;
  maxX = (fx + fw) > Width ? Width : (fx + fw);
  maxY = (fy + fh) > Height ? Height : (fy + fh);

  for (int b = PwmBits - 1; b >= 0; b--)
  {
    for (int x = fx; x < maxX; x++)
    {
      for (int y = fy; y < maxY; y++)
      {
        GpioPins *bits = &_plane[b].row[y & 0xf].column[x];

        if (y < 16)
        {
          // Upper sub-panel
	  bits->bits.r1 >>= 1;
          bits->bits.g1 >>= 1;
          bits->bits.b1 >>= 1;
        }
        else
        {
          // Lower sub-panel
          bits->bits.r2 >>= 1;
          bits->bits.g2 >>= 1;
          bits->bits.b2 >>= 1;
        }
      }
    }
    //TODO: make this param and/or dependent on PwmBits (longer sleep for fewer PwmBits).
    usleep(100000); // 1/10 second
  }
}


// Call this after drawing on the display and before calling fadeIn().
void RgbMatrix::setupFadeIn()
{
  // Copy the _plane and then set all bits to 0.
  memcpy(&_fadeInPlane, &_plane, sizeof(_plane));
  clearDisplay();
}


// Fade in whatever is stored in the _fadeInPlane.
void RgbMatrix::fadeIn()
{
  // Loop over copy of _plane and set bits in actual _plane.
  for (int b = 0; b < PwmBits; b++)
  {
    for (int x = 0; x < Width; x++)
    {
      for (int y = 0; y < Height; y++)
      {
        GpioPins *fiBits = &_fadeInPlane[b].row[y & 0xf].column[x];
        GpioPins *bits = &_plane[b].row[y & 0xf].column[x];

        if (y < 16)
        {
          // Upper sub-panel
	  bits->bits.r1 = fiBits->bits.r1;
          bits->bits.g1 = fiBits->bits.g1;
          bits->bits.b1 = fiBits->bits.b1;
        }
        else
        {
          // Lower sub-panel
          bits->bits.r2 = fiBits->bits.r2;
          bits->bits.g2 = fiBits->bits.g2;
          bits->bits.b2 = fiBits->bits.b2;
        }
      }
    }
    //TODO: make this a param and/or dependent on PwmBits (longer sleep for fewer PwmBits).
    usleep(100000); // 1/10 second
  }
}


// Wipe all pixels down off the screen
void RgbMatrix::wipeDown()
{
  for (int frame = 0; frame < Height; frame++)
  {
    //Each time through, clear the top row.
    for (int x = 0; x < Width; x++)
    {
      for (int b = PwmBits - 1; b >= 0; b--)
      {
        GpioPins *bits = &_plane[b].row[(frame) & 0xf].column[x];

        if (frame < 16)
        {
	  // Upper sub-panel
	  bits->bits.r1 = 0;
	  bits->bits.g1 = 0;
	  bits->bits.b1 = 0;
	}
	else
	{
	  // Lower sub-panel
	  bits->bits.r2 = 0;
	  bits->bits.g2 = 0;
	  bits->bits.b2 = 0;
	}
      }
    }

    for (int y = Height - 1; y > frame; y--)
    {
      for (int x = 0; x < Width; x++)
      {
	for (int b = PwmBits - 1; b >= 0; b--)
	{
	  GpioPins *prevBits = &_plane[b].row[(y-1) & 0xf].column[x];
	  GpioPins *currBits = &_plane[b].row[y & 0xf].column[x];

          if (y == 16) //Special case when we cross the panels
          {
            currBits->bits.r2 = prevBits->bits.r1;
            currBits->bits.g2 = prevBits->bits.g1;
            currBits->bits.b2 = prevBits->bits.b1;
          }
	  else if (y < 16)
	  {
	    // Upper sub-panel
	    currBits->bits.r1 = prevBits->bits.r1;
	    currBits->bits.g1 = prevBits->bits.g1;
	    currBits->bits.b1 = prevBits->bits.b1;
	  }
	  else
	  {
	    // Lower sub-panel
	    currBits->bits.r2 = prevBits->bits.r2;
	    currBits->bits.g2 = prevBits->bits.g2;
	    currBits->bits.b2 = prevBits->bits.b2;
	  }
	}
      }
    }

    //TODO: make this param and/or dependent on PwmBits (longer sleep for fewer PwmBits).
    usleep(25000);
  }
}


void RgbMatrix::drawPixel(uint8_t x, uint8_t y, Color color)
{
  if (x >= Width || y >= Height) return;

  // Four 32x32 panels would be connected like:  [>] [>]
  //                                             [<] [<]
  // Which would be 64 columns and 32 rows from L to R, then flipping backwards
  // for the next 32 rows (and 64 columns).
  if (y > 31)
  {
    x = 127 - x;
    y = 63 - y;
  }

  // Break out values from structure
  uint8_t red   = color.red;
  uint8_t green = color.green;
  uint8_t blue  = color.blue;

  //TODO: Adding Gamma correction slowed down the PWM and made
  //      the matrix flicker, so I'm removing it for now.

  // Gamma correct
  //red   = pgm_read_byte(&Gamma[red]);
  //green = pgm_read_byte(&Gamma[green]);
  //blue  = pgm_read_byte(&Gamma[blue]);

  // Scale to the number of bit planes, so MSB matches MSB of PWM.
  red   >>= 8 - PwmBits;
  green >>= 8 - PwmBits;
  blue  >>= 8 - PwmBits;

  // Set RGB bits for this pixel in each PWM bit plane.
  for (int b = 0; b < PwmBits; b++)
  {
    uint8_t mask = 1 << b;
    GpioPins *bits = &_plane[b].row[y & 0xf].column[x];

    if (y < 16)
    {
      // Upper sub-panel
      bits->bits.r1 = (red & mask) == mask;
      bits->bits.g1 = (green & mask) == mask;
      bits->bits.b1 = (blue & mask) == mask;
    }
    else
    {
      // Lower sub-panel
      bits->bits.r2 = (red & mask) == mask;
      bits->bits.g2 = (green & mask) == mask;
      bits->bits.b2 = (blue & mask) == mask;
    }
  }
}


// Bresenham's Line Algorithm
void RgbMatrix::drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,
                         Color color)
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
      drawPixel(y0, x0, color);
    }
    else
    {
      drawPixel(x0, y0, color);
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
void RgbMatrix::drawVLine(uint8_t x, uint8_t y, uint8_t h, Color color)
{
  drawLine(x, y, x, y + h - 1, color);
}


// Draw a horizontal line
void RgbMatrix::drawHLine(uint8_t x, uint8_t y, uint8_t w, Color color)
{
  drawLine(x, y, x + w - 1, y, color);
}


// Draw the outline of a rectangle (no fill)
void RgbMatrix::drawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, Color color)
{
  drawHLine(x, y, w, color);
  drawHLine(x, y + h - 1, w, color);
  drawVLine(x, y, h, color);
  drawVLine(x + w - 1, y, h, color);
}


void RgbMatrix::fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, Color color)
{
  for (uint8_t i = x; i < x + w; i++)
  {
    drawVLine(i, y, h, color);
  }
}


void RgbMatrix::fillScreen(Color color)
{
  fillRect(0, 0, Width, Height, color);
}


// Draw a rounded rectangle with radius r.
void RgbMatrix::drawRoundRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t r,
                              Color color)
{
  drawHLine(x + r    , y        , w - 2 * r, color);
  drawHLine(x + r    , y + h - 1, w - 2 * r, color);
  drawVLine(x        , y + r    , h - 2 * r, color);
  drawVLine(x + w - 1, y + r    , h - 2 * r, color);

  drawCircleQuadrant(x + r        , y + r        , r, 1, color);
  drawCircleQuadrant(x + w - r - 1, y + r        , r, 2, color);
  drawCircleQuadrant(x + w - r - 1, y + h - r - 1, r, 4, color);
  drawCircleQuadrant(x + r        , y + h - r - 1, r, 8, color);
}


void RgbMatrix::fillRoundRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t r,
                              Color color)
{
  fillRect(x + r, y, w - 2 * r, h, color);

  fillCircleHalf(x + r        , y + r, r, 1, h - 2 * r - 1, color);
  fillCircleHalf(x + w - r - 1, y + r, r, 2, h - 2 * r - 1, color);
}


// Draw the outline of a cirle (no fill) - Midpoint Circle Algorithm
void RgbMatrix::drawCircle(uint8_t x, uint8_t y, uint8_t r, Color color)
{
  int16_t f = 1 - r;
  int16_t ddFx = 1;
  int16_t ddFy = -2 * r;
  int16_t x1 = 0;
  int16_t y1 = r;

  drawPixel(x, y + r, color);
  drawPixel(x, y - r, color);
  drawPixel(x + r, y, color);
  drawPixel(x - r, y, color);

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

    drawPixel(x + x1, y + y1, color);
    drawPixel(x - x1, y + y1, color);
    drawPixel(x + x1, y - y1, color);
    drawPixel(x - x1, y - y1, color);
    drawPixel(x + y1, y + x1, color);
    drawPixel(x - y1, y + x1, color);
    drawPixel(x + y1, y - x1, color);
    drawPixel(x - y1, y - x1, color);
  }
}

// Draw one of the four quadrants of a circle.
void RgbMatrix::drawCircleQuadrant(uint8_t x, uint8_t y, uint8_t r, uint8_t quadrant,
                                   Color color)
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
      drawPixel(x - y1, y - x1, color);
      drawPixel(x - x1, y - y1, color);
    }

    //Upper Right
    if (quadrant & 0x2)
    {
      drawPixel(x + x1, y - y1, color);
      drawPixel(x + y1, y - x1, color);
    }

    //Lower Right
    if (quadrant & 0x4)
    {
      drawPixel(x + x1, y + y1, color);
      drawPixel(x + y1, y + x1, color);
    }

    //Lower Left
    if (quadrant & 0x8)
    {
      drawPixel(x - y1, y + x1, color);
      drawPixel(x - x1, y + y1, color);
    }
  }
}


void RgbMatrix::fillCircle(uint8_t x, uint8_t y, uint8_t r, Color color)
{
  drawVLine(x, y - r, 2 * r + 1, color);
  fillCircleHalf(x, y, r, 3, 0, color);
}


void RgbMatrix::fillCircleHalf(uint8_t x, uint8_t y, uint8_t r,
                               uint8_t half, uint8_t stretch,
                               Color color)
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
      drawVLine(x - x1, y - y1, 2 * y1 + 1 + stretch, color);
      drawVLine(x - y1, y - x1, 2 * x1 + 1 + stretch, color);
    }

    //Right
    if (half & 0x2)
    {
      drawVLine(x + x1, y - y1, 2 * y1 + 1 + stretch, color);
      drawVLine(x + y1, y - x1, 2 * x1 + 1 + stretch, color);
   }
  }
}

// Draw an Arc
void RgbMatrix::drawArc(uint8_t x, uint8_t y, uint8_t r,
                        float startAngle, float endAngle,
                        Color color)
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
    drawLine(prevX, prevY, x + r * cos(theta), y + r * sin(theta), color);

    prevX = x + r * cos(theta);
    prevY = y + r * sin(theta);
  }

  drawLine(prevX, prevY, x + r * cos(endAngle), y + r * sin(endAngle), color);
}


// Draw the outline of a wedge.
void RgbMatrix::drawWedge(uint8_t x, uint8_t y, uint8_t r,  //TODO: add inner radius
                          float startAngle, float endAngle,
                          Color color)
{
  // Convert degrees to radians
  float degreesPerRadian = M_PI / 180;

  float startAngleDeg = startAngle * degreesPerRadian;
  float endAngleDeg = endAngle * degreesPerRadian;

  uint8_t prevX = x + r * cos(startAngleDeg);
  uint8_t prevY = y + r * sin(startAngleDeg);

  //Special cases to overcome floating point limitations
  if (startAngle == 90 || startAngle == 270)
  {
    prevX = x;
  }
  else if (startAngle == 0 || startAngle == 180 || startAngle == 360)
  {
    prevY = y;
  }

  drawLine(x, y, prevX, prevY, color);

  drawArc(x, y, r, startAngle, endAngle, color);

  prevX = x + r * cos(endAngleDeg);
  prevY =  y + r * sin(endAngleDeg);

  //Special cases to overcome floating point limitations
  if (endAngle == 90 || endAngle == 270)
  {
    prevX = x;
  }
  else if (endAngle == 0 || endAngle == 180 || endAngle == 360)
  {
    prevY = y;
  }

  drawLine(prevX, prevY, x, y, color);
}


void RgbMatrix::drawTriangle(uint8_t x1, uint8_t y1,
                             uint8_t x2, uint8_t y2,
                             uint8_t x3, uint8_t y3,
                             Color color)
{
  drawLine(x1, y1, x2, y2, color);
  drawLine(x2, y2, x3, y3, color);
  drawLine(x3, y3, x1, y1, color);
}


void RgbMatrix::fillTriangle(uint8_t x1, uint8_t y1,
                             uint8_t x2, uint8_t y2,
                             uint8_t x3, uint8_t y3,
                             Color color)
{
  int16_t a, b, y, last;

  // Sort coordinates by Y order (y3 >= y2 >= y1)
  if (y1 > y2)
  {
    std::swap(y1, y2);
    std::swap(x1, x2);
  }

  if (y2 > y3)
  {
    std::swap(y3, y2);
    std::swap(x3, x2);
  }

  if (y1 > y2)
  {
    std::swap(y1, y2);
    std::swap(x1, x2);
  }

  // Handle case where all points are on the same line.
  if(y1 == y3)
  {
    a = b = x1;
    if(x2 < a)
      a = x2;
    else if(x2 > b)
      b = x2;
    if(x3 < a)
      a = x3;
    else if(x3 > b)
      b = x3;

    drawHLine(a, y1, b-a+1, color);
    return;
  }

  int16_t dx12 = x2 - x1,
          dy12 = y2 - y1,
          dx13 = x3 - x1,
          dy13 = y3 - y1,
          dx23 = x3 - x2,
          dy23 = y3 - y2,
          sa   = 0,
          sb   = 0;

  // For upper part of triangle, find scanline crossings for segments
  // 1-2 and 1-3.  If y2==y3 (flat-bottomed triangle), the scanline y2
  // is included here (and second loop will be skipped, avoiding a /0
  // error there), otherwise scanline y2 is skipped here and handled
  // in the second loop...which also avoids a /0 error here if y1=y2
  // (flat-topped triangle).
  if(y2 == y3)
    last = y2;   // Include y2 scanline
  else
    last = y2-1; // Skip it

  for(y = y1; y <= last; y++)
  {
    a   = x1 + sa / dy12;
    b   = x1 + sb / dy13;
    sa += dx12;
    sb += dx13;

    /* longhand:
    a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
    b = x1 + (x3 - x1) * (y - y1) / (y3 - y1);
    */

    if(a > b)
      std::swap(a,b);

    drawHLine(a, y, b-a+1, color);
  }

  // For lower part of triangle, find scanline crossings for segments
  // 1-3 and 2-3.  This loop is skipped if y2==y3.
  sa = dx23 * (y - y2);
  sb = dx12 * (y - y1);

  for(; y<=y3; y++)
  {
    a   = x2 + sa / dy23;
    b   = x1 + sb / dy13;
    sa += dx23;
    sb += dx13;

    /* longhand:
    a = x2 + (x3 - x2) * (y - y2) / (y3 - y2);
    b = x1 + (x3 - x1) * (y - y1) / (y3 - y1);
    */

    if(a > b)
      std::swap(a,b);

    drawHLine(a, y, b-a+1, color);
  }
}


// Special method to create a color wheel on the display.
void RgbMatrix::drawColorWheel()
{
  int x, y, hue;
  float dx, dy, d;
  uint8_t sat, val;

  if (Height != Width)
    fprintf(stderr, "Error: method drawColorWheel() only works when Height = Width.");
  
  float const Half = (Width - 1) / 2;

  Color color;

  for(y=0; y < Width; y++)
  {
    dy = Half - (float)y;

    for(x=0; x < Height; x++)
    {
      dx = Half - (float)x;
      d  = dx * dx + dy * dy;

      // In the circle...
      if(d <= ((Half+1) * (Half+1)))
      {
        hue = (int)((atan2(-dy, dx) + M_PI) * 1536.0 / (M_PI * 2.0));
        d = sqrt(d);

        if(d > Half)
        {
          // Do a little pseudo anti-aliasing along perimeter
          sat = 255;
          val = (int)((1.0 - (d - Half)) * 255.0 + 0.5);
        }
        else
        {
          // White at center
          sat = (int)(d / Half * 255.0 + 0.5);
          val = 255;
        }

        color = colorHSV(hue, sat, val);
      }
      else
      {
        color.red = 0;
        color.green = 0;
        color.blue = 0;
      }

      drawPixel(x, y, color);
    }
  }
}

void RgbMatrix::setTextCursor(uint8_t x, uint8_t y)
{
  _textCursorX = x;
  _textCursorY = y;
}

void RgbMatrix::setFontColor(Color color)
{
  _fontColor = color;
}

void RgbMatrix::setFontSize(uint8_t size)
{
  _fontSize = (size >= 3) ? 3 : size; //only 3 sizes for now

  if (_fontSize == 1)
  {
    _fontWidth = 3;
    _fontHeight = 5;
  }
  else if (_fontSize == 2) //medium (4x6)
  {
    _fontWidth = 4;
    _fontHeight = 6;
  }
  else if (_fontSize == 3) //large (5x7)
  {
    _fontWidth = 5;
    _fontHeight = 7;
  }
}

void RgbMatrix::setWordWrap(bool wrap)
{
  _wordWrap = wrap;
}

// Write a character using the Text cursor and stored Font settings.
void RgbMatrix::writeChar(unsigned char c)
{
  if (c == '\n')
  {
    _textCursorX = 0;
    _textCursorY += _fontHeight;
  }
  else if (c == '\r')
  {
    ; //ignore
  }
  else
  {
    putChar(_textCursorX, _textCursorY, c, _fontSize, _fontColor);

    _textCursorX += _fontWidth + 1;

    if (_wordWrap && (_textCursorX > (Width - _fontWidth)))
    {
      _textCursorX = 0;
      _textCursorY += _fontHeight + 1;
    }
  }
}

// Put a character on the display using glcd fonts.
void RgbMatrix::putChar(uint8_t x, uint8_t y, unsigned char c, uint8_t size,
                        Color color)
{
  unsigned char *font = Font5x7;
  uint8_t fontWidth = 5;
  uint8_t fontHeight = 7;

  if (size == 1) //small (3x5)
  {
    font = Font3x5;
    fontWidth = 3;
    fontHeight = 5;
  }
  else if (size == 2) //medium (4x6)
  {
    font = Font4x6;
    fontWidth = 4;
    fontHeight = 6;
  }
  else if (size == 3) //large (5x7)
  {
    ; //already initialized as default
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
      line = pgm_read_byte(font + ((c - 0x20) * fontWidth) + i);
    }

    for (int j=0; j < fontHeight+1; j++)
    {
      if (line & 0x1)
      {
        drawPixel(x+i, y+j, color);
      }

      line >>= 1;
    }
  }
}


//Leave output in 24-bit color (#RRGGBB)
Color RgbMatrix::colorHSV(long hue, uint8_t sat, uint8_t val)
{
  uint8_t r, g, b, lo;
  uint16_t s1, v1;

  // Hue
  hue %= 1536;             // -1535 to +1535
  if(hue < 0) hue += 1536; //     0 to +1535

  lo = hue & 255;  // Low byte = primary/secondary color mix

  // High byte = sextant of colorwheel
  switch(hue >> 8)
  {
    case 0 : r = 255;      g =  lo     ; b =   0;      break; // R to Y
    case 1 : r = 255 - lo; g = 255     ; b =   0;      break; // Y to G
    case 2 : r =   0;      g = 255     ; b =  lo;      break; // G to C
    case 3 : r =   0;      g = 255 - lo; b = 255;      break; // C to B
    case 4 : r =  lo;      g =   0     ; b = 255;      break; // B to M
    default: r = 255;      g =   0     ; b = 255 - lo; break; // M to R
  }

  // Saturation: add 1 so range is 1 to 256, which allows a bitwise right shift
  // on the result rather than a costly divide.
  s1 = sat + 1;

  r  = 255 - (((255 - r) * s1) >> 8);
  g  = 255 - (((255 - g) * s1) >> 8);
  b  = 255 - (((255 - b) * s1) >> 8);

  // Value (brightness): Add 1, similar to above.
  v1 = val + 1;

  Color c;
  c.red   = (r * v1) >> 8;
  c.green = (g * v1) >> 8;
  c.blue  = (b * v1) >> 8;

  return c;
}



