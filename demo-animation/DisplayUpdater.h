// Copyright (c) 2013 Matt Hill
// Use of this source code is governed by The MIT License
// that can be found in the LICENSE file.
//
// Special class to drive pixels to the display.
//
// Must have realtime priority to prevent jitter that will make PWM uneven.

#ifndef RPI_DISPLAYUPDATER_H
#define RPI_DISPLAYUPDATER_H

#include "RgbMatrixContainer.h"

class DisplayUpdater : public RgbMatrixContainer
{
public:
  DisplayUpdater(RgbMatrix *m) : RgbMatrixContainer(m) {}

  void run()
  {
    while (!isDone())
    {
      _matrix->updateDisplay();
    }
  }

};

#endif
