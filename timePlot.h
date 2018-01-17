/* Copyright 2017 Dan Williams. All Rights Reserved.
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
 *
 * timePlot.h
 *
 *  Created on: May 15, 2015
 *      Author: d
 */
#ifndef TIMEPLOT_H_
#define TIMEPLOT_H_

#if (defined(_WIN32) || defined(__WIN32__))
   #include "winTimeSpec.h"
#else
   #include <time.h>
#endif

#include "smartPlotMessage.h"


static inline void timePlot_init(const char* plotName, const char* curveName, int plotSize)
{
   ePlotDataTypes timeStructType = sizeof(struct timespec) <= 8 ? E_TIME_STRUCT_64 : E_TIME_STRUCT_128; // Determine if timespec is 8 bytes or 16 bytes.
   smartPlot_1D(NULL, timeStructType, 0, plotSize, -1, plotName, curveName);
}

static inline void timePlot_getTime(const char* plotName, const char* curveName, int updateSize)
{
   struct timespec nowTime;
   clock_gettime(CLOCK_MONOTONIC, &nowTime);
   smartPlot_1D(&nowTime, (ePlotDataTypes)-1, 1, 0, updateSize, plotName, curveName);
}

static inline void timePlot_update(const char* plotName, const char* curveName, int updateSize)
{
   smartPlot_1D(NULL, (ePlotDataTypes)-1, 0, 0, updateSize, plotName, curveName);
}

static inline void timePlot_plotAll(const char* plotName, const char* curveName)
{
   smartPlot_1D(NULL, (ePlotDataTypes)-1, 0, 0, 0, plotName, curveName);
}



#endif /* TIMEPLOT_H_ */
