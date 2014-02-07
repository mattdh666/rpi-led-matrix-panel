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


//Display an animated Line
class RgbMatrixAnimatedLine : public RgbMatrixContainer
{
public:
  RgbMatrixAnimatedLine(RgbMatrix *m) : RgbMatrixContainer(m) {}

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
    float rotation = 22.5;  //315;

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


};




// Display an animated GIF
class RgbMatrixAnimatedGif : public RgbMatrixContainer
{
public:
  RgbMatrixAnimatedGif(RgbMatrix *m) : RgbMatrixContainer(m) {}

/*
  // Simple GIF loader.
  bool loadGif(const char *filename)
  {
    if (_image)
    {
      delete [] _image;
      _image = NULL;
    }

    FILE *f = fopen(filename, "r");
    if (f == NULL) return false;
    char header_buf[256];
    const char *line = ReadLine(f, header_buf, sizeof(header_buf));

#define EXIT_WITH_MSG(m) { fprintf(stderr, "%s: %s |%s", filename, m, line); \
     fclose(f); return false; }

    if (sscanf(line, "P6 ") == EOF)
      EXIT_WITH_MSG("Can only handle P6 as PPM type.");

    line = ReadLine(f, header_buf, sizeof(header_buf));

    if (!line || sscanf(line, "%d %d ", &width_, &height_) != 2)
      EXIT_WITH_MSG("Width/height expected");

    int value;
    line = ReadLine(f, header_buf, sizeof(header_buf));

    if (!line || sscanf(line, "%d ", &value) != 1 || value != 255)
      EXIT_WITH_MSG("Only 255 for maxval allowed.");

    const size_t pixel_count = width_ * height_;
    _image = new Pixel [ pixel_count ];
    assert(sizeof(Pixel) == 3);   // we make that assumption.

    if (fread(_image, sizeof(Pixel), pixel_count, f) != pixel_count)
    {
      line = "";
      EXIT_WITH_MSG("Not enough pixels read.");
    }

#undef EXIT_WITH_MSG

    fclose(f);
    fprintf(stderr, "Read image with %dx%d\n", width_, height_);
    _depth = 0;
    return true;
  }
*/



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


private:
  struct Pixel {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
  };

  // Read line, skip comments.
  char *readLine(FILE *f, char *buffer, size_t len)
  {
    char *result;
    do 
    {
      result = fgets(buffer, len, f);
    } while (result != NULL && result[0] == '#');

    return result;
  }

  const Pixel &getPixel(int x, int y) 
  {
    static Pixel dummy;
    if (x < 0 || x > width_ || y < 0 || y > height_) return dummy;
    return _image[x + width_ * y];
  }

  int width_;
  int height_;
  int _depth;

  Pixel *_image; //array of pixels...

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
  printf("      |      (5) Display an Animated Line              |\n");
  printf("      |      (6) Draw a Color Wheel                    |\n");
  printf("      |      (7) Quit                                  |\n");
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

  while (choice != '7' && choice != 'q' && choice != 'Q')
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
        display = new RgbMatrixAnimatedLine(m);
        //display = new RgbMatrixAnimatedGif(m);
        updater = new DisplayUpdater(m);
        printf("\n\nRunning Demo #5.\n\n");
        runDemo();  
        break;

      case '6':
        display = new RgbMatrixColorWheel(m);
        updater = new DisplayUpdater(m);
        printf("\n\nRunning Demo #6.\n\n");
        runDemo();
        break;

      case '7':
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

