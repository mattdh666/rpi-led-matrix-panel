// Copyright (c) 2013 Matt Hill
// Use of this source code is govered by The MIT License
// that can be found in the LICENSE file.
//
// This code is based on an example found at:
//   http://elinux.org/Rpi_Datasheet_751_GPIO_Registers

#include "GpioProxy.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>


#define BCM2708_PERI_BASE 0x20000000  /* Hardware registers for peripherals start at this address */
#define GPIO_BASE (BCM2708_PERI_BASE + 0x200000)  /* Offset for the GPIO controller */

// Page and Block size are both 4kb
//#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(_gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(_gpio+((g)/10)) |=  (1<<(((g)%10)*3))
//#define SET_GPIO_ALT(g,a) *(_gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))


GpioProxy::GpioProxy() : _outputBits(0), _gpio(NULL)
{
}


bool GpioProxy::initialize()
{
  int mem_fd;
  if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0)
  {
    perror("Cannot open /dev/mem: \n");
    return false;
  }

  char *gpio_map = (char*) mmap(
           NULL,                    //Any adddress in our space will do
           BLOCK_SIZE,              //Map length
           PROT_READ | PROT_WRITE,  // Enable reading & writing to mapped memory
           MAP_SHARED,              //Shared with other processes
           mem_fd,                  //File to map
           GPIO_BASE                //Offset to GPIO peripheral
  );

  close(mem_fd); //No need to keep mem_fd open after mmap

  if (gpio_map == MAP_FAILED)
  {
    fprintf(stderr, "mmap error %ld\n", (long)gpio_map);
    return false;
  }

  _gpio = (volatile uint32_t *)gpio_map;

  return true;
}



uint32_t GpioProxy::setupOutputBits(uint32_t outputs)
{
  if (_gpio == NULL)
  {
    fprintf(stderr, "Attempt set Output bits wihtout first initializing the GPIO.\n");
    return 0;
  }

  // Make sure only available GPIO bits are used for output. 
  outputs &= GpioBits;

  _outputBits = outputs;

  for (uint32_t b = 0; b < 27; ++b)
  {
    if (outputs & (1 << b))
    {
      INP_GPIO(b);  // Must use INP_GPIO before using OUT_GPIO.
      OUT_GPIO(b);
    }
  }

  return _outputBits;
}

