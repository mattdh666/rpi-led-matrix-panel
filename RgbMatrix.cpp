// Copyright (c) 2013 Matt Hill
// Use of this source code is governed by The MIT License
// that can be found in the LICENSE file.

// This class is for controlling a 32x32 RGB LED Matrix panel using
// the Raspberry Pi GPIO.

// This code is based on an example found at:
//   https://github.com/hzeller/rpi-rgb-led-matrix

#include "RgbMatrix.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


// Clocking in a row takes about 3.4usec (TODO: this is actually per board)
// Because clocking the data in is part of the 'wait time', we need to
// substract that from the row sleep time.
static const int kRowClockTime = 3400;

const long row_sleep_nanos[8] = {   // Only using the first PwmResolution elements.
  (1 * kRowClockTime) - kRowClockTime,
  (2 * kRowClockTime) - kRowClockTime,
  (4 * kRowClockTime) - kRowClockTime,
  (8 * kRowClockTime) - kRowClockTime,
  (16 * kRowClockTime) - kRowClockTime,
  (32 * kRowClockTime) - kRowClockTime,
  (64 * kRowClockTime) - kRowClockTime,
  // Too much flicker with 8 bits. We should have a separate screen pass
  // with this bit plane. Or interlace. Or trick with -OE switch on in the
  // middle of row-clocking, thus have kRowClockTime / 2
  (128 * kRowClockTime) - kRowClockTime, // too much flicker.
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

  //TODO: Why do I need an whole "GpioPins" for these bits?
  GpioPins clock, output_enable, strobe;

  clock.bits.clock = 1;
  output_enable.bits.outputEnabled = 1;
  strobe.bits.latch = 1;

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

      _gpio->setBits(output_enable.raw);  // switch off while strobe.

      row_bits.bits.rowAddress = row;
      _gpio->setBits(row_bits.raw & row_mask.raw);
      _gpio->clearBits(~row_bits.raw & row_mask.raw);

      _gpio->setBits(strobe.raw);   // Strobe
      _gpio->clearBits(strobe.raw);

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


void RgbMatrix::fillScreen(uint8_t red, uint8_t green, uint8_t blue)
{
  for (int x = 0; x < Width; ++x)
  {
    for (int y = 0; y < Height; ++y)
    {
      drawPixel(x, y, red, green, blue);
    }
  }
}


// Draw the outline of a cirle (no fill)
void RgbMatrix::drawCircle(uint8_t x, uint8_t y, uint8_t r,
                           uint8_t red, uint8_t green, uint8_t blue)
{
  int16_t f = 1 - r;
  int16_t ddFx = 1;
  int16_t ddFy = -2 * r;
  int16_t x1 = 0;
  int16_t y1 = r;

  drawPixel(x, y+r, red, green, blue);
  drawPixel(x, y-r, red, green, blue);
  drawPixel(x+r, y, red, green, blue);
  drawPixel(x-r, y, red, green, blue);

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

