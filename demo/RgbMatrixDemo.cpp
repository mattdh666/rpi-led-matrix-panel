// Copyright (c) 2013 Matt Hill
// Use of this source code is governed by The MIT License
// that can be found in the LICENSE file.

#include "DisplayUpdater.h"
#include "RgbMatrix.h"
#include "RgbMatrixContainer.h"
#include "Thread.h"

#include <cstdlib>
#include <stdio.h>
#include <unistd.h>


// Below are several classes that demo the capabilities of the RgbMatrix class.


// Demo of the capabilities of the RgbMatrix class
class RgbMatrixDrawShapes1 : public RgbMatrixContainer
{
public:
  RgbMatrixDrawShapes1(RgbMatrix *m) : RgbMatrixContainer(m) {}

  void run()
  {
    Color red;
    red.red = 135;

    Color green;
    green.green = 135;

    Color blue;
    blue.blue = 80;

    _matrix->setTextCursor(1, 19);
    _matrix->setFontSize(2);
    _matrix->setFontColor(green);
    _matrix->writeLetter('S');
    _matrix->writeLetter('i');
    _matrix->writeLetter('m');
    _matrix->writeLetter('p');
    _matrix->writeLetter('l');
    _matrix->writeLetter('e');

    _matrix->setTextCursor(1, 26);
    _matrix->setFontColor(blue);
    _matrix->writeLetter('S');
    _matrix->writeLetter('h');
    _matrix->writeLetter('a');
    _matrix->writeLetter('p');
    _matrix->writeLetter('e');
    _matrix->writeLetter('s');

    _matrix->drawRect(1, 1, 6, 6, blue);

    _matrix->drawCircle(10, 7, 3, red);

    _matrix->drawTriangle(25, 1, 31, 8, 20, 8, green);
  }

};


// Demo of the capabilities of the RgbMatrix class
class RgbMatrixDrawShapes2 : public RgbMatrixContainer
{
public:
  RgbMatrixDrawShapes2(RgbMatrix *m) : RgbMatrixContainer(m) {}

  void run()
  {
    Color b1;
    b1.blue = 155;

    Color b2;
    b2.green = 17;
    b2.blue = 205;

    Color b3;
    b3.green = 17;
    b3.blue = 224;

    Color b4;
    b4.green = 17;
    b4.blue = 234;

    Color b5;
    b5.blue = 255;
    
    _matrix->drawRect(0, 0, 1, 32, b1);
    _matrix->drawRect(9, 15, 1, 17, b2);
    _matrix->drawRect(10, 15, 1, 17, b3);
    _matrix->drawRect(11, 15, 1, 17, b4);
    _matrix->drawRect(12, 15, 1, 17, b5);
    _matrix->drawRect(13, 15, 1, 17, b1);
    _matrix->drawRect(14, 15, 1, 17, b2);
    _matrix->drawRect(15, 15, 1, 17, b3);
    _matrix->drawRect(16, 15, 1, 17, b4);
    _matrix->drawRect(17, 15, 1, 17, b5);
  }

};



// Cycle through colors, and pulse the entire matrix the same color.
class RgbMatrixPulsePixels : public RgbMatrixContainer
{
public:
  RgbMatrixPulsePixels(RgbMatrix *m) : RgbMatrixContainer(m) {}

  void run()
  {
    uint32_t count = 0;
    
    while (!isDone())
    {
      count++;

      int color = (count >> 9) % 7;
      int value = count & 0xFF;

      if (count & 0x100) value = 255 - value;

      int r, g, b;

      switch (color)
      {
        case 0: r = value; g = b = 0; break;
        case 1: g = value; r = b = 0; break;
        case 2: b = value; r = g = 0; break;
        case 3: r = g = value; b = 0; break;
        case 4: r = b = value; g = 0; break;
        case 5: g = b = value; r = 0; break;
        default: r = g = b = value; break;
      }

      Color pulse;
      pulse.red = r;
      pulse.green = g;
      pulse.blue = b;

      _matrix->fillScreen(pulse);

      usleep(5000);
    }
  }

};



// Demo of the capabilities of the RgbMatrix class
class RgbMatrixPulsePixelsGradient : public RgbMatrixContainer
{
public:
  RgbMatrixPulsePixelsGradient(RgbMatrix *m) : RgbMatrixContainer(m) {}

  void run()
  {
    uint32_t count = 0;

    Color fish;
    fish.red = 255;

    _matrix->drawPixel(0, 0, fish);
    
    while (!isDone())
    {
      count++;

      int color = (count >> 9) % 7; //512 steps for each color (256 up / 256 down)
      int value = count & 0xFF;

      if (count & 0x100) value = 255 - value; // pulse down

      int r, g, b;

      switch (color)
      {
        case 0: r = value; g = b = 0; break;
        case 1: g = value; r = b = 0; break;
        case 2: b = value; r = g = 0; break;
        case 3: r = g = value; b = 0; break;
        case 4: r = b = value; g = 0; break;
        case 5: g = b = value; r = 0; break;
        default: r = g = b = value; break;
      }

      for (int i=0; i < 32; i++)
      {
        Color iColor;
        iColor.red   = (((i+1) * 8) > r) ? r : 0;
        iColor.green = (((i+1) * 8) > g) ? g : 0;
        iColor.blue  = (((i+1) * 8) > b) ? b : 0;

        //int numero = rand() % 32;
        //_matrix->drawRect(i, numero, 1, 31 - numero, iColor);

        _matrix->drawRect(0, i, 32, 1, iColor);
      }

      usleep(2500);
    }
  }

};



// Draw a color wheel on the matrix. 
class RgbMatrixColorWheel : public RgbMatrixContainer
{
public:
  RgbMatrixColorWheel(RgbMatrix *m) : RgbMatrixContainer(m) {}

  void run()
  {
    _matrix->drawColorWheel();
  }
};


//-----------------------------------------------------------------------------
// Display a menu and allow choosing different demos.

RgbMatrix *m = NULL;

RgbMatrixContainer *display = NULL;
RgbMatrixContainer *updater = NULL;


void displayMenu()
{
  std::system("clear");
  printf("      |------------------------------------------------|\n");
  printf("      |     Select an option from the menu below:      |\n");
  printf("      |------------------------------------------------|\n");
  printf("      |      (1) Pulse All Pixels                      |\n");
  printf("      |      (2) Pulse All Pixels with a Gradient      |\n");
  printf("      |      (3) Demo of Drawing Shapes                |\n");
  printf("      |      (4) Another Demo of Drawing Shapes        |\n");
  printf("      |      (5) Draw a Color Wheel                    |\n");
  printf("      |      (6) Quit                                  |\n");
  printf("      |------------------------------------------------|\n");
  printf("                   Your Choice: ");
}


void runDemo()
{
  updater->start(10);
  display->start();

  printf("Press <RETURN> when done viewing demo.\n");
  getchar();

  delete display;
  delete updater;

  display = NULL;
  updater = NULL;

  m->clearDisplay();
  m->updateDisplay();
}



int main(int argc, char *argv[])
{
  GpioProxy io;

 if (!io.initialize())
    return 1;

  m = new RgbMatrix(&io);

  char choice = '1';

  while (choice != '6' && choice != 'q' && choice != 'Q')
  {
    displayMenu();

    choice = getchar();
    getchar(); //consume the \n

    switch (choice)
    {
      case '1':
        display = new RgbMatrixPulsePixels(m);
        updater = new DisplayUpdater(m);
        printf("\n\nRunning Demo #1.\n\n");
        runDemo();
        break;

      case '2':
        display = new RgbMatrixPulsePixelsGradient(m);
        updater = new DisplayUpdater(m);
        printf("\n\nRunning Demo #2.\n\n");
        runDemo();
        break;

      case '3':
        display = new RgbMatrixDrawShapes1(m);
        updater = new DisplayUpdater(m);
        printf("\n\nRunning Demo #3.\n\n");
        runDemo();
        break;

      case '4':
        display = new RgbMatrixDrawShapes2(m);
        updater = new DisplayUpdater(m);
        printf("\n\nRunning Demo #4.\n\n");
        runDemo();  
        break;

      case '5':
        display = new RgbMatrixColorWheel(m);
        updater = new DisplayUpdater(m);
        printf("\n\nRunning Demo #5.\n\n");
        runDemo();
        break;

      case '6':
        printf("\n\nHave a nice day!\n\n");
        break;

      default:
        break;
    }
  }

  // Stop threads and wait for them to join.
  if (display) delete display;
  if (updater) delete updater;

  // Clear and refresh the display.
  m->clearDisplay();
  m->updateDisplay();

  delete m;

  return 0;
}

