/* Copyright 2017, 2024 Dan Williams. All Rights Reserved.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "sendMemoryToPlot.h"
#include "plotThreading.h"
#include "sendTCPPacket.h"

//*****************************************************************************
// Types
//*****************************************************************************
// Callback function to a specific function to plot the data, can be used to skip a few if checks
// each time a plot message is being generated and sent.
typedef void (*tPlotMsgCallback)(tSendMemToPlot*);


//*****************************************************************************
// Constants
//*****************************************************************************
#define GROUP_MSG_HEADER_SIZE (8)


//*****************************************************************************
// Local Variables
//*****************************************************************************
static int g_groupPlotMsgs = 0;
static char* g_groupPlot_memory = NULL;
static int g_groupPlot_curSize = 0;
static tSendMemToPlot* g_groupSendMemToPlotPtr = NULL;
static CREATE_PLOT_MUTEX(gt_sendMemToPlot_mutex);


//*****************************************************************************
// Macros
//*****************************************************************************
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

#define strToArray(dstArray, srcStrPtr) \
   memset(dstArray, 0, sizeof(dstArray)); \
   memcpy(dstArray, srcStrPtr, MIN( strlen(srcStrPtr), sizeof(dstArray) - 1 ));


//*****************************************************************************
// Local Function Prototypes
//*****************************************************************************
static int sendPlotPacket(tSendMemToPlot* _this, const char* msg, unsigned int msgSize, int isGroupFinalMsg);
static tPlotMsgCallback determinePlotMsgGenCallback(tSendMemToPlot* _this);


//*****************************************************************************
// Functions
//*****************************************************************************
void plotMsgGroupStart()
{
   plotThreading_mutexLock(&gt_sendMemToPlot_mutex);
   g_groupPlotMsgs = 1;
}

void plotMsgGroupEnd()
{
   if(g_groupPlotMsgs)
   {
      if(g_groupPlot_memory != NULL)
      {
         unsigned int plotMsgId = E_MULPITLE_PLOTS;
         unsigned int plotMsgSize = GROUP_MSG_HEADER_SIZE + g_groupPlot_curSize;

         // Error Checking.
         assert(g_groupPlot_curSize > 0);
         assert(g_groupSendMemToPlotPtr != NULL);

         // Pack the Header.
         memcpy(&g_groupPlot_memory[0], &plotMsgId, 4);
         memcpy(&g_groupPlot_memory[4], &plotMsgSize, 4);

         // Send the message to the plotter.
         sendPlotPacket(g_groupSendMemToPlotPtr, g_groupPlot_memory, plotMsgSize, 1);
         free(g_groupPlot_memory);
      }

      g_groupPlot_memory = NULL;
      g_groupPlot_curSize = 0;
      g_groupSendMemToPlotPtr = NULL;
      g_groupPlotMsgs = 0;
      plotThreading_mutexUnlock(&gt_sendMemToPlot_mutex);
   }
}

static void plotMsgGroupAdd(tSendMemToPlot* _this, const char* msg, unsigned int size)
{
   char* newGroupMsgMem = NULL;
   ePlotAction readPlotActionType = E_INVALID_PLOT_ACTION;

   // Check if this is the first message being added to a group message.
   if(g_groupPlot_memory == NULL)
   {
      // Error Checking.
      assert(g_groupPlot_curSize == 0);
      assert(g_groupSendMemToPlotPtr == NULL);

      g_groupSendMemToPlotPtr = _this; // Just use the first messages parameters.
   }

   // If the message passed in is already a group message, strip the
   // group message header from the message.
   memcpy(&readPlotActionType, msg, sizeof(readPlotActionType));
   if(readPlotActionType == E_MULPITLE_PLOTS)
   {
      msg += GROUP_MSG_HEADER_SIZE;
      size -= GROUP_MSG_HEADER_SIZE;
   }

   // Allocate a buffer that can hold all the old messages and the new one.
   newGroupMsgMem = (char*)malloc(GROUP_MSG_HEADER_SIZE + g_groupPlot_curSize + size);

   // Move the message from the old buffer to the new buffer (if there was an old buffer).
   if(g_groupPlot_memory != NULL)
   {
      memcpy(newGroupMsgMem+GROUP_MSG_HEADER_SIZE, g_groupPlot_memory+GROUP_MSG_HEADER_SIZE, g_groupPlot_curSize);
      free(g_groupPlot_memory);
   }

   // Copy the new message into the group plot memory.
   g_groupPlot_memory = newGroupMsgMem;
   memcpy(g_groupPlot_memory+GROUP_MSG_HEADER_SIZE+g_groupPlot_curSize, msg, size);

   g_groupPlot_curSize += size;
}

static inline void packArrayOfStructs( void* dest,
                                       void* srcArray,
                                       unsigned int numSamples,
                                       unsigned int dataSize,
                                       unsigned int bytesBetweenValues )
{
   char* pc_dst = (char*)dest;
   char* pc_src = ((char*)srcArray);
   unsigned int packIndex = 0;

   // copy the data into the message
   for(packIndex = 0; packIndex < numSamples; ++packIndex)
   {
      memcpy( &pc_dst[packIndex*dataSize],
              &pc_src[bytesBetweenValues*packIndex],
              dataSize);
   }
}

static tPlotMsgCallback determinePlotMsgGenCallback(tSendMemToPlot* _this)
{
   tPlotMsgCallback callback = sendMemoryToPlot_Create1D;

   if(_this->t_plotMem.e_plotDim == E_PLOT_1D)
   {
      if(!_this->b_useReadWriteIndex)
      {
         callback = sendMemoryToPlot_Create1D;
      }
      else
      {
         callback = sendMemoryToPlot_Update1D;
      }
   }
   else
   {
      if(!_this->b_useReadWriteIndex)
      {
         if(_this->t_plotMem.b_interleaved)
         {
            callback = sendMemoryToPlot_Create2D_Interleaved;
         }
         else
         {
            callback = sendMemoryToPlot_Create2D;
         }
      }
      else
      {
         if(_this->t_plotMem.b_interleaved)
         {
            callback = sendMemoryToPlot_Update2D_Interleaved;
         }
         else
         {
            callback = sendMemoryToPlot_Update2D;
         }
      }
   }

   return callback;
}

void sendMemoryToPlot_Init( tSendMemToPlot* _this,
                            const char* plotterIpAddr,
                            unsigned short plotterIpPort,
                            PLOTTER_BOOL useReadWriteIndex,
                            const char* plotName,
                            const char* curveName)
{
   // TODO: Lots of input checking. For now, better send the correct inputs
   _this->b_useReadWriteIndex = useReadWriteIndex;
   _this->i_readIndex = 0;
   _this->i_writeIndex = 0;
   _this->b_closeSocketAfterSend = FALSE;
   _this->i_tcpSocketFd = 0; // Initialize to invalid value.

   if(plotterIpAddr != NULL)
   {
      strToArray(_this->ac_ipAddr, plotterIpAddr);
      _this->pc_ipAddr = _this->ac_ipAddr;
   }

   if(plotterIpPort != 0)
   {
      _this->s_ipPort = plotterIpPort;
   }

   if(plotName != NULL)
   {
      strToArray(_this->ac_plotName, plotName);
      _this->pc_plotName = _this->ac_plotName;
   }

   if(curveName != NULL)
   {
      strToArray(_this->ac_curveName, curveName);
      _this->pc_curveName = _this->ac_curveName;
   }

}

void sendMemoryToPlot(tSendMemToPlot* _this)
{
   (determinePlotMsgGenCallback(_this))(_this);
}

void sendMemoryToPlot_Create1D(tSendMemToPlot* _this)
{
   char* msg = NULL;
   t1dPlot plot;
   unsigned int plotMsgSize = 0;

   plot.curveName = _this->pc_curveName;
   plot.plotName = _this->pc_plotName;
   plot.yAxisType = _this->t_plotMem.e_dataType;
   plot.numSamp = _this->t_plotMem.i_numSamples;

   plotMsgSize = getCreatePlot1dMsgSize(&plot);
   msg = (char*)malloc(plotMsgSize);
   if(NULL == msg)
      return;

   if(!_this->t_plotMem.b_arrayOfStructs)
   {
      packCreate1dPlotMsg(&plot, _this->t_plotMem.pc_memory, msg);
   }
   else
   {
      unsigned int dataStartIndex = 0;

      dataStartIndex = packCreate1dPlotMsg_withoutData(&plot, msg);

      packArrayOfStructs( msg+dataStartIndex,
                          _this->t_plotMem.pc_memory,
                          plot.numSamp,
                          _this->t_plotMem.i_dataSizeBytes,
                          _this->t_plotMem.i_bytesBetweenValues );
   }

   sendPlotPacket(_this, msg, plotMsgSize, 0);

   free(msg);
}


void sendMemoryToPlot_Create2D(tSendMemToPlot* _this)
{
   char* msg = NULL;
   t2dPlot plot;
   unsigned int plotMsgSize = 0;

   plot.curveName = _this->pc_curveName;
   plot.plotName = _this->pc_plotName;
   plot.numSamp = _this->t_plotMem.i_numSamples;
   plot.xAxisType = _this->t_plotMem.e_dataType;
   plot.yAxisType = _this->t_plotMem_separateYAxis.e_dataType;

   plotMsgSize = getCreatePlot2dMsgSize(&plot);
   msg = (char*)malloc(plotMsgSize);
   if(NULL == msg)
      return;

   if(!_this->t_plotMem.b_arrayOfStructs && !_this->t_plotMem_separateYAxis.b_arrayOfStructs)
   {
      packCreate2dPlotMsg(&plot, _this->t_plotMem.pc_memory, _this->t_plotMem_separateYAxis.pc_memory, msg);
   }
   else
   {
      unsigned int dataStartIndex = 0;
      dataStartIndex = packCreate2dPlotMsg_withoutData(&plot, msg);

      packArrayOfStructs( msg+dataStartIndex,
                          _this->t_plotMem.pc_memory,
                          plot.numSamp,
                          _this->t_plotMem.i_dataSizeBytes,
                          _this->t_plotMem.i_bytesBetweenValues );

      packArrayOfStructs( msg+dataStartIndex + (PLOT_DATA_TYPE_SIZES[_this->t_plotMem.e_dataType]*plot.numSamp),
                          _this->t_plotMem_separateYAxis.pc_memory,
                          plot.numSamp,
                          _this->t_plotMem_separateYAxis.i_dataSizeBytes,
                          _this->t_plotMem_separateYAxis.i_bytesBetweenValues );
   }

   sendPlotPacket(_this, msg, plotMsgSize, 0);

   free(msg);
}

void sendMemoryToPlot_Create2D_Interleaved(tSendMemToPlot* _this)
{
   char* msg = NULL;
   t2dPlot plot;
   unsigned int plotMsgSize = 0;

   plot.curveName = _this->pc_curveName;
   plot.plotName = _this->pc_plotName;
   plot.numSamp = _this->t_plotMem.i_numSamples;
   plot.xAxisType = _this->t_plotMem.e_dataType;
   plot.yAxisType = _this->t_plotMem.e_dataType;

   plotMsgSize = getCreatePlot2dMsgSize(&plot);
   msg = (char*)malloc(plotMsgSize);
   if(NULL == msg)
      return;

   packCreate2dPlotMsg_Interleaved(&plot, _this->t_plotMem.pc_memory, msg);

   sendPlotPacket(_this, msg, plotMsgSize, 0);

   free(msg);
}

void sendMemoryToPlot_Update1D(tSendMemToPlot* _this)
{
   unsigned int writeIndex = _this->i_writeIndex;
   unsigned int readIndex = _this->i_readIndex;
   if(writeIndex != readIndex)
   {
      char* msg1 = NULL;
      char* msg2 = NULL;

      unsigned int dataIndex1 = 0;
      unsigned int dataIndex2 = 0;

      unsigned int plotMsgSize1 = 0;
      unsigned int plotMsgSize2 = 0;

      PLOTTER_BOOL bContiguous = FALSE;
      unsigned int stopIndexMsg1 = 0;

      t1dPlot plot;
      plot.curveName = _this->pc_curveName;
      plot.plotName = _this->pc_plotName;
      plot.yAxisType = _this->t_plotMem.e_dataType;

      if(writeIndex > readIndex)
      {
         bContiguous = TRUE;
         stopIndexMsg1 = writeIndex;
      }
      else
      {
         bContiguous = writeIndex == 0; // Contiguous if write index is at the beginning of the circular buffer.
         stopIndexMsg1 = _this->t_plotMem.i_numSamples;
      }

      plot.numSamp = stopIndexMsg1 - readIndex;
      plotMsgSize1 = getUpdatePlot1dMsgSize(&plot);
      msg1 = (char*)malloc(plotMsgSize1);
      if(NULL == msg1)
         return;
      dataIndex1 = packUpdate1dPlotMsg_withoutData(&plot, readIndex, msg1);

      packArrayOfStructs( msg1+dataIndex1,
                          _this->t_plotMem.pc_memory + (_this->t_plotMem.i_bytesBetweenValues*readIndex),
                          plot.numSamp,
                          _this->t_plotMem.i_dataSizeBytes,
                          _this->t_plotMem.i_bytesBetweenValues );

      if(!bContiguous)
      {
         plot.numSamp = writeIndex;
         plotMsgSize2 = getUpdatePlot1dMsgSize(&plot);
         msg2 = (char*)malloc(plotMsgSize2);
         if(NULL == msg2)
            return;
         dataIndex2 = packUpdate1dPlotMsg_withoutData(&plot, 0, msg2);

         packArrayOfStructs( msg2+dataIndex2,
                             _this->t_plotMem.pc_memory,
                             plot.numSamp,
                             _this->t_plotMem.i_dataSizeBytes,
                             _this->t_plotMem.i_bytesBetweenValues );
      }

      _this->i_readIndex = writeIndex;

      if(msg1 != NULL)
      {
         sendPlotPacket(_this, msg1, plotMsgSize1, 0);
         free(msg1);
      }

      if(msg2 != NULL)
      {
         sendPlotPacket(_this, msg2, plotMsgSize2, 0);
         free(msg2);
      }
   }
}

void sendMemoryToPlot_Update2D(tSendMemToPlot* _this)
{
   unsigned int writeIndex = _this->i_writeIndex;
   unsigned int readIndex = _this->i_readIndex;
   if(writeIndex != readIndex)
   {
      char* msg1 = NULL;
      char* msg2 = NULL;

      unsigned int dataIndex1 = 0;
      unsigned int dataIndex2 = 0;

      unsigned int plotMsgSize1 = 0;
      unsigned int plotMsgSize2 = 0;

      PLOTTER_BOOL bContiguous = FALSE;
      unsigned int stopIndexMsg1 = 0;

      t2dPlot plot;
      plot.curveName = _this->pc_curveName;
      plot.plotName = _this->pc_plotName;
      plot.interleaved = _this->t_plotMem.b_interleaved;

      plot.xAxisType = _this->t_plotMem.e_dataType;
      plot.yAxisType = _this->t_plotMem_separateYAxis.e_dataType;

      if(writeIndex > readIndex)
      {
         bContiguous = TRUE;
         stopIndexMsg1 = writeIndex;
      }
      else
      {
         bContiguous = writeIndex == 0; // Contiguous if write index is at the beginning of the circular buffer.
         stopIndexMsg1 = _this->t_plotMem.i_numSamples;
      }

      plot.numSamp = stopIndexMsg1 - readIndex;
      plotMsgSize1 = getUpdatePlot2dMsgSize(&plot);
      msg1 = (char*)malloc(plotMsgSize1);
      if(NULL == msg1)
         return;

      dataIndex1 = packUpdate2dPlotMsg_withoutData(&plot, readIndex, msg1);

      packArrayOfStructs( msg1+dataIndex1,
                          _this->t_plotMem.pc_memory + (_this->t_plotMem.i_bytesBetweenValues*readIndex),
                          plot.numSamp,
                          _this->t_plotMem.i_dataSizeBytes,
                          _this->t_plotMem.i_bytesBetweenValues );

      packArrayOfStructs( msg1+dataIndex1 + (PLOT_DATA_TYPE_SIZES[_this->t_plotMem.e_dataType]*plot.numSamp),
                          _this->t_plotMem_separateYAxis.pc_memory + (_this->t_plotMem_separateYAxis.i_bytesBetweenValues*readIndex),
                          plot.numSamp,
                          _this->t_plotMem_separateYAxis.i_dataSizeBytes,
                          _this->t_plotMem_separateYAxis.i_bytesBetweenValues );

      if(!bContiguous)
      {
         plot.numSamp = writeIndex;
         plotMsgSize2 = getUpdatePlot2dMsgSize(&plot);
         msg2 = (char*)malloc(plotMsgSize2);
         if(NULL == msg2)
            return;

         dataIndex2 = packUpdate2dPlotMsg_withoutData(&plot, 0, msg2);

         packArrayOfStructs( msg2+dataIndex2,
                             _this->t_plotMem.pc_memory,
                             plot.numSamp,
                             _this->t_plotMem.i_dataSizeBytes,
                             _this->t_plotMem.i_bytesBetweenValues );

         packArrayOfStructs( msg2+dataIndex2 + (PLOT_DATA_TYPE_SIZES[_this->t_plotMem.e_dataType]*plot.numSamp),
                             _this->t_plotMem_separateYAxis.pc_memory,
                             plot.numSamp,
                             _this->t_plotMem_separateYAxis.i_dataSizeBytes,
                             _this->t_plotMem_separateYAxis.i_bytesBetweenValues );
      }

      _this->i_readIndex = writeIndex;

      if(msg1 != NULL)
      {

         sendPlotPacket(_this, msg1, plotMsgSize1, 0);
         free(msg1);
      }

      if(msg2 != NULL)
      {
         sendPlotPacket(_this, msg2, plotMsgSize2, 0);
         free(msg2);
      }
   }
}

void sendMemoryToPlot_Update2D_Interleaved(tSendMemToPlot* _this)
{
   unsigned int writeIndex = _this->i_writeIndex;
   unsigned int readIndex = _this->i_readIndex;
   if(writeIndex != readIndex)
   {
      char* msg1 = NULL;
      char* msg2 = NULL;

      unsigned int dataIndex1 = 0;
      unsigned int dataIndex2 = 0;

      unsigned int plotMsgSize1 = 0;
      unsigned int plotMsgSize2 = 0;

      PLOTTER_BOOL bContiguous = FALSE;
      unsigned int stopIndexMsg1 = 0;

      t2dPlot plot;
      plot.curveName = _this->pc_curveName;
      plot.plotName = _this->pc_plotName;
      plot.interleaved = _this->t_plotMem.b_interleaved;

      plot.xAxisType = _this->t_plotMem.e_dataType;
      plot.yAxisType = _this->t_plotMem.e_dataType;

      if(writeIndex > readIndex)
      {
         bContiguous = TRUE;
         stopIndexMsg1 = writeIndex;
      }
      else
      {
         bContiguous = writeIndex == 0; // Contiguous if write index is at the beginning of the circular buffer.
         stopIndexMsg1 = _this->t_plotMem.i_numSamples;
      }

      plot.numSamp = stopIndexMsg1 - readIndex;
      plotMsgSize1 = getUpdatePlot2dMsgSize(&plot);
      msg1 = (char*)malloc(plotMsgSize1);
      if(NULL == msg1)
         return;

      dataIndex1 = packUpdate2dPlotMsg_Interleaved_withoutData(&plot, readIndex, msg1);

      packArrayOfStructs( msg1+dataIndex1,
                          _this->t_plotMem.pc_memory + (_this->t_plotMem.i_bytesBetweenValues*readIndex),
                          plot.numSamp,
                          _this->t_plotMem.i_dataSizeBytes,
                          _this->t_plotMem.i_bytesBetweenValues );

      if(!bContiguous)
      {
         plot.numSamp = writeIndex;
         plotMsgSize2 = getUpdatePlot2dMsgSize(&plot);
         msg2 = (char*)malloc(plotMsgSize2);
         if(NULL == msg2)
            return;

         dataIndex2 = packUpdate2dPlotMsg_Interleaved_withoutData(&plot, 0, msg2);

         packArrayOfStructs( msg2+dataIndex2,
                             _this->t_plotMem.pc_memory,
                             plot.numSamp,
                             _this->t_plotMem.i_dataSizeBytes,
                             _this->t_plotMem.i_bytesBetweenValues );
      }

      _this->i_readIndex = writeIndex;

      if(msg1 != NULL)
      {

         sendPlotPacket(_this, msg1, plotMsgSize1, 0);
         free(msg1);
      }

      if(msg2 != NULL)
      {
         sendPlotPacket(_this, msg2, plotMsgSize2, 0);
         free(msg2);
      }
   }
}

static int sendPlotPacket(tSendMemToPlot* _this, const char* msg, unsigned int msgSize, int isGroupFinalMsg)
{
   int retVal = -1;
   const char* ipAddr = _this->pc_ipAddr;
   unsigned short ipPort = _this->s_ipPort;

   plotThreading_mutexLock(&gt_sendMemToPlot_mutex); // If this is on the group plot thread, the recursive mutex will return immediately. On an other thread this will return when the group plot thread is done.

   if(g_groupPlotMsgs && !isGroupFinalMsg)
   {
      plotMsgGroupAdd(_this, msg, msgSize); // This is being called from the group plot thread and this isn't the final message, so just queue it up.
      plotThreading_mutexUnlock(&gt_sendMemToPlot_mutex); // Unlock before early return.
      return 0;
   }
   plotThreading_mutexUnlock(&gt_sendMemToPlot_mutex); // Unlock now that we know this isn't an early return.

   if(_this->b_closeSocketAfterSend)
   {
      retVal = sendTCPPacket(ipAddr, ipPort, msg, msgSize);
   }
   else
   {
      PLOTTER_BOOL b_newConnection = FALSE;
      if( (int)_this->i_tcpSocketFd <= 0 ) // Consider FD of 0 as invalid
      {
         // Need to init
         _this->i_tcpSocketFd = sendTCPPacket_init(ipAddr, ipPort);
         b_newConnection = TRUE;
      }
      if( (int)_this->i_tcpSocketFd > 0 )
      {
         // Is valid FD, send packet
         retVal = sendTCPPacket_send(_this->i_tcpSocketFd, msg, msgSize);
         if(retVal < 0 && b_newConnection == FALSE)
         {
            // Bad send. Close, init and try to send again
            sendTCPPacket_close(_this->i_tcpSocketFd);

            _this->i_tcpSocketFd = sendTCPPacket_init(ipAddr, ipPort);
            if( (int)_this->i_tcpSocketFd > 0 )
            {
               retVal = sendTCPPacket_send(_this->i_tcpSocketFd, msg, msgSize);
            }
         }
      }

   }

   return retVal;
}

void sendMemoryToPlot_Interleaved1DPlots(tSendMemToPlot* xAxis, tSendMemToPlot* yAxis)
{
   tSendMemToPlot* xAxis_sendMem = xAxis;
   unsigned int xAxis_writeIndex = 0; // This will be determined later to ensure we send the same number of X and Y axis samples.
   unsigned int xAxis_readIndex = xAxis_sendMem->i_readIndex;
   PLOTTER_BOOL xAxis_contiguous = FALSE;
   unsigned int xAxis_stopIndexMsg1 = 0;
   t1dPlot xAxis_plot;
   unsigned int xAxis_plotMsg1Size = 0;
   unsigned int xAxis_plotMsg2Size = 0;

   tSendMemToPlot* yAxis_sendMem = yAxis;
   unsigned int yAxis_writeIndex = 0; // This will be determined later to ensure we send the same number of X and Y axis samples.
   unsigned int yAxis_readIndex = yAxis_sendMem->i_readIndex;
   PLOTTER_BOOL yAxis_contiguous = FALSE;
   unsigned int yAxis_stopIndexMsg1 = 0;
   t1dPlot yAxis_plot;
   unsigned int yAxis_plotMsg1Size = 0;
   unsigned int yAxis_plotMsg2Size = 0;

   unsigned int xAxis_numSampAvailable = xAxis->i_writeIndex >= xAxis->i_readIndex ?
         xAxis->i_writeIndex - xAxis->i_readIndex :
         xAxis->t_plotMem.i_numSamples + xAxis->i_writeIndex - xAxis->i_readIndex;
   unsigned int yAxis_numSampAvailable = yAxis->i_writeIndex >= yAxis->i_readIndex ?
         yAxis->i_writeIndex - yAxis->i_readIndex :
         yAxis->t_plotMem.i_numSamples + yAxis->i_writeIndex - yAxis->i_readIndex;

   unsigned int xAxis_dataToUseEndIndex = xAxis->i_writeIndex;
   unsigned int yAxis_dataToUseEndIndex = yAxis->i_writeIndex;

   // If there is no data to plot, return early.
   if(xAxis_numSampAvailable == 0 || yAxis_numSampAvailable == 0)
   {
      return;
   }

   // Only plot min(numXSampAvailable, numYSampAvailable) number of samples.
   if(xAxis_numSampAvailable > yAxis_numSampAvailable)
   {
      // Need to reduce the X Axis Data to Use End Index.
      unsigned int numSampToRemove = xAxis_numSampAvailable - yAxis_numSampAvailable;
      xAxis_dataToUseEndIndex = xAxis_dataToUseEndIndex >= numSampToRemove ?
            xAxis_dataToUseEndIndex - numSampToRemove :
            xAxis->t_plotMem.i_numSamples + xAxis_dataToUseEndIndex - numSampToRemove;
   }
   else if(yAxis_numSampAvailable > xAxis_numSampAvailable)
   {
      // Need to reduce the Y Axis Data to Use End Index.
      unsigned int numSampToRemove = yAxis_numSampAvailable - xAxis_numSampAvailable;
      yAxis_dataToUseEndIndex = yAxis_dataToUseEndIndex >= numSampToRemove ?
            yAxis_dataToUseEndIndex - numSampToRemove :
            yAxis->t_plotMem.i_numSamples + yAxis_dataToUseEndIndex - numSampToRemove;
   }
   // Else there is the same amount to samples to plot for the X Axis and Y Axis. No Dothing.

   xAxis_writeIndex = xAxis_dataToUseEndIndex;
   yAxis_writeIndex = yAxis_dataToUseEndIndex;

   // Determine the size of the X-Axis message(s)
   xAxis_plot.curveName = xAxis_sendMem->pc_curveName;
   xAxis_plot.plotName = xAxis_sendMem->pc_plotName;
   xAxis_plot.yAxisType = xAxis_sendMem->t_plotMem.e_dataType;

   if(xAxis_writeIndex > xAxis_readIndex)
   {
      xAxis_contiguous = TRUE;
      xAxis_stopIndexMsg1 = xAxis_writeIndex;
   }
   else
   {
      xAxis_contiguous = xAxis_writeIndex == 0; // Contiguous if write index is at the beginning of the circular buffer.
      xAxis_stopIndexMsg1 = xAxis_sendMem->t_plotMem.i_numSamples;
   }

   xAxis_plot.numSamp = xAxis_stopIndexMsg1 - xAxis_readIndex;
   xAxis_plotMsg1Size = getUpdatePlot1dMsgSize(&xAxis_plot);

   if(!xAxis_contiguous)
   {
      xAxis_plot.numSamp = xAxis_writeIndex;
      xAxis_plotMsg2Size = getUpdatePlot1dMsgSize(&xAxis_plot);
   }

   // Determine the size of the Y-Axis message(s)
   yAxis_plot.curveName = yAxis_sendMem->pc_curveName;
   yAxis_plot.plotName = yAxis_sendMem->pc_plotName;
   yAxis_plot.yAxisType = yAxis_sendMem->t_plotMem.e_dataType;

   if(yAxis_writeIndex > yAxis_readIndex)
   {
      yAxis_contiguous = TRUE;
      yAxis_stopIndexMsg1 = yAxis_writeIndex;
   }
   else
   {
      yAxis_contiguous = yAxis_writeIndex == 0; // Contiguous if write index is at the beginning of the circular buffer.
      yAxis_stopIndexMsg1 = yAxis_sendMem->t_plotMem.i_numSamples;
   }

   yAxis_plot.numSamp = yAxis_stopIndexMsg1 - yAxis_readIndex;
   yAxis_plotMsg1Size = getUpdatePlot1dMsgSize(&yAxis_plot);

   if(!yAxis_contiguous)
   {
      yAxis_plot.numSamp = yAxis_writeIndex;
      yAxis_plotMsg2Size = getUpdatePlot1dMsgSize(&yAxis_plot);
   }

   // Pack and send the final message.
   {
      unsigned int newMsgSize = GROUP_MSG_HEADER_SIZE + xAxis_plotMsg1Size + xAxis_plotMsg2Size + yAxis_plotMsg1Size + yAxis_plotMsg2Size;
      char *msgX1, *msgX2, *msgY1, *msgY2;

      unsigned int dataIndex = 0;
      int temp = E_MULPITLE_PLOTS;

      char* multiPlotMsg = (char*)malloc(newMsgSize);
      if(NULL == multiPlotMsg)
         return;

      msgX1 = multiPlotMsg + GROUP_MSG_HEADER_SIZE;
      msgX2 = msgX1 + xAxis_plotMsg1Size;
      msgY1 = msgX2 + xAxis_plotMsg2Size;
      msgY2 = msgY1 + yAxis_plotMsg1Size;

      // Pack the Header.
      memcpy(&multiPlotMsg[0], &temp, 4);
      memcpy(&multiPlotMsg[4], &newMsgSize, 4);

      // Pack X-Axis
      xAxis_plot.numSamp = xAxis_stopIndexMsg1 - xAxis_readIndex;
      dataIndex = packUpdate1dPlotMsg_withoutData(&xAxis_plot, xAxis_readIndex, msgX1);
      packArrayOfStructs( msgX1+dataIndex,
                          xAxis_sendMem->t_plotMem.pc_memory + (xAxis_sendMem->t_plotMem.i_bytesBetweenValues*xAxis_readIndex),
                          xAxis_plot.numSamp,
                          xAxis_sendMem->t_plotMem.i_dataSizeBytes,
                          xAxis_sendMem->t_plotMem.i_bytesBetweenValues );

      if(xAxis_plotMsg2Size > 0)
      {
         xAxis_plot.numSamp = xAxis_writeIndex;
         dataIndex = packUpdate1dPlotMsg_withoutData(&xAxis_plot, 0, msgX2);
         packArrayOfStructs( msgX2+dataIndex,
                             xAxis_sendMem->t_plotMem.pc_memory,
                             xAxis_plot.numSamp,
                             xAxis_sendMem->t_plotMem.i_dataSizeBytes,
                             xAxis_sendMem->t_plotMem.i_bytesBetweenValues );
      }
      xAxis_sendMem->i_readIndex = xAxis_writeIndex;


      // Pack Y-Axis
      yAxis_plot.numSamp = yAxis_stopIndexMsg1 - yAxis_readIndex;
      dataIndex = packUpdate1dPlotMsg_withoutData(&yAxis_plot, yAxis_readIndex, msgY1);
      packArrayOfStructs( msgY1+dataIndex,
                          yAxis_sendMem->t_plotMem.pc_memory + (yAxis_sendMem->t_plotMem.i_bytesBetweenValues*yAxis_readIndex),
                          yAxis_plot.numSamp,
                          yAxis_sendMem->t_plotMem.i_dataSizeBytes,
                          yAxis_sendMem->t_plotMem.i_bytesBetweenValues );

      if(yAxis_plotMsg2Size > 0)
      {
         yAxis_plot.numSamp = yAxis_writeIndex;
         dataIndex = packUpdate1dPlotMsg_withoutData(&yAxis_plot, 0, msgY2);
         packArrayOfStructs( msgY2+dataIndex,
                             yAxis_sendMem->t_plotMem.pc_memory,
                             yAxis_plot.numSamp,
                             yAxis_sendMem->t_plotMem.i_dataSizeBytes,
                             yAxis_sendMem->t_plotMem.i_bytesBetweenValues );
      }
      yAxis_sendMem->i_readIndex = yAxis_writeIndex;

      sendPlotPacket(xAxis, multiPlotMsg, newMsgSize, 0);

      free(multiPlotMsg);
   }

}

