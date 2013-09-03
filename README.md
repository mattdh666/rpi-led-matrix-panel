Raspberry Pi Library for Controlling an RGB LED Matrix
------------------------------------------------------

This library enables drawing shapes and writing text on an RGB LED Matrix using Raspberry Pi GPIO.
I developed it using a Raspberry Pi Model B running Debian GNU/Linux 7.0 (wheezy) and a 32x32 RGB LED Matrix purchased from the excellent Adafruit.
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


Check out Adafruit's tutorial for more details:  http://learn.adafruit.com/32x16-32x32-rgb-led-matrix
 
*NOTE*: My LED panel has the Green and Blue pins reversed when compared to the mapping in Adafruit's tutorial, which is:

     GPIO 17            -->  R1 (LED 1: Red)
     GPIO 18 (PCM_CLK)  -->  G1 (LED 1: Green)
     GPIO 22            -->  B1 (LED 1: Blue)
     GPIO 23            -->  R2 (LED 2: Red)
     GPIO 24            -->  G2 (LED 2: Green)
     GPIO 25            -->  B2 (LED 2: Blue)


### Running

There are several examples in the demo directory. To run them, grab this repository and build the library by running make in the root of the repository.

	$ make

Once the library builds successfully, change to the 'demo' directory and run make again to build the demo.

	$ cd demo
	$ make

To run the examples, run the demo as the super user:

	$ sudo ./demo

You will be presented with a menu with several examples.

        |------------------------------------------------|
        |     Select an option from the menu below:      |
        |------------------------------------------------|
        |      (1) Draw Simple Shapes                    |
        |      (2) Draw and Fill Shapes                  |
        |      (3) Pulse All Pixels                      |
        |      (4) Pulse Pixels with a Gradient          |
        |      (5) Draw a Color Wheel                    |
        |      (6) Quit                                  |
        |------------------------------------------------|
                     Your Choice:

Choose an option and watch it go.


### Credits

Many thanks for the code snippets taken from:  https://github.com/hzeller/rpi-rgb-led-matrix


### License

Written by Matt Hill and released under an MIT License. See the LICENSE file for deatils.
