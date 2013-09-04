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


// Draw some shapes on the RGB Matrix. 
class RgbMatrixDrawShapes1 : public RgbMatrixContainer
{
public:
  RgbMatrixDrawShapes1(RgbMatrix *m) : RgbMatrixContainer(m) {}

  void run()
  {
    Color red;
    red.red = 255;

    Color green;
    green.green = 255;

    Color blue;
    blue.blue = 255;

    Color purple;
    purple.red = 135;
    purple.blue = 255;

    Color yellow;
    yellow.red = 255;
    yellow.green = 255;

    Color turquoise;
    turquoise.green = 255;
    turquoise.blue = 255;

    // Draw some shapes
    _matrix->drawRect(1, 1, 8, 8, blue);

    _matrix->drawCircle(14, 5, 4, red);

    _matrix->drawTriangle(25, 1, 30, 8, 20, 8, green);

    _matrix->drawRect(1, 11, 9, 6, purple);

    _matrix->drawRoundRect(11, 11, 10, 6, 2, yellow);

    _matrix->drawWedge(30, 18, 8, 190, 270, turquoise);

    // Write some text
    _matrix->setTextCursor(1, 19);
    _matrix->setFontSize(2); // Medium (4x6) Font
    _matrix->setFontColor(green);
    _matrix->writeChar('S');
    _matrix->writeChar('i');
    _matrix->writeChar('m');
    _matrix->writeChar('p');
    _matrix->writeChar('l');
    _matrix->writeChar('e');

    _matrix->setTextCursor(1, 26);
    _matrix->setFontColor(blue);
    _matrix->writeChar('S');
    _matrix->writeChar('h');
    _matrix->writeChar('a');
    _matrix->writeChar('p');
    _matrix->writeChar('e');
    _matrix->writeChar('s');
 }

};


// Draw and fill some shapes on the RGB Matrix.
class RgbMatrixDrawShapes2 : public RgbMatrixContainer
{
public:
  RgbMatrixDrawShapes2(RgbMatrix *m) : RgbMatrixContainer(m) {}

  void run()
  {
    Color red;
    red.red = 255;

    Color green;
    green.green = 255;

    Color blue;
    blue.blue = 255;

    Color purple;
    purple.red = 135;
    purple.blue = 255;

    Color yellow;
    yellow.red = 255;
    yellow.green = 255;

    Color turquoise;
    turquoise.green = 255;
    turquoise.blue = 255;

    Color white;
    white.red = 255;
    white.green = 255;
    white.blue = 255;

    // Draw some shapes
    _matrix->drawRect(1, 1, 8, 8, blue);
    _matrix->fillRect(2, 2, 6, 6, red);

    _matrix->drawCircle(14, 5, 4, red);
    _matrix->fillCircle(14, 5, 3, blue);

    _matrix->drawTriangle(25, 1, 30, 8, 20, 8, green);
    _matrix->fillTriangle(25, 2, 29, 7, 21, 7, yellow);

    _matrix->fillRect(1, 11, 9, 6, purple);

    _matrix->drawRoundRect(11, 11, 10, 6, 2, yellow);
    _matrix->fillRoundRect(12, 12, 8, 4, 0, white);

    _matrix->fillCircleHalf(26, 14, 4, 1, 0, turquoise);
    _matrix->fillCircleHalf(26, 14, 4, 2, 0, purple);

    // Write some text
    _matrix->setTextCursor(1, 19);
    _matrix->setFontSize(2); // Medium (4x6) Font
    _matrix->setFontColor(green);
    _matrix->writeChar('F');
    _matrix->writeChar('i');
    _matrix->writeChar('l');
    _matrix->writeChar('l');
    _matrix->writeChar('e');
    _matrix->writeChar('d');

    _matrix->setTextCursor(1, 26);
    _matrix->setFontColor(blue);
    _matrix->writeChar('S');
    _matrix->writeChar('h');
    _matrix->writeChar('a');
    _matrix->writeChar('p');
    _matrix->writeChar('e');
    _matrix->writeChar('s');
  }

};


// Cycle through colors and pulse the entire matrix the same color.
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


// Cycle through colors and pulse the matrix in a gradient pattern.
class RgbMatrixPulsePixelsGradient : public RgbMatrixContainer
{
public:
  RgbMatrixPulsePixelsGradient(RgbMatrix *m) : RgbMatrixContainer(m) {}

  void run()
  {
    uint32_t count = 0;

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
  printf("      |      (1) Draw Simple Shapes                    |\n");
  printf("      |      (2) Draw and Fill Shapes                  |\n");
  printf("      |      (3) Pulse All Pixels                      |\n");
  printf("      |      (4) Pulse Pixels with a Gradient          |\n");
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
        display = new RgbMatrixDrawShapes1(m);
        updater = new DisplayUpdater(m);
        printf("\n\nRunning Demo #1.\n\n");
        runDemo();
        break;

      case '2':
        display = new RgbMatrixDrawShapes2(m);
        updater = new DisplayUpdater(m);
        printf("\n\nRunning Demo #2.\n\n");
        runDemo();
        break;

      case '3':
        display = new RgbMatrixPulsePixels(m);
        updater = new DisplayUpdater(m);
        printf("\n\nRunning Demo #3.\n\n");
        runDemo();
        break;

      case '4':
        display = new RgbMatrixPulsePixelsGradient(m);
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
      case 'q':
      case 'Q':
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

