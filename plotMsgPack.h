/* Copyright 2017, 2021, 2025 Dan Williams. All Rights Reserved.
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
#ifndef plotMsgPack_h
#define plotMsgPack_h

#include "smartPlotMessage.h" // defines the ePlotDataTypes type
#include "plotMsgTypes.h" // defines ints/floats for current platform
#include <string.h>

#define PLOT_MSG_SIZE_TYPE PLOTTER_UINT_32

#if defined PLOTTER_WINDOWS_BUILD && !defined __cplusplus
   #ifndef inline
      #define inline __inline
   #endif
#endif


typedef enum
{
   E_PLOT_1D,
   E_PLOT_2D
}ePlotDim;

typedef enum
{
   E_MULPITLE_PLOTS      = (int)0x4D8828E3,
   E_CREATE_1D_PLOT      = (int)0xF29E92F3,
   E_CREATE_2D_PLOT      = (int)0x7A123F89,
   E_UPDATE_1D_PLOT      = (int)0xF1331DFF,
   E_UPDATE_2D_PLOT      = (int)0x0FAF479C,
   E_INVALID_PLOT_ACTION = (int)0x079C7B2C
}ePlotAction;

static inline int isPlotDataTypeValid(ePlotDataTypes plotDataType)
{
   return (plotDataType >= 0 && plotDataType < E_INVALID_DATA_TYPE);
}

static const int PLOT_DATA_TYPE_SIZES[]=
{
   sizeof(PLOTTER_INT_8),
   sizeof(PLOTTER_UINT_8),
   sizeof(PLOTTER_INT_16),
   sizeof(PLOTTER_UINT_16),
   sizeof(PLOTTER_INT_32),
   sizeof(PLOTTER_UINT_32),
   sizeof(PLOTTER_INT_64),
   sizeof(PLOTTER_UINT_64),
   sizeof(PLOTTER_FLOAT_32),
   sizeof(PLOTTER_FLOAT_64),
   sizeof(PLOTTER_UINT_32) + sizeof(PLOTTER_UINT_32), // E_TIME_STRUCT_64
   sizeof(PLOTTER_UINT_64) + sizeof(PLOTTER_UINT_64),  // E_TIME_STRUCT_128
   sizeof(PLOTTER_UINT_16) // E_FLOAT_16
};


typedef struct
{
   const char*     plotName;
   const char*     curveName;
   PLOTTER_UINT_32 numSamp;
   ePlotDataTypes  yAxisType;
}t1dPlot;

typedef struct
{
   const char*     plotName;
   const char*     curveName;
   PLOTTER_UINT_32 numSamp;
   ePlotDataTypes  xAxisType;
   ePlotDataTypes  yAxisType;
   char            interleaved; // Boolean
}t2dPlot;

static inline PLOT_MSG_SIZE_TYPE getCreatePlot1dMsgSize(const t1dPlot* param)
{
   PLOT_MSG_SIZE_TYPE totalMsgSize =
      sizeof(ePlotAction) +
      sizeof(PLOT_MSG_SIZE_TYPE) +
      (unsigned int)strlen(param->plotName) + 1 +
      (unsigned int)strlen(param->curveName) + 1 +
      sizeof(param->numSamp) +
      sizeof(param->yAxisType) +
      (unsigned int)(param->numSamp*PLOT_DATA_TYPE_SIZES[param->yAxisType]);
   return totalMsgSize;
}

static inline PLOT_MSG_SIZE_TYPE getCreatePlot2dMsgSize(const t2dPlot* param)
{
   PLOT_MSG_SIZE_TYPE totalMsgSize =
      sizeof(ePlotAction) +
      sizeof(PLOT_MSG_SIZE_TYPE) +
      (unsigned int)strlen(param->plotName) + 1 +
      (unsigned int)strlen(param->curveName) + 1 +
      sizeof(param->numSamp) +
      sizeof(param->xAxisType) +
      sizeof(param->yAxisType) +
      sizeof(param->interleaved) +
      (unsigned int)(param->numSamp*PLOT_DATA_TYPE_SIZES[param->xAxisType]) +
      (unsigned int)(param->numSamp*PLOT_DATA_TYPE_SIZES[param->yAxisType]);
   return totalMsgSize;
}

static inline PLOT_MSG_SIZE_TYPE getUpdatePlot1dMsgSize(const t1dPlot* param)
{
   // 1 more UINT_32 parameter for update message
   return getCreatePlot1dMsgSize(param) + sizeof(PLOTTER_UINT_32);
}

static inline PLOT_MSG_SIZE_TYPE getUpdatePlot2dMsgSize(const t2dPlot* param)
{
   // 1 more UINT_32 parameter for update message
   return getCreatePlot2dMsgSize(param) + sizeof(PLOTTER_UINT_32);
}

static inline void packPlotMsgParam(char* baseWritePtr, unsigned int* idx, const void* srcPtr, unsigned int copySize)
{
   memcpy(&baseWritePtr[*idx], srcPtr, copySize);
   (*idx) += copySize;
}


static inline void packCreate1dPlotMsg(const t1dPlot* param, void* yPoints, char* packedMsg)
{
   PLOT_MSG_SIZE_TYPE totalMsgSize = 0;
   ePlotAction plotAction = E_CREATE_1D_PLOT;
   unsigned int idx = 0;

   totalMsgSize = getCreatePlot1dMsgSize(param);

   packPlotMsgParam(packedMsg, &idx, &plotAction, sizeof(plotAction));
   packPlotMsgParam(packedMsg, &idx, &totalMsgSize, sizeof(totalMsgSize));
   packPlotMsgParam(packedMsg, &idx, param->plotName, (unsigned int)strlen(param->plotName)+1);
   packPlotMsgParam(packedMsg, &idx, param->curveName, (unsigned int)strlen(param->curveName)+1);
   packPlotMsgParam(packedMsg, &idx, &param->numSamp, sizeof(param->numSamp));
   packPlotMsgParam(packedMsg, &idx, &param->yAxisType, sizeof(param->yAxisType));
   packPlotMsgParam(packedMsg, &idx, yPoints, (param->numSamp*PLOT_DATA_TYPE_SIZES[param->yAxisType]));
}

static inline unsigned int packCreate1dPlotMsg_withoutData(const t1dPlot* param, char* packedMsg)
{
   PLOT_MSG_SIZE_TYPE totalMsgSize = 0;
   ePlotAction plotAction = E_CREATE_1D_PLOT;
   unsigned int idx = 0;

   totalMsgSize = getCreatePlot1dMsgSize(param);

   packPlotMsgParam(packedMsg, &idx, &plotAction, sizeof(plotAction));
   packPlotMsgParam(packedMsg, &idx, &totalMsgSize, sizeof(totalMsgSize));
   packPlotMsgParam(packedMsg, &idx, param->plotName, (unsigned int)strlen(param->plotName)+1);
   packPlotMsgParam(packedMsg, &idx, param->curveName, (unsigned int)strlen(param->curveName)+1);
   packPlotMsgParam(packedMsg, &idx, &param->numSamp, sizeof(param->numSamp));
   packPlotMsgParam(packedMsg, &idx, &param->yAxisType, sizeof(param->yAxisType));

   return idx; // This is where the data can be copied to after this function.
}

static inline void packCreate2dPlotMsg(const t2dPlot* param, void* xPoints, void* yPoints, char* packedMsg)
{
   PLOT_MSG_SIZE_TYPE totalMsgSize = 0;
   ePlotAction plotAction = E_CREATE_2D_PLOT;
   unsigned int idx = 0;
   char interleaved = 0; // 0 = Not Interleaved

   totalMsgSize = getCreatePlot2dMsgSize(param);

   packPlotMsgParam(packedMsg, &idx, &plotAction, sizeof(plotAction));
   packPlotMsgParam(packedMsg, &idx, &totalMsgSize, sizeof(totalMsgSize));
   packPlotMsgParam(packedMsg, &idx, param->plotName, (unsigned int)strlen(param->plotName)+1);
   packPlotMsgParam(packedMsg, &idx, param->curveName, (unsigned int)strlen(param->curveName)+1);
   packPlotMsgParam(packedMsg, &idx, &param->numSamp, sizeof(param->numSamp));
   packPlotMsgParam(packedMsg, &idx, &param->xAxisType, sizeof(param->xAxisType));
   packPlotMsgParam(packedMsg, &idx, &param->yAxisType, sizeof(param->yAxisType));
   packPlotMsgParam(packedMsg, &idx, &interleaved, sizeof(param->interleaved));
   packPlotMsgParam(packedMsg, &idx, xPoints, (param->numSamp*PLOT_DATA_TYPE_SIZES[param->xAxisType]));
   packPlotMsgParam(packedMsg, &idx, yPoints, (param->numSamp*PLOT_DATA_TYPE_SIZES[param->yAxisType]));
}

static inline unsigned int packCreate2dPlotMsg_withoutData(const t2dPlot* param, char* packedMsg)
{
   PLOT_MSG_SIZE_TYPE totalMsgSize = 0;
   ePlotAction plotAction = E_CREATE_2D_PLOT;
   unsigned int idx = 0;
   char interleaved = 0; // 0 = Not Interleaved

   totalMsgSize = getCreatePlot2dMsgSize(param);

   packPlotMsgParam(packedMsg, &idx, &plotAction, sizeof(plotAction));
   packPlotMsgParam(packedMsg, &idx, &totalMsgSize, sizeof(totalMsgSize));
   packPlotMsgParam(packedMsg, &idx, param->plotName, (unsigned int)strlen(param->plotName)+1);
   packPlotMsgParam(packedMsg, &idx, param->curveName, (unsigned int)strlen(param->curveName)+1);
   packPlotMsgParam(packedMsg, &idx, &param->numSamp, sizeof(param->numSamp));
   packPlotMsgParam(packedMsg, &idx, &param->xAxisType, sizeof(param->xAxisType));
   packPlotMsgParam(packedMsg, &idx, &param->yAxisType, sizeof(param->yAxisType));
   packPlotMsgParam(packedMsg, &idx, &interleaved, sizeof(param->interleaved));

   return idx; // This is where the data can be copied to after this function.
}

static inline void packCreate2dPlotMsg_Interleaved(const t2dPlot* param, void* xyPoints, char* packedMsg)
{
   PLOT_MSG_SIZE_TYPE totalMsgSize = 0;
   ePlotAction plotAction = E_CREATE_2D_PLOT;
   unsigned int idx = 0;
   char interleaved = 1; // 1 = Interleaved

   totalMsgSize = getCreatePlot2dMsgSize(param);

   packPlotMsgParam(packedMsg, &idx, &plotAction, sizeof(plotAction));
   packPlotMsgParam(packedMsg, &idx, &totalMsgSize, sizeof(totalMsgSize));
   packPlotMsgParam(packedMsg, &idx, param->plotName, (unsigned int)strlen(param->plotName)+1);
   packPlotMsgParam(packedMsg, &idx, param->curveName, (unsigned int)strlen(param->curveName)+1);
   packPlotMsgParam(packedMsg, &idx, &param->numSamp, sizeof(param->numSamp));
   packPlotMsgParam(packedMsg, &idx, &param->xAxisType, sizeof(param->xAxisType));
   packPlotMsgParam(packedMsg, &idx, &param->yAxisType, sizeof(param->yAxisType));
   packPlotMsgParam(packedMsg, &idx, &interleaved, sizeof(param->interleaved));
   packPlotMsgParam(packedMsg, &idx, xyPoints,
      (param->numSamp*PLOT_DATA_TYPE_SIZES[param->xAxisType]) +
      (param->numSamp*PLOT_DATA_TYPE_SIZES[param->yAxisType]) );
}

static inline unsigned int packCreate2dPlotMsg_Interleaved_withoutData(const t2dPlot* param, char* packedMsg)
{
   PLOT_MSG_SIZE_TYPE totalMsgSize = 0;
   ePlotAction plotAction = E_CREATE_2D_PLOT;
   unsigned int idx = 0;
   char interleaved = 1; // 1 = Interleaved

   totalMsgSize = getCreatePlot2dMsgSize(param);

   packPlotMsgParam(packedMsg, &idx, &plotAction, sizeof(plotAction));
   packPlotMsgParam(packedMsg, &idx, &totalMsgSize, sizeof(totalMsgSize));
   packPlotMsgParam(packedMsg, &idx, param->plotName, (unsigned int)strlen(param->plotName)+1);
   packPlotMsgParam(packedMsg, &idx, param->curveName, (unsigned int)strlen(param->curveName)+1);
   packPlotMsgParam(packedMsg, &idx, &param->numSamp, sizeof(param->numSamp));
   packPlotMsgParam(packedMsg, &idx, &param->xAxisType, sizeof(param->xAxisType));
   packPlotMsgParam(packedMsg, &idx, &param->yAxisType, sizeof(param->yAxisType));
   packPlotMsgParam(packedMsg, &idx, &interleaved, sizeof(param->interleaved));

   return idx;
}

static inline void packUpdate1dPlotMsg(const t1dPlot* param, PLOTTER_UINT_32 sampleStartIndex, void* yPoints, char* packedMsg)
{
   PLOT_MSG_SIZE_TYPE totalMsgSize = 0;
   ePlotAction plotAction = E_UPDATE_1D_PLOT;
   unsigned int idx = 0;

   totalMsgSize = getUpdatePlot1dMsgSize(param);

   packPlotMsgParam(packedMsg, &idx, &plotAction, sizeof(plotAction));
   packPlotMsgParam(packedMsg, &idx, &totalMsgSize, sizeof(totalMsgSize));
   packPlotMsgParam(packedMsg, &idx, param->plotName, (unsigned int)strlen(param->plotName)+1);
   packPlotMsgParam(packedMsg, &idx, param->curveName, (unsigned int)strlen(param->curveName)+1);
   packPlotMsgParam(packedMsg, &idx, &param->numSamp, sizeof(param->numSamp));
   packPlotMsgParam(packedMsg, &idx, &sampleStartIndex, sizeof(sampleStartIndex));
   packPlotMsgParam(packedMsg, &idx, &param->yAxisType, sizeof(param->yAxisType));
   packPlotMsgParam(packedMsg, &idx, yPoints, (param->numSamp*PLOT_DATA_TYPE_SIZES[param->yAxisType]));
}

static inline unsigned int packUpdate1dPlotMsg_withoutData(const t1dPlot* param, PLOTTER_UINT_32 sampleStartIndex, char* packedMsg)
{
   PLOT_MSG_SIZE_TYPE totalMsgSize = 0;
   ePlotAction plotAction = E_UPDATE_1D_PLOT;
   unsigned int idx = 0;

   totalMsgSize = getUpdatePlot1dMsgSize(param);

   packPlotMsgParam(packedMsg, &idx, &plotAction, sizeof(plotAction));
   packPlotMsgParam(packedMsg, &idx, &totalMsgSize, sizeof(totalMsgSize));
   packPlotMsgParam(packedMsg, &idx, param->plotName, (unsigned int)strlen(param->plotName)+1);
   packPlotMsgParam(packedMsg, &idx, param->curveName, (unsigned int)strlen(param->curveName)+1);
   packPlotMsgParam(packedMsg, &idx, &param->numSamp, sizeof(param->numSamp));
   packPlotMsgParam(packedMsg, &idx, &sampleStartIndex, sizeof(sampleStartIndex));
   packPlotMsgParam(packedMsg, &idx, &param->yAxisType, sizeof(param->yAxisType));

   return idx;
}

static inline void packUpdate2dPlotMsg(const t2dPlot* param, PLOTTER_UINT_32 sampleStartIndex, void* xPoints, void* yPoints, char* packedMsg)
{
   PLOT_MSG_SIZE_TYPE totalMsgSize = 0;
   ePlotAction plotAction = E_UPDATE_2D_PLOT;
   unsigned int idx = 0;
   char interleaved = 0; // 0 = Not Interleaved

   totalMsgSize = getUpdatePlot2dMsgSize(param);

   packPlotMsgParam(packedMsg, &idx, &plotAction, sizeof(plotAction));
   packPlotMsgParam(packedMsg, &idx, &totalMsgSize, sizeof(totalMsgSize));
   packPlotMsgParam(packedMsg, &idx, param->plotName, (unsigned int)strlen(param->plotName)+1);
   packPlotMsgParam(packedMsg, &idx, param->curveName, (unsigned int)strlen(param->curveName)+1);
   packPlotMsgParam(packedMsg, &idx, &param->numSamp, sizeof(param->numSamp));
   packPlotMsgParam(packedMsg, &idx, &sampleStartIndex, sizeof(sampleStartIndex));
   packPlotMsgParam(packedMsg, &idx, &param->xAxisType, sizeof(param->xAxisType));
   packPlotMsgParam(packedMsg, &idx, &param->yAxisType, sizeof(param->yAxisType));
   packPlotMsgParam(packedMsg, &idx, &interleaved, sizeof(param->interleaved));
   packPlotMsgParam(packedMsg, &idx, xPoints, (param->numSamp*PLOT_DATA_TYPE_SIZES[param->xAxisType]));
   packPlotMsgParam(packedMsg, &idx, yPoints, (param->numSamp*PLOT_DATA_TYPE_SIZES[param->yAxisType]));
}

static inline unsigned int packUpdate2dPlotMsg_withoutData(const t2dPlot* param, PLOTTER_UINT_32 sampleStartIndex, char* packedMsg)
{
   PLOT_MSG_SIZE_TYPE totalMsgSize = 0;
   ePlotAction plotAction = E_UPDATE_2D_PLOT;
   unsigned int idx = 0;
   char interleaved = 0; // 0 = Not Interleaved

   totalMsgSize = getUpdatePlot2dMsgSize(param);

   packPlotMsgParam(packedMsg, &idx, &plotAction, sizeof(plotAction));
   packPlotMsgParam(packedMsg, &idx, &totalMsgSize, sizeof(totalMsgSize));
   packPlotMsgParam(packedMsg, &idx, param->plotName, (unsigned int)strlen(param->plotName)+1);
   packPlotMsgParam(packedMsg, &idx, param->curveName, (unsigned int)strlen(param->curveName)+1);
   packPlotMsgParam(packedMsg, &idx, &param->numSamp, sizeof(param->numSamp));
   packPlotMsgParam(packedMsg, &idx, &sampleStartIndex, sizeof(sampleStartIndex));
   packPlotMsgParam(packedMsg, &idx, &param->xAxisType, sizeof(param->xAxisType));
   packPlotMsgParam(packedMsg, &idx, &param->yAxisType, sizeof(param->yAxisType));
   packPlotMsgParam(packedMsg, &idx, &interleaved, sizeof(param->interleaved));

   return idx;
}

static inline void packUpdate2dPlotMsg_Interleaved(const t2dPlot* param, PLOTTER_UINT_32 sampleStartIndex, void* xyPoints, char* packedMsg)
{
   PLOT_MSG_SIZE_TYPE totalMsgSize = 0;
   ePlotAction plotAction = E_UPDATE_2D_PLOT;
   unsigned int idx = 0;
   char interleaved = 1; // 1 = Interleaved

   totalMsgSize = getUpdatePlot2dMsgSize(param);

   packPlotMsgParam(packedMsg, &idx, &plotAction, sizeof(plotAction));
   packPlotMsgParam(packedMsg, &idx, &totalMsgSize, sizeof(totalMsgSize));
   packPlotMsgParam(packedMsg, &idx, param->plotName, (unsigned int)strlen(param->plotName)+1);
   packPlotMsgParam(packedMsg, &idx, param->curveName, (unsigned int)strlen(param->curveName)+1);
   packPlotMsgParam(packedMsg, &idx, &param->numSamp, sizeof(param->numSamp));
   packPlotMsgParam(packedMsg, &idx, &sampleStartIndex, sizeof(sampleStartIndex));
   packPlotMsgParam(packedMsg, &idx, &param->xAxisType, sizeof(param->xAxisType));
   packPlotMsgParam(packedMsg, &idx, &param->yAxisType, sizeof(param->yAxisType));
   packPlotMsgParam(packedMsg, &idx, &interleaved, sizeof(param->interleaved));
   packPlotMsgParam(packedMsg, &idx, xyPoints,
      (param->numSamp*PLOT_DATA_TYPE_SIZES[param->xAxisType]) +
      (param->numSamp*PLOT_DATA_TYPE_SIZES[param->yAxisType]) );
}

static inline unsigned int packUpdate2dPlotMsg_Interleaved_withoutData(const t2dPlot* param, PLOTTER_UINT_32 sampleStartIndex, char* packedMsg)
{
   PLOT_MSG_SIZE_TYPE totalMsgSize = 0;
   ePlotAction plotAction = E_UPDATE_2D_PLOT;
   unsigned int idx = 0;
   char interleaved = 1; // 1 = Interleaved

   totalMsgSize = getUpdatePlot2dMsgSize(param);

   packPlotMsgParam(packedMsg, &idx, &plotAction, sizeof(plotAction));
   packPlotMsgParam(packedMsg, &idx, &totalMsgSize, sizeof(totalMsgSize));
   packPlotMsgParam(packedMsg, &idx, param->plotName, (unsigned int)strlen(param->plotName)+1);
   packPlotMsgParam(packedMsg, &idx, param->curveName, (unsigned int)strlen(param->curveName)+1);
   packPlotMsgParam(packedMsg, &idx, &param->numSamp, sizeof(param->numSamp));
   packPlotMsgParam(packedMsg, &idx, &sampleStartIndex, sizeof(sampleStartIndex));
   packPlotMsgParam(packedMsg, &idx, &param->xAxisType, sizeof(param->xAxisType));
   packPlotMsgParam(packedMsg, &idx, &param->yAxisType, sizeof(param->yAxisType));
   packPlotMsgParam(packedMsg, &idx, &interleaved, sizeof(param->interleaved));

   return idx;
}
#endif

