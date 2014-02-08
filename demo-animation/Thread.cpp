// Copyright (c) 2013 Matt Hill
// Use of this source code is governed by The MIT License
// that can be found in the LICENSE file.

#include "Thread.h"

#include <assert.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>


Thread::Thread() : _thread(0), _status(Created)
{
  _threadDown = false;  //The thread has not crashed.
  _stopThread = false;  //The thread has not been signaled to stop.
}


Thread::~Thread()
{
  if (_status != Running) return;

  _status = Finished;

  int returnVal = pthread_join(_thread, NULL);

  if (returnVal != 0)
  {
    fprintf(stderr, "Error: %d %s\n", returnVal, strerror(returnVal));
  }
}


//------------------------------------------------------------------------------
// Create a thread and attach code to it.
void Thread::start(int priority)
{
  assert(_status != Running);
  
  int status = pthread_create(&_thread, NULL, &executeThread, this);

  // Allow realtime threads with high priority.
  if (priority > 0)
  {
    struct sched_param p;
    p.sched_priority = priority;
    pthread_setschedparam(_thread, SCHED_FIFO, &p);
  }

  if (status == 0)
    _status = Running;
  else
    _status = Invalid;

  return;
}


//------------------------------------------------------------------------------
// Static method to call the overriden run() method for 'this' object.
void *Thread::executeThread(void *i_thread)
{
  reinterpret_cast<Thread*>(i_thread)->run();
  reinterpret_cast<Thread*>(i_thread)->_status = Finished;
  return NULL;
}


//------------------------------------------------------------------------------
// Override in the derived class.
void Thread::run()
{
  fprintf(stderr, "Error: Thread:run() should be overriden in derived class.");
}


//------------------------------------------------------------------------------
// Signal a thread to stop.
void Thread::stop()
{
  _stopThread = true;
}


//------------------------------------------------------------------------------
// Terminate the thread.
void Thread::terminate(unsigned long i_return)
{
  _status = Finished;
  pthread_exit(&i_return);
}

