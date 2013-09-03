Raspberry Pi Library for Controlling an RGB LED Matrix
------------------------------------------------------

This library enables drawing shapes and writing text on an RGB LED Matrix using Raspberry Pi GPIO.

I developed this library using a 32x32 RGB LED Matrix I purchased from the excellent Adafruit:
  http://www.adafruit.com/products/607


### Wiring

Wire the Raspberry Pi GPIO pins to the 32x32 RGB LED Matrix as follows:

     GPIO 2 (SDA)       -->  OE (Output Enabled)
     GPIO 3 (SCL)       -->  CLK (Serial Clock)
     GPIO 4 (GPCLK0)    -->  LAT (Data Latch)
     GPIO 7 (CE1)       -->  A  --|
     GPIO 8 (CE0)       -->  B    |   Row
     GPIO 9 (MISO)      -->  C    | Address
     GPIO 10 (MOSI)     -->  D  --|
     GPIO 17            -->  R1 (LED 1: Red)
     GPIO 18 (PCM_CLK)  -->  B1 (LED 1: Blue)
     GPIO 22            -->  G1 (LED 1: Green)
     GPIO 23            -->  R2 (LED 2: Red)
     GPIO 24            -->  B2 (LED 2: Blue)
     GPIO 25            -->  G2 (LED 2: Green)

  // NOTE: My LED panel has the Green and Blue pins reversed whem compared to
  //       the original mapping I got from Adafruit's website:
     GPIO 17            -->  R1 (LED 1: Red)
     GPIO 18 (PCM_CLK)  -->  G1 (LED 1: Green)
     GPIO 22            -->  B1 (LED 1: Blue)
     GPIO 23            -->  R2 (LED 2: Red)
     GPIO 24            -->  G2 (LED 2: Green)
     GPIO 25            -->  B2 (LED 2: Blue)


Also, check out Adafruit's tutorial on the 32x32 RGB LED Matrix:
  http://learn.adafruit.com/32x16-32x32-rgb-led-matrix
 

### Running

TODO: How to run the code. Anything you might need to change.

Build:

	$ make

Run:

	$ sudo ./demo


### Caveats

TODO: Anything you should know about...


### Credits

Many code snippets taken from this repo:
  https://github.com/hzeller/rpi-rgb-led-matrix


### License

Written by Matt Hill. Raspberry Pi LED Matrix Panel library released under the MIT License.

