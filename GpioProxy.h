// Copyright (c) 2013 Matt Hill
// Use of this source code is governed by The MIT License
// that can be found in the LICENSE file.
//
// This class handles communication with the GPIO (General Purpose Input/Output)
// pins on a Raspberry Pi. This is this pin layout on a RPi v2:
//
//      01: 3.3V Power              02: 5V Power
//      03: GPIO 2 (SDA)            04: 5V Power
//      05: GPIO 3 (SCL)            06: Ground
//      07: GPIO 4 (GPCLK0)         08: GPIO 14 (TXD)
//      09: Ground                  10: GPIO 15 (RXD)
//      11: GPIO 17                 12: GPIO 18 (PCM_CLK)
//      13: GPIO 27                 14: Ground
//      15: GPIO 22                 16: GPIO 23
//      17: 3.3V Power              18: GPIO 24
//      19: GPIO 10 (MOSI)          20: Ground
//      21: GPIO 9 (MISO)           22: GPIO 25
//      23: GPIO 11 (SCLK)          24: GPIO 8 (CE0)
//      25: Ground                  26: GPIO 7 (CE1)
//
// NOTE: For this project (controlling RGB LED Matrix Panels), only Output is used.
//       Will need to develop the Input (read) functions when required.

#ifndef RPI_GPIO_PROXY_H
#define RPI_GPIO_PROXY_H

#include <stdint.h>


class GpioProxy
{
public:

  // Bits with GPIO Pins
  static const uint32_t GpioBits =
    ((1 <<  2) | (1 <<  3) | (1 <<  4) | (1 <<  7) | (1 << 8) | (1 <<  9) |
     (1 << 10) | (1 << 11) | (1 << 14) | (1 << 15) | (1 <<17) | (1 << 18) |
     (1 << 22) | (1 << 23) | (1 << 24) | (1 << 25) | (1 << 27));


  GpioProxy();

  bool initialize();

  // Tries to setup bits for output. Returns bits that are ready for output.
  uint32_t setupOutputBits(uint32_t outputBits);

  // Sets bits which are 1. Ignores bits which are 0.
  //  Converted from Macro: #define GPIO_SET *(gpio+7) 
  inline void setBits(uint32_t value)
  {
    *(_gpio+7) = value;
  }

  // Clears bits which are 1. Ignores bits which are 0.
  //  Converted from Macro: #define GPIO_CLR *(gpio+10)
  inline void clearBits(uint32_t value)
  {
    *(_gpio+10) = value;
  }

/*
  inline void write(uint32_t value)
  {
    //TODO: Might need to sleep between the two.
    setBits(value & _outputBits);
    clearBits(~value & _outputBits);
  }
*/


private:

  uint32_t _outputBits;
  volatile uint32_t *_gpio;

};

#endif
