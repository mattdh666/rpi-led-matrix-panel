// Copyright (c) 2013 Matt Hill
//

#include "thread.h"
#include "RgbMatrix.h"

#include <assert.h>
#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>

using std::min;
using std::max;

// Base-class for a Thread that does something with a matrix.
class RGBMatrixManipulator : public Thread {
public:
  RGBMatrixManipulator(RgbMatrix *m) : running_(true), matrix_(m) {}
  virtual ~RGBMatrixManipulator() { running_ = false; }

  // Run() implementation needs to check running_ regularly.

protected:
  volatile bool running_;  // TODO: use mutex, but this is good enough for now.
  RgbMatrix *const matrix_;
};

// Pump pixels to screen. Needs to be high priority real-time because jitter
// here will make the PWM uneven.
class DisplayUpdater : public RGBMatrixManipulator {
public:
  DisplayUpdater(RgbMatrix *m) : RGBMatrixManipulator(m) {}

  void Run() {
    while (running_) {
      matrix_->updateDisplay();
    }
  }
};


// -- The following are demo image generators.

// Simple generator that pulses through RGB and White.
class ColorPulseGenerator : public RGBMatrixManipulator
{
 public:
  ColorPulseGenerator(RgbMatrix *m) : RGBMatrixManipulator(m) {}

  void Run()
  {
    //const int width = matrix_->Width;
    //const int height = matrix_->Height;
    uint32_t count = 0;

    while (running_)
    {
      usleep(5000);
      ++count;
      int color = (count >> 9) % 6;
      int value = count & 0xFF;
      if (count & 0x100) value = 255 - value;
      int r, g, b;

      switch (color)
      {
        case 0: r = value; g = b = 0; break;
        case 1: r = g = value; b = 0; break;
        case 2: g = value; r = b = 0; break;
        case 3: g = b = value; r = 0; break;
        case 4: b = value; r = g = 0; break;
        default: r = g = b = value; break;
      }

      //matrix_->fillScreen(r, g, b);

      //matrix_->drawPixel(0, 0, r, g, b);
      //matrix_->drawPixel(15, 15, r, g, b);
      //matrix_->drawPixel(31, 31, r, g, b);

      //matrix_->fillCircleHalf(16, 16, 12, 1, 0, 0, 255, 0); //green - swell (was: 85, 255, 68)
      //matrix_->fillCircle(16, 16, 12, 85, 255, 68);
   
      matrix_->drawWedge(16, 16, 12, 135, 225, 0, 255, 0);
      matrix_->drawArc(16, 16, 8, 180, 270, 0, 0, 255);

      matrix_->drawLine(1, 16, 16, 16, 255, 255, 255);

      matrix_->fillCircle(16, 16, 1, 255, 0, 0); //red - buoy

      matrix_->putChar(9, 25, 'M', 1, 0, 0, 60);
      matrix_->putChar(13, 25, 'a', 1, 0, 0, 60);
      matrix_->putChar(17, 25, 't', 1, 0, 0, 60);
      matrix_->putChar(21, 25, 't', 1, 0, 0, 60);

      //matrix_->putChar(9, 25, 'M', 2, 0, 0, 60);
      //matrix_->putChar(15, 25, 'a', 2, 0, 0, 60);
      //matrix_->putChar(21, 25, 't', 2, 0, 0, 60);
      //matrix_->putChar(27, 25, 't', 2, 0, 0, 60);
 
      //matrix_->drawRect(6, 11, 21, 11, b, g, r);
      //matrix_->fillRect(7, 12, 19, 9, r, g, b);

      //matrix_->drawRoundRect(6, 11, 21, 11, 4, b, g, r);
      //matrix_->fillRoundRect(6, 11, 21, 11, 4, b, g, r);

      usleep(5000);

    }
  }
};

class SimpleSquare : public RGBMatrixManipulator {
public:
  SimpleSquare(RgbMatrix *m) : RGBMatrixManipulator(m) {}
  void Run() {
    const int width = matrix_->Width;
    const int height = matrix_->Height;
    // Diagonaly
    for (int x = 0; x < width; ++x) {
        matrix_->drawPixel(x, x, 255, 255, 255);
        matrix_->drawPixel(height -1 - x, x, 255, 0, 255);
    }
    for (int x = 0; x < width; ++x) {
      matrix_->drawPixel(x, 0, 255, 0, 0);
      matrix_->drawPixel(x, height - 1, 255, 255, 0);
    }
    for (int y = 0; y < height; ++y) {
      matrix_->drawPixel(0, y, 0, 0, 255);
      matrix_->drawPixel(width - 1, y, 0, 255, 0);
    }
  }
};

// Simple class that generates a rotating block on the screen.
class RotatingBlockGenerator : public RGBMatrixManipulator {
public:
  RotatingBlockGenerator(RgbMatrix *m) : RGBMatrixManipulator(m) {}

  uint8_t scale_col(int val, int lo, int hi) {
    if (val < lo) return 0;
    if (val > hi) return 255;
    return 255 * (val - lo) / (hi - lo);
  }

  void Run() {
    const int cent_x = matrix_->Width / 2;
    const int cent_y = matrix_->Height / 2;

    // The square to rotate (inner square + black frame) needs to cover the
    // whole area, even if diagnoal.
    const int rotate_square = min(matrix_->Width, matrix_->Height) * 1.41;
    const int min_rotate = cent_x - rotate_square / 2;
    const int max_rotate = cent_x + rotate_square / 2;

    // The square to display is within the visible area.
    const int display_square = min(matrix_->Width, matrix_->Height) * 0.7;
    const int min_display = cent_x - display_square / 2;
    const int max_display = cent_x + display_square / 2;

    const float deg_to_rad = 2 * 3.14159265 / 360;
    int rotation = 0;
    while (running_) {
      ++rotation;
      usleep(15 * 1000);
      rotation %= 360;
      for (int x = min_rotate; x < max_rotate; ++x) {
        for (int y = min_rotate; y < max_rotate; ++y) {
          float disp_x, disp_y;
          Rotate(x - cent_x, y - cent_y,
                 deg_to_rad * rotation, &disp_x, &disp_y);
          if (x >= min_display && x < max_display &&
              y >= min_display && y < max_display) { // within display square
            matrix_->drawPixel(disp_x + cent_x, disp_y + cent_y,
                              scale_col(x, min_display, max_display),
                              255 - scale_col(y, min_display, max_display),
                              scale_col(y, min_display, max_display));
          } else {
            // black frame.
            matrix_->drawPixel(disp_x + cent_x, disp_y + cent_y, 0, 0, 0);
          }
        }
      }
    }
  }

private:
  void Rotate(int x, int y, float angle,
              float *new_x, float *new_y) {
    *new_x = x * cosf(angle) - y * sinf(angle);
    *new_y = x * sinf(angle) + y * cosf(angle);
  }
};

class ImageScroller : public RGBMatrixManipulator {
public:
  ImageScroller(RgbMatrix *m)
    : RGBMatrixManipulator(m), image_(NULL), horizontal_position_(0) {
  }

  // _very_ simplified. Can only read binary P6 PPM. Expects newlines in headers
  // Not really robust. Use at your own risk :)
  bool LoadPPM(const char *filename) {
    if (image_) {
      delete [] image_;
      image_ = NULL;
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
    image_ = new Pixel [ pixel_count ];
    assert(sizeof(Pixel) == 3);   // we make that assumption.
    if (fread(image_, sizeof(Pixel), pixel_count, f) != pixel_count) {
      line = "";
      EXIT_WITH_MSG("Not enough pixels read.");
    }
#undef EXIT_WITH_MSG
    fclose(f);
    fprintf(stderr, "Read image with %dx%d\n", width_, height_);
    horizontal_position_ = 0;
    return true;
  }

  void Run() {
    const int screen_height = matrix_->Height;
    const int screen_width = matrix_->Width;
    while (running_) {
      if (image_ == NULL) {
        usleep(100 * 1000);
        continue;
      }
      usleep(30 * 1000);
      for (int x = 0; x < screen_width; ++x) {
        for (int y = 0; y < screen_height; ++y) {
          const Pixel &p = getPixel((horizontal_position_ + x) % width_, y);
          // Display upside down on my desk. Lets flip :)
          int disp_x = screen_width - x;
          int disp_y = screen_height - y;
          matrix_->drawPixel(disp_x, disp_y, p.red, p.green, p.blue);
        }
      }
      ++horizontal_position_;
    }
  }

private:
  struct Pixel {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
  };

  // Read line, skip comments.
  char *ReadLine(FILE *f, char *buffer, size_t len) {
    char *result;
    do {
      result = fgets(buffer, len, f);
    } while (result != NULL && result[0] == '#');
    return result;
  }

  const Pixel &getPixel(int x, int y) {
    static Pixel dummy;
    if (x < 0 || x > width_ || y < 0 || y > height_) return dummy;
    return image_[x + width_ * y];
  }

  int width_;
  int height_;
  Pixel *image_;
  uint32_t horizontal_position_;
};

int main(int argc, char *argv[]) {
  int demo = 0;
  if (argc > 1) {
    demo = atoi(argv[1]);
  }
  fprintf(stderr, "Using demo %d\n", demo);

  GpioProxy io;
  if (!io.initialize())
    return 1;

  RgbMatrix m(&io);
    
  RGBMatrixManipulator *image_gen = NULL;

  switch (demo)
  {
  case 0:
    image_gen = new RotatingBlockGenerator(&m);
    break;

  case 1:
    if (argc > 2) {
      ImageScroller *scroller = new ImageScroller(&m);
      if (!scroller->LoadPPM(argv[2]))
        return 1;
      image_gen = scroller;
    } else {
      fprintf(stderr, "Demo %d Requires PPM image as parameter", demo);
      return 1;
    }
    break;

  case 2:
    image_gen = new SimpleSquare(&m);
    break;

  default:
    image_gen = new ColorPulseGenerator(&m);
    break;
  }

  if (image_gen == NULL)
    return 1;

  RGBMatrixManipulator *updater = new DisplayUpdater(&m);
  updater->Start(10);  // high priority

  image_gen->Start();

  // Things are set up. Just wait for <RETURN> to be pressed.
  printf("Press <RETURN> to exit and reset LEDs\n");
  getchar();

  // Stopping threads and wait for them to join.
  delete image_gen;
  delete updater;

  // Final thing before exit: clear screen and update once, so that
  // we don't have random pixels burn
  m.clearDisplay();
  m.updateDisplay();

  return 0;
}
