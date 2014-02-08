// Copyright (c) 2013 Matt Hill
// Use of this source code is governed by The MIT License
// that can be found in the LICENSE file.

#include "DisplayUpdater.h"
#include "RgbMatrix.h"
#include "RgbMatrixContainer.h"
#include "Thread.h"

#include <cstdlib>
#include <iostream>
#include <math.h>
#include <stdio.h>
#include <unistd.h>

#define _USE_MATH_DEFINES



//Display an animated Line
class RgbMatrixAnimatedLine : public RgbMatrixContainer
{
public:
  RgbMatrixAnimatedLine(RgbMatrix *m, float r) : RgbMatrixContainer(m)
  {
    rotation = r;
  }

  void run()
  {
    Color color;
    color.green = 255;

    Color black;

    const int midX = _matrix->Width / 2;
    const int midY = _matrix->Height / 2;

    const float degreesPerRadian = M_PI / 180;

    //0 (W)
    //90 (N)
    //180 (E)
    //270 (S)

    //TODO: handle all these angles: 
    //      22.5, 67.5, 112.5, 135, 157.5, 202.5, 225, 247.5, 292.5, 337.5, 360
    //float rotation = 45.0; //22.5;  //315;

    int steps = 32;
    int rotationSubtractX = midX;
    int rotationSubtractY = midY;
    int drawLineAddX = midX;
    int drawLineAddY = midY;

    if (rotation == 45.0)  //NW
    {
       steps = 64;
       drawLineAddX = 0;
       drawLineAddY = 0;
    }
    else if (rotation == 315.0)  //SW
    {
      //TODO: make this one look good...

       steps = 64;
       //drawLineAddX = 0;
       //drawLineAddY = 0;
    }



    while (!isDone())
    {
      for (int i=0; i < steps; i++)
      {
        int x1 = i;
        int y1 = 0;
        int x2 = i;
        int y2 = 31;

        float rx1, ry1, rx2, ry2;
        rotate(x1 - rotationSubtractX, y1 - rotationSubtractY, degreesPerRadian * rotation, &rx1, &ry1);
        rotate(x2 - rotationSubtractX, y2 - rotationSubtractY, degreesPerRadian * rotation, &rx2, &ry2);
/*
        std::cout << "---------------- i: " << i << " ----------------" << std::endl;

        std::cout << "(x1, y1):                 " << x1 << ", " << y1 << std::endl <<
                     "(x1 - midX, y1 - midY):   " << (x1 - midX) << ", " << (y1 - midY) << std::endl <<
                     "(rx1, ry1):               " << rx1 << ", " << ry1 << std::endl << std::endl;

        std::cout << "(x2, y2):                " << x2 << ", " << y2 << std::endl <<
                     "(x2 - midX, y2 - midY):  " << (x2 - midX) << ", " << (y2 - midY) << std::endl <<
                     "(rx2, ry2):              " << rx2 << ", " << ry2 << std::endl << std::endl;
*/

/*
        //Fade color...
        if (i == 0)
        {
          color.green = 255;
        }
        else if (i > 15 && (i % 2 == 0))
        {
          color.green = color.green / 1.4;
        }
*/
        _matrix->drawLine(rx1 + drawLineAddX, ry1 + drawLineAddY, rx2 + drawLineAddX, ry2 + drawLineAddY, color);

        //TODO: This will determine the speed...
        usleep(60000);

        _matrix->drawLine(rx1 + drawLineAddX, ry1 + drawLineAddY, rx2 + drawLineAddX, ry2 + drawLineAddY, black);  //clear the line
      }
    }
  }


private:

  void rotate(int x, int y, float angle, float *new_x, float *new_y)
  {
    //TODO: Add cases to handle floating point limitations?
    //Round
    *new_x = floorf((x * cos(angle) - y * sin(angle)) + 0.5);
    *new_y = floorf((x * sin(angle) + y * cos(angle)) + 0.5);
    //*new_x = x * cos(angle) - y * sin(angle);
    //*new_y = x * sin(angle) + y * cos(angle);
  }

  float rotation;


};




//-----------------------------------------------------------------------------
// Display a menu and allow choosing different angles.

RgbMatrix *m = NULL;

RgbMatrixContainer *display = NULL;
RgbMatrixContainer *updater = NULL;


void displayMenu()
{
  std::system("clear");
  printf("      |------------------------------------------------|\n");
  printf("      |     Select an option from the menu below:      |\n");
  printf("      |------------------------------------------------|\n");
  printf("      |      (1) North  360/0     90                   |\n");
  printf("      |                                                |\n");
  printf("      |      (2) NNW    337.5     67.5                 |\n");
  printf("      |      (3) NW     315.0     45.0                 |\n");
  printf("      |      (4) WNW    292.5     22.5                 |\n");
  printf("      |                                                |\n");
  printf("      |      (5) West   270      360/0                 |\n");
  printf("      |                                                |\n");
  printf("      |      (6) WSW    247.5    337.5                 |\n");
  printf("      |      (7) SW     225.0    315.0                 |\n");
  printf("      |      (8) SSW    202.5    292.5                 |\n");
  printf("      |                                                |\n");
  printf("      |      (9) South  180      270                   |\n");
  printf("      |                                                |\n");
  printf("      |      (0) EXIT                                  |\n");
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

  while (choice != '0' && choice != 'q' && choice != 'Q')
  {
    // Clear and refresh the display.
    m->clearDisplay();
    m->updateDisplay();

    displayMenu();

    choice = getchar();
    getchar(); //consume the \n

    switch (choice)
    {
      case '1':
        display = new RgbMatrixAnimatedLine(m, 90);
        break;

      case '2':
        display = new RgbMatrixAnimatedLine(m, 67.5);
        break;

      case '3':
        display = new RgbMatrixAnimatedLine(m, 45);
        break;

      case '4':
        display = new RgbMatrixAnimatedLine(m, 22.5);
        break;

      case '5':
        display = new RgbMatrixAnimatedLine(m, 0);
        break;

      case '6':
        display = new RgbMatrixAnimatedLine(m, 337.5);
        break;
  
      case '7':
        display = new RgbMatrixAnimatedLine(m, 315);
        break;

      case '8':
        display = new RgbMatrixAnimatedLine(m, 292.5);
        break;

      case '9':
        display = new RgbMatrixAnimatedLine(m, 270);
        break;
/*
      case '10':
        display = new RgbMatrixAnimatedLine(m, 202.5);
        break;

      case '11':
        display = new RgbMatrixAnimatedLine(m, 225.0);
        break;

      case '12':
        display = new RgbMatrixAnimatedLine(m, 247.5);
        break;

      case '13':
        display = new RgbMatrixAnimatedLine(m, 270.0);
        break;

      case '14':
        display = new RgbMatrixAnimatedLine(m, 292.5);
        break;

      case '15':
        display = new RgbMatrixAnimatedLine(m, 315.0);
        break;

      case '16':
        display = new RgbMatrixAnimatedLine(m, 337.5);
        break;
*/

      case '0':
      case 'q':
      case 'Q':
        printf("\n\nHave a nice day!\n\n");
        break;

      default:
        break;
    }

    updater = new DisplayUpdater(m);
    printf("\n\nRunning Demo...\n\n");
    runDemo();
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

