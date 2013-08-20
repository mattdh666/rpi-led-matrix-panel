// Copyright (c) 2013 Matt Hill
// Use of this source code is governed by The MIT License
// that can be found in the LICENSE file.
//
// Base class to display something on the matrix.

#ifndef RPI_RGBMATRIXCONTAINER_H
#define RPI_RGBMATRIXCONTAINER_H

#include "RgbMatrix.h"
#include "Thread.h"


class RgbMatrixContainer : public Thread
{
public:
  RgbMatrixContainer(RgbMatrix *m) : _matrix(m) {}
  virtual ~RgbMatrixContainer() {}

protected:
  RgbMatrix *const _matrix;

};

#endif
