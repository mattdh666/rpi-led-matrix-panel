// Copyright (c) 2013 Matt Hill
// Use of this source code is governed by The MIT License
// that can be found in the LICENSE file.
//
// Very basic abstract class for creating a POSIX thread on a Raspberry Pi.
//
// Class was developed using Wheezy (Linux).

#ifndef RPI_THREAD_H
#define RPI_THREAD_H

#include <pthread.h>


class Thread
{
public:

  enum threadStatus { Created, Running, Finished, Invalid };

  Thread();
  virtual ~Thread();

  // Start the thread and execute the run() method.
  // Priority >0 will run thread as realtime.
  void start(int priority = 0);

  // Signal a thread to stop.
  void stop();
 
  inline threadStatus getStatus() const { return _status; }

  bool isDone() const;

   
protected:

  // Check if thread was signaled to stop.
  inline bool shouldStop() { return _stopThread; }
  
  // Force a thread down.
  void terminate(unsigned long i_return);


private:

  static void *executeThread(void *tobject);

  // Thread worker method. Override in derived class.
  virtual void run();

  pthread_t             _thread;  
  volatile threadStatus _status;
  bool                  _stopThread;
  bool                  _threadDown;

};


inline bool Thread::isDone() const
{
  if (_status == Created || _status == Running)
  {
    return false;
  }
  else  // Finished || Invalid
  {
    return true;
  }
}

#endif
