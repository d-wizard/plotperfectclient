/* Copyright 2017, 2021 - 2022, 2024 Dan Williams. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
 * to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#ifndef PLOTTHREADING_H_
#define PLOTTHREADING_H_
#include <stdlib.h>
#include "plotMsgTypes.h"

typedef void *(*plotThreading_threadCallback)(void *);

// Define whether pthreads are available based on platform
#ifndef PLOTTER_PTHREADS_AVAILABLE
   #ifdef __unix__
      #define PLOTTER_PTHREADS_AVAILABLE
   #endif
#endif

// Determine whether to use C++11 threads or pthreads
#if defined __cplusplus && !defined PLOTTER_FORCE_PTHREADS
   #ifndef PLOT_THREADING_USE_CPP11_TYPES
      #define PLOT_THREADING_USE_CPP11_TYPES
   #endif
#endif

// Struct that defines some settings for the Plotting Background Thread.
typedef struct
{
   unsigned int timeBetweenMs;
   int setPriorityPolicy;
   int priority;
   int policy;
}tPlotThreadParams;

#ifdef PLOT_THREADING_USE_CPP11_TYPES
   #ifndef __cplusplus
   #error "Can't use C++11 threading types if not compiling for C++"
   #endif

   // Use c++11 threads.
   // TODO: I couldn't get this to work with Visual Studio 2015 x64. More work is probably needed. Use at your own risk.
   #include <thread>
   #include <mutex>

   typedef std::recursive_mutex tPlotMutex;
   typedef std::thread tPlotThread;

   #define CREATE_PLOT_MUTEX(variableName) tPlotMutex variableName

   static inline tPlotThread* plotThreading_createNewThread(plotThreading_threadCallback threadCallback, unsigned int timeBetweenMs)
   {
      tPlotThreadParams* threadParams = new tPlotThreadParams;
      threadParams->timeBetweenMs = timeBetweenMs;
      threadParams->setPriorityPolicy = 0;

      return new tPlotThread(threadCallback, threadParams);
   }

   static inline tPlotThread* plotThreading_createNewThread_withPriorityPolicy(
         plotThreading_threadCallback threadCallback,
         unsigned int timeBetweenMs,
         int priority,
         int policy )
   {
      tPlotThreadParams* threadParams = new tPlotThreadParams;
      threadParams->timeBetweenMs = timeBetweenMs;
      threadParams->setPriorityPolicy = 1;
      threadParams->priority = priority;
      threadParams->policy = policy;

      return new tPlotThread(threadCallback, threadParams);
   }

   static inline void plotThreading_mutexLock(tPlotMutex* mutex)
   {
      mutex->lock();
   }
   static inline void plotThreading_mutexUnlock(tPlotMutex* mutex)
   {
      mutex->unlock();
   }
#else
   // Use pthreads.
   #include <assert.h>
   #include "pthread.h"
   #include "sched.h"
   typedef pthread_mutex_t tPlotMutex;
   typedef pthread_t tPlotThread;

   #define CREATE_PLOT_MUTEX(variableName) tPlotMutex variableName = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

   static inline tPlotThread* plotThreading_createNewThread(plotThreading_threadCallback threadCallback, unsigned int timeBetweenMs)
   {
      tPlotThread* newThread = (tPlotThread*)malloc(sizeof(tPlotThread));
      tPlotThreadParams* threadParams = (tPlotThreadParams*)malloc(sizeof(tPlotThreadParams));

      threadParams->timeBetweenMs = timeBetweenMs;
      threadParams->setPriorityPolicy = 0;

      if(newThread != NULL)
      {
         pthread_create(newThread, NULL, threadCallback, threadParams);
      }

      return newThread;
   }

   static inline tPlotThread* plotThreading_createNewThread_withPriorityPolicy(
         plotThreading_threadCallback threadCallback,
         unsigned int timeBetweenMs,
         int priority,
         int policy )
   {
      tPlotThread* newThread = (tPlotThread*)malloc(sizeof(tPlotThread));
      tPlotThreadParams* threadParams = (tPlotThreadParams*)malloc(sizeof(tPlotThreadParams));

      threadParams->timeBetweenMs = timeBetweenMs;
      threadParams->setPriorityPolicy = 1;
      threadParams->priority = priority;
      threadParams->policy = policy;

      if(newThread != NULL)
      {
         pthread_attr_t t_threadAttr;
         struct sched_param t_schedParam;

         assert(pthread_attr_init(&t_threadAttr) == 0);
         pthread_attr_setinheritsched(&t_threadAttr, PTHREAD_EXPLICIT_SCHED);
         t_schedParam.sched_priority = priority;
         pthread_attr_setschedpolicy(&t_threadAttr, policy);
         pthread_attr_setschedparam(&t_threadAttr, &t_schedParam);

         pthread_create(newThread, &t_threadAttr, threadCallback, threadParams);
      }

      return newThread;
   }

   static inline void plotThreading_mutexLock(tPlotMutex* mutex)
   {
      pthread_mutex_lock(mutex);
   }
   static inline void plotThreading_mutexUnlock(tPlotMutex* mutex)
   {
      pthread_mutex_unlock(mutex);
   }

#endif



#endif /* PLOTTHREADING_H_ */
