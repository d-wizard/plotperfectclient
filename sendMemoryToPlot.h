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
 */
#ifndef sendMemoryToPlot_h
#define sendMemoryToPlot_h

#include "plotMsgTypes.h"
#include "plotMsgPack.h"

//*****************************************************************************
// Constants
//*****************************************************************************
#define MAX_IP_ADDR_STRING_SIZE (50) // Enough for IPv6. If not enough for string names it can be increased
#define MAX_PLOT_CURVE_STRING_SIZE (50)


//*****************************************************************************
// Types
//*****************************************************************************
// tPlotMemory defines all the information about a segments of memory that will
// be used to generate a plot.
typedef struct
{
   char*          pc_memory;
   unsigned int   i_numSamples;
   ePlotDataTypes e_dataType;
   unsigned int   i_dataSizeBytes;
   ePlotDim       e_plotDim;
   PLOTTER_BOOL   b_interleaved;

   PLOTTER_BOOL   b_arrayOfStructs;
   unsigned int   i_bytesBetweenValues;
}tPlotMemory;

// tSendMemToPlot defines all the information about creating plot messages.
// This includes the information about the raw memory segments that contain
// the actual data to be plotted (tPlotMemory), and infomation such as the
// IP/port of the plotting GUI, Plot Name, Curve Name, circular buffer pointers
// (if used) and timer information (if the plot is to be updated on an interval).
typedef struct
{
   tPlotMemory t_plotMem; // Y axis for 1D, X & Y axis for 2D interleaved, X axis for 2D non-interleaved
   tPlotMemory t_plotMem_separateYAxis; // Y axis for 2D non-interleaved

   PLOTTER_BOOL b_useReadWriteIndex;
   unsigned int i_readIndex;
   unsigned int i_writeIndex;

   const char* pc_ipAddr;
   unsigned short s_ipPort;

   const char* pc_plotName;
   const char* pc_curveName;

   PLOTTER_BOOL b_closeSocketAfterSend;

   int i_tcpSocketFd;

   // Full copys of the strings. Usefull when using printf to generate string values.
   char ac_ipAddr[MAX_IP_ADDR_STRING_SIZE];
   char ac_plotName[MAX_PLOT_CURVE_STRING_SIZE];
   char ac_curveName[MAX_PLOT_CURVE_STRING_SIZE];

}tSendMemToPlot;

//*****************************************************************************
// Prototypes
//*****************************************************************************
#ifdef __cplusplus
extern "C" {
#endif

void sendMemoryToPlot_Init( tSendMemToPlot* _this,
                            const char* plotterIpAddr,
                            unsigned short plotterIpPort,
                            PLOTTER_BOOL useReadWriteIndex,
                            const char* plotName,
                            const char* curveName);

void sendMemoryToPlot(tSendMemToPlot* _this);

void sendMemoryToPlot_Create1D(tSendMemToPlot* _this);
void sendMemoryToPlot_Create2D(tSendMemToPlot* _this);
void sendMemoryToPlot_Create2D_Interleaved(tSendMemToPlot* _this);
void sendMemoryToPlot_Update1D(tSendMemToPlot* _this);
void sendMemoryToPlot_Update2D(tSendMemToPlot* _this);
void sendMemoryToPlot_Update2D_Interleaved(tSendMemToPlot* _this);
void sendMemoryToPlot_Interleaved1DPlots(tSendMemToPlot* xAxis, tSendMemToPlot* yAxis);

void plotMsgGroupStart();
void plotMsgGroupEnd();

#ifdef __cplusplus
}
#endif

#endif

