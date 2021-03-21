/* Copyright 2017, 2021 Dan Williams. All Rights Reserved.
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


#include "smartPlotMessage.h"


/**************************************************************************
Function:     timePlot_1D

Description:  Adds a plot point with the current time as the Y Axis point.
              Time is Monotonic / Steady Clock time (i.e. time since boot).

              See smartPlot_1D Description in smartPlotMessage.h for more
              info about the input parameters.
*/
static inline void timePlot_1D(int plotSize, int updateSize, const char* plotName, const char* curveName)
{
   tSmartPlotTime nowTime;
   smartPlot_getTime(&nowTime);
   smartPlot_1D(&nowTime, E_TIME_STRUCT_AUTO, 1, plotSize, updateSize, plotName, curveName);
}

/**************************************************************************
Function:     timePlot_2D

Description:  Adds a plot point with the current time as the X Axis point and
              the value passed in as the Y Axis point.
              Time is Monotonic / Steady Clock time (i.e. time since boot).

              See smartPlot_2D Description in smartPlotMessage.h for more
              info about the input parameters.
*/
static inline void timePlot_2D(
   const void* inDataToPlotY,
   ePlotDataTypes inDataTypeY,
   int plotSize,
   int updateSize,
   const char* plotName,
   const char* curveName)
{
   tSmartPlotTime nowTime;
   smartPlot_getTime(&nowTime);
   smartPlot_2D(&nowTime, E_TIME_STRUCT_AUTO, inDataToPlotY, inDataTypeY, 1, plotSize, updateSize, plotName, curveName);
}

/**************************************************************************
Function:     timePlot_2D_Int

Description:  Adds a plot point with the current time as the X Axis point and
              the value passed in (signed 32 bit integer) as the Y Axis point.
              Time is Monotonic / Steady Clock time (i.e. time since boot).

              See smartPlot_2D Description in smartPlotMessage.h for more
              info about the input parameters.
*/
static inline void timePlot_2D_Int(
   long num,
   int plotSize,
   int updateSize,
   const char* plotName,
   const char* curveName)
{
   tSmartPlotTime nowTime;
   smartPlot_getTime(&nowTime);
   smartPlot_2D(&nowTime, E_TIME_STRUCT_AUTO, &num, E_INT_32, 1, plotSize, updateSize, plotName, curveName);
}


#endif /* TIMEPLOT_H_ */
