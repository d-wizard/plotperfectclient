/* Copyright 2017 - 2019, 2021 Dan Williams. All Rights Reserved.
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
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "sendMemoryToPlot.h"
#include "smartPlotMessage.h"
#include "plotThreading.h"
#include "sendTCPPacket.h"

#ifdef TIME_PLOT_WINDOWS
#include <windows.h>
#pragma comment(lib, "wsock32.lib")
#else
#include <time.h>
#endif

//*****************************************************************************
// Preprocessor Directives
//*****************************************************************************
#define GROUP_INTERLEAVED_PLOT_MSGS

//*****************************************************************************
// Types
//*****************************************************************************
typedef struct smartPlotListElem
{
   struct smartPlotListElem* prev;
   tSendMemToPlot cur;
   struct smartPlotListElem* next;

   struct smartPlotListElem* interleavedPair;
   PLOTTER_BOOL interleaved_isXAxis;
}tSmartPlotListElem;

//*****************************************************************************
// Globals
//*****************************************************************************
// This is a circular list. Meaning the last element in the list's next will point to
// the beginning of the list and the first element in the list's prev will point to the end
// of the list.
static tSmartPlotListElem* gt_smartPlotList = NULL;

static CREATE_PLOT_MUTEX(gt_smartPlotList_mutex);

static char g_plotHostName[MAX_IP_ADDR_STRING_SIZE] = "plotter";
static unsigned short g_plotPort = 2000;

//*****************************************************************************
// Local Functions
//*****************************************************************************
static tSmartPlotListElem* smartPlot_findListElem(const char* plotName, const char* curveName)
{
   tSmartPlotListElem* listElem = NULL;
   if(gt_smartPlotList != NULL)
   {
      tSmartPlotListElem* list = gt_smartPlotList;

      do
      {
         if( strcmp(list->cur.pc_curveName, curveName) == 0 &&
             strcmp(list->cur.pc_plotName,  plotName) == 0 )
         {
            listElem = list;
         }
         list = list->next;
      }
      while(list != gt_smartPlotList && listElem == NULL); // List is circular. When it wraps back to beginning of the list, stop looping.
   }
   return listElem;
}


static PLOTTER_BOOL smartPlot_find(const char* plotName, const char* curveName, tSmartPlotListElem** retListElem, PLOTTER_BOOL allowNewPlot)
{
   PLOTTER_BOOL newPlot = FALSE;
   tSmartPlotListElem* listElem = NULL;

   plotThreading_mutexLock(&gt_smartPlotList_mutex);

   listElem = smartPlot_findListElem(plotName, curveName);
   if(listElem != NULL)
   {
      *retListElem = listElem;
   }
   else
   {
      newPlot = TRUE;
      if(allowNewPlot)
      {
         tSmartPlotListElem* newListElem = (tSmartPlotListElem*)malloc(sizeof(tSmartPlotListElem));

         if(newListElem != NULL)
         {
            // Claim this plot by setting its plot/curve name.
            newListElem->cur.pc_curveName = curveName;
            newListElem->cur.pc_plotName = plotName;

            newListElem->interleavedPair = NULL;

            // Update list.
            if(gt_smartPlotList == NULL)
            {
               // This is the first element in the circular list.
               gt_smartPlotList = newListElem;
               gt_smartPlotList->prev = newListElem;
               gt_smartPlotList->next = newListElem;
            }
            else
            {
               // Add to end of circular list.
               newListElem->next = gt_smartPlotList;
               newListElem->prev = gt_smartPlotList->prev;
               newListElem->prev->next = newListElem;
               gt_smartPlotList->prev = newListElem;
            }
         }
         *retListElem = newListElem;
      }
   }

   plotThreading_mutexUnlock(&gt_smartPlotList_mutex);

   return newPlot;
}

static void* smartPlot_flushThread(void* p_sleepBetweenFlush_ms)
{
   unsigned int sleepBetweenFlush_ms = *((unsigned int*)p_sleepBetweenFlush_ms);
   while(1)
   {
#ifdef PLOTTER_WINDOWS_BUILD
      Sleep(sleepBetweenFlush_ms);
#else
      usleep(sleepBetweenFlush_ms*1000);
#endif
      smartPlot_flush_all();
   }
   return NULL;
}

//*****************************************************************************
// Public Functions
//*****************************************************************************

void smartPlot_networkConfigure(const char *hostName, const unsigned short port)
{
#if defined PLOTTER_WINDOWS_BUILD && !defined __MINGW32_VERSION
   strncpy_s(g_plotHostName, sizeof(g_plotHostName), hostName, sizeof(g_plotHostName));
#else
   strncpy(g_plotHostName, hostName, sizeof(g_plotHostName));
   g_plotHostName[sizeof(g_plotHostName)-1] = '\0'; // Make sure null terminated.
#endif

   g_plotPort = port;
}


void smartPlot_interleaved( const void* inDataToPlot,
                            ePlotDataTypes inDataType,
                            int inDataSize,
                            int plotSize,
                            int updateSize,
                            const char* plotName,
                            const char* curveName_x,
                            const char* curveName_y )
{
   tSmartPlotListElem* listElem_x = NULL;
   tSmartPlotListElem* listElem_y = NULL;
   tSendMemToPlot* plot_x = NULL;
   tSendMemToPlot* plot_y = NULL;

   // We can't make a new plot if the plot size or plot data type are not valid.
   // If we are not making a new plot (i.e. the plot already exists) then these
   // values do not need to be valid (we will just use the values that were used
   // when the plot was created).
   PLOTTER_BOOL newPlotParametersAreValid = (plotSize > 0 && isPlotDataTypeValid(inDataType));

   PLOTTER_BOOL newPlot_x = smartPlot_find(plotName, curveName_x, &listElem_x, newPlotParametersAreValid);
   PLOTTER_BOOL newPlot_y = smartPlot_find(plotName, curveName_y, &listElem_y, newPlotParametersAreValid);

   assert(newPlot_x == newPlot_y); // If the are inequal, something is wrong.

   if(listElem_x == NULL || listElem_y == NULL)
   {
      return;
   }

   plot_x = &listElem_x->cur;
   plot_y = &listElem_y->cur;

   if(newPlot_x && newPlot_y)
   {
      int memberSize = PLOT_DATA_TYPE_SIZES[inDataType];
      void* newMem = malloc(memberSize * 2 * plotSize);
      if(NULL == newMem)
         return;

      plot_x->t_plotMem.b_arrayOfStructs = TRUE;
      plot_x->t_plotMem.b_interleaved = FALSE; // Interleaved assume 1 2D plot, rather than 2 1D plots that we are doing here.
      plot_x->t_plotMem.e_dataType = inDataType;
      plot_x->t_plotMem.e_plotDim = E_PLOT_1D;
      plot_x->t_plotMem.i_bytesBetweenValues = 2 * memberSize;
      plot_x->t_plotMem.i_dataSizeBytes = memberSize;
      plot_x->t_plotMem.i_numSamples = plotSize;
      plot_x->t_plotMem.pc_memory = (char*)newMem;

      // plot_y will be the same exept for the start position of the first member
      plot_y->t_plotMem = plot_x->t_plotMem;
      plot_y->t_plotMem.pc_memory = ((char*)newMem) + memberSize;

      sendMemoryToPlot_Init( plot_x, g_plotHostName, g_plotPort, TRUE, plotName, curveName_x);
      sendMemoryToPlot_Init( plot_y, g_plotHostName, g_plotPort, TRUE, plotName, curveName_y);

      // Link the 2 list elements together.
      listElem_x->interleavedPair = listElem_y;
      listElem_x->interleaved_isXAxis = TRUE;
      listElem_y->interleavedPair = listElem_x;
      listElem_y->interleaved_isXAxis = FALSE;
   }
   else if(plotSize != (int)plot_x->t_plotMem.i_numSamples && plotSize > 0 && isPlotDataTypeValid(plot_x->t_plotMem.e_dataType))
   {
      int memberSize = PLOT_DATA_TYPE_SIZES[plot_x->t_plotMem.e_dataType];
      char* oldMem_toFree = plot_x->t_plotMem.pc_memory;
      char* newMem = NULL;

      plot_x->i_readIndex = plot_x->i_writeIndex = 0;
      plot_y->i_readIndex = plot_y->i_writeIndex = 0;
      plot_x->t_plotMem.i_numSamples = plotSize;
      plot_y->t_plotMem.i_numSamples = plotSize;

      newMem = (char*)malloc(memberSize * 2 * plotSize);
      if(NULL == newMem)
         return;

      plot_x->t_plotMem.pc_memory = newMem;
      plot_y->t_plotMem.pc_memory = plot_x->t_plotMem.pc_memory + memberSize;

      free(oldMem_toFree);
   }

   {
      int numSampToLeftToWrite = inDataSize;
      int numSampWritten = 0;
      int writeIndex = plot_x->i_writeIndex;
      int numSampLeftForPlotSend;
      char* writeLocationPtr = plot_x->t_plotMem.pc_memory;
      char* readLocationPtr = (char*)inDataToPlot;
      int memberSize = PLOT_DATA_TYPE_SIZES[plot_x->t_plotMem.e_dataType];

      int numSampAlreadyInBuff = (int)plot_x->i_writeIndex - (int)plot_x->i_readIndex;
      if(numSampAlreadyInBuff < 0)
         numSampAlreadyInBuff += plot_x->t_plotMem.i_numSamples;

      // When update size is a negative number, no plot message should be sent.
      // Set update size to a value large than the number of samples in the plot
      // to ensure a message is not sent.
      if(updateSize < 0)
         updateSize = plot_x->t_plotMem.i_numSamples + 1;

      numSampLeftForPlotSend = updateSize - numSampAlreadyInBuff;

      while(numSampToLeftToWrite > 0)
      {
         int numSampToEnd = plot_x->t_plotMem.i_numSamples - writeIndex;
         int numSampToWrite = (numSampToEnd < numSampToLeftToWrite) ? numSampToEnd : numSampToLeftToWrite;
         memcpy( &writeLocationPtr[2 * memberSize * writeIndex],
                 &readLocationPtr[2 * memberSize * numSampWritten],
                 2 * memberSize * numSampToWrite );

         numSampWritten += numSampToWrite;
         numSampToLeftToWrite -= numSampToWrite;
         writeIndex += numSampToWrite;
         if(writeIndex >= (int)plot_x->t_plotMem.i_numSamples)
         {
            writeIndex = 0;
         }
      }

      plot_x->i_writeIndex = writeIndex;
      plot_y->i_writeIndex = writeIndex;

      // Never update plot if update size is greater than the plot size.
      if(plot_x->t_plotMem.i_numSamples >= (unsigned int)updateSize)
      {
         if( (numSampAlreadyInBuff + numSampWritten) >= (int)plot_x->t_plotMem.i_numSamples )
         {
            // More samples need to be updated than size of the plot, send Create message (which will
            // send all the samples in the plot).
            sendMemoryToPlot_Create1D(plot_x);
            sendMemoryToPlot_Create1D(plot_y);

            // By sending the Create plot, all the data has been read. Set the read index to the write index.
            plot_x->i_readIndex = plot_x->i_writeIndex;
            plot_y->i_readIndex = plot_y->i_writeIndex;
         }
         else if(numSampWritten >= numSampLeftForPlotSend)
         {
#ifdef GROUP_INTERLEAVED_PLOT_MSGS
            sendMemoryToPlot_Interleaved1DPlots(plot_x, plot_y);
#else
            sendMemoryToPlot(plot_x);
            sendMemoryToPlot(plot_y);
#endif
         }
      }

   }

}

void smartPlot_1D( const void* inDataToPlot,
                   ePlotDataTypes inDataType,
                   int inDataSize,
                   int plotSize,
                   int updateSize,
                   const char* plotName,
                   const char* curveName )
{
   tSmartPlotListElem* listElem = NULL;
   tSendMemToPlot* plot = NULL;

   // We can't make a new plot if the plot size or plot data type are not valid.
   // If we are not making a new plot (i.e. the plot already exists) then these
   // values do not need to be valid (we will just use the values that were used
   // when the plot was created).
   PLOTTER_BOOL newPlotParametersAreValid = (plotSize > 0 && isPlotDataTypeValid(inDataType));

   PLOTTER_BOOL newPlot = smartPlot_find(plotName, curveName, &listElem, newPlotParametersAreValid);

   if(listElem == NULL)
   {
      return;
   }

   plot = &listElem->cur;

   if(newPlot)
   {
      int memberSize = PLOT_DATA_TYPE_SIZES[inDataType];
      void* newMem = malloc(memberSize * plotSize);
      if(NULL == newMem)
         return;

      plot->t_plotMem.b_arrayOfStructs = FALSE;
      plot->t_plotMem.b_interleaved = FALSE;
      plot->t_plotMem.e_dataType = inDataType;
      plot->t_plotMem.e_plotDim = E_PLOT_1D;
      plot->t_plotMem.i_bytesBetweenValues = memberSize;
      plot->t_plotMem.i_dataSizeBytes = memberSize;
      plot->t_plotMem.i_numSamples = plotSize;
      plot->t_plotMem.pc_memory = (char*)newMem;

      sendMemoryToPlot_Init( plot, g_plotHostName, g_plotPort, TRUE, plotName, curveName);
   }
   else if(plotSize != (int)plot->t_plotMem.i_numSamples && plotSize > 0 && isPlotDataTypeValid(plot->t_plotMem.e_dataType))
   {
      int memberSize = PLOT_DATA_TYPE_SIZES[plot->t_plotMem.e_dataType];
      char* oldMem_toFree = plot->t_plotMem.pc_memory;
      char* newMem = NULL;

      plot->i_readIndex = plot->i_writeIndex = 0;

      newMem = (char*)malloc(memberSize * plotSize);
      if(NULL == newMem)
         return;

      plot->t_plotMem.pc_memory = newMem;
      plot->t_plotMem.i_numSamples = plotSize;
      free(oldMem_toFree);
   }

   {
      int numSampToLeftToWrite = inDataSize;
      int numSampWritten = 0;
      int writeIndex = plot->i_writeIndex;
      int numSampLeftForPlotSend;
      char* writeLocationPtr = plot->t_plotMem.pc_memory;
      char* readLocationPtr = (char*)inDataToPlot;
      int memberSize = PLOT_DATA_TYPE_SIZES[plot->t_plotMem.e_dataType];

      int numSampAlreadyInBuff = (int)plot->i_writeIndex - (int)plot->i_readIndex;
      if(numSampAlreadyInBuff < 0)
         numSampAlreadyInBuff += plot->t_plotMem.i_numSamples;

      // When update size is a negative number, no plot message should be sent.
      // Set update size to a value large than the number of samples in the plot
      // to ensure a message is not sent.
      if(updateSize < 0)
         updateSize = plot->t_plotMem.i_numSamples + 1;

      numSampLeftForPlotSend = updateSize - numSampAlreadyInBuff;

      while(numSampToLeftToWrite > 0)
      {
         int numSampToEnd = plot->t_plotMem.i_numSamples - writeIndex;
         int numSampToWrite = (numSampToEnd < numSampToLeftToWrite) ? numSampToEnd : numSampToLeftToWrite;
         memcpy( &writeLocationPtr[memberSize * writeIndex],
                 &readLocationPtr[memberSize * numSampWritten],
                 memberSize * numSampToWrite );

         numSampWritten += numSampToWrite;
         numSampToLeftToWrite -= numSampToWrite;
         writeIndex += numSampToWrite;
         if(writeIndex >= (int)plot->t_plotMem.i_numSamples)
         {
            writeIndex = 0;
         }
      }

      plot->i_writeIndex = writeIndex;


      // Never update plot if update size is greater than the plot size.
      if(plot->t_plotMem.i_numSamples >= (unsigned int)updateSize)
      {
         if( (numSampAlreadyInBuff + numSampWritten) >= (int)plot->t_plotMem.i_numSamples )
         {
            // More samples need to be updated than size of the plot, send Create message (which will
            // send all the samples in the plot).
            sendMemoryToPlot_Create1D(plot);

            // By sending the Create plot, all the data has been read. Set the read index to the write index.
            plot->i_readIndex = plot->i_writeIndex;
         }
         else if(numSampWritten >= numSampLeftForPlotSend)
         {
            sendMemoryToPlot(plot);
         }
      }

   }

}

void smartPlot_2D( const void* inDataToPlotX,
                   ePlotDataTypes inDataTypeX,
                   const void* inDataToPlotY,
                   ePlotDataTypes inDataTypeY,
                   int inDataSize,
                   int plotSize,
                   int updateSize,
                   const char* plotName,
                   const char* curveName )
{
   tSmartPlotListElem* listElem = NULL;
   tSendMemToPlot* plot = NULL;

   // We can't make a new plot if the plot size or plot data type are not valid.
   // If we are not making a new plot (i.e. the plot already exists) then these
   // values do not need to be valid (we will just use the values that were used
   // when the plot was created).
   PLOTTER_BOOL newPlotParametersAreValid = (plotSize > 0 && isPlotDataTypeValid(inDataTypeX) && isPlotDataTypeValid(inDataTypeY));

   PLOTTER_BOOL newPlot = smartPlot_find(plotName, curveName, &listElem, newPlotParametersAreValid);

   if(listElem == NULL)
   {
      return;
   }

   plot = &listElem->cur;

   if(newPlot)
   {
      int memberSizeX = PLOT_DATA_TYPE_SIZES[inDataTypeX];
      int memberSizeY = PLOT_DATA_TYPE_SIZES[inDataTypeY];
      void* newMemX = malloc(memberSizeX * plotSize);
      void* newMemY = malloc(memberSizeY * plotSize);
      if(NULL == newMemX || newMemY == NULL)
         return;

      plot->t_plotMem.b_arrayOfStructs = FALSE;
      plot->t_plotMem.b_interleaved = FALSE;
      plot->t_plotMem.e_dataType = inDataTypeX;
      plot->t_plotMem.e_plotDim = E_PLOT_2D;
      plot->t_plotMem.i_bytesBetweenValues = memberSizeX;
      plot->t_plotMem.i_dataSizeBytes = memberSizeX;
      plot->t_plotMem.i_numSamples = plotSize;
      plot->t_plotMem.pc_memory = (char*)newMemX;

      plot->t_plotMem_separateYAxis = plot->t_plotMem;
      plot->t_plotMem_separateYAxis.e_dataType = inDataTypeY;
      plot->t_plotMem_separateYAxis.i_bytesBetweenValues = memberSizeY;
      plot->t_plotMem_separateYAxis.i_dataSizeBytes = memberSizeY;
      plot->t_plotMem_separateYAxis.pc_memory = (char*)newMemY;

      sendMemoryToPlot_Init( plot, g_plotHostName, g_plotPort, TRUE, plotName, curveName);
   }
   else if(plotSize != (int)plot->t_plotMem.i_numSamples && plotSize > 0 && isPlotDataTypeValid(plot->t_plotMem.e_dataType) && isPlotDataTypeValid(plot->t_plotMem_separateYAxis.e_dataType))
   {
      int memberSizeX = PLOT_DATA_TYPE_SIZES[plot->t_plotMem.e_dataType];
      int memberSizeY = PLOT_DATA_TYPE_SIZES[plot->t_plotMem_separateYAxis.e_dataType];
      char* oldMem_toFreeX = plot->t_plotMem.pc_memory;
      char* oldMem_toFreeY = plot->t_plotMem_separateYAxis.pc_memory;
      char* newMemX = NULL;
      char* newMemY = NULL;

      plot->i_readIndex = plot->i_writeIndex = 0;

      newMemX = (char*)malloc(memberSizeX * plotSize);
      newMemY = (char*)malloc(memberSizeY * plotSize);
      if(NULL == newMemX || NULL == newMemY)
         return;

      plot->t_plotMem.pc_memory = newMemX;
      plot->t_plotMem.i_numSamples = plotSize;
      free(oldMem_toFreeX);

      plot->t_plotMem_separateYAxis.pc_memory = newMemY;
      plot->t_plotMem_separateYAxis.i_numSamples = plotSize;
      free(oldMem_toFreeY);
   }

   {
      int numSampToLeftToWrite = inDataSize;
      int numSampWritten = 0;
      int writeIndex = plot->i_writeIndex;
      int numSampLeftForPlotSend;
      char* writeLocationPtrX = plot->t_plotMem.pc_memory;
      char* writeLocationPtrY = plot->t_plotMem_separateYAxis.pc_memory;
      char* readLocationPtrX = (char*)inDataToPlotX;
      char* readLocationPtrY = (char*)inDataToPlotY;
      int memberSizeX = PLOT_DATA_TYPE_SIZES[plot->t_plotMem.e_dataType];
      int memberSizeY = PLOT_DATA_TYPE_SIZES[plot->t_plotMem_separateYAxis.e_dataType];

      int numSampAlreadyInBuff = (int)plot->i_writeIndex - (int)plot->i_readIndex;
      if(numSampAlreadyInBuff < 0)
         numSampAlreadyInBuff += plot->t_plotMem.i_numSamples;

      // When update size is a negative number, no plot message should be sent.
      // Set update size to a value large than the number of samples in the plot
      // to ensure a message is not sent.
      if(updateSize < 0)
         updateSize = plot->t_plotMem.i_numSamples + 1;

      numSampLeftForPlotSend = updateSize - numSampAlreadyInBuff;

      while(numSampToLeftToWrite > 0)
      {
         int numSampToEnd = plot->t_plotMem.i_numSamples - writeIndex;
         int numSampToWrite = (numSampToEnd < numSampToLeftToWrite) ? numSampToEnd : numSampToLeftToWrite;

         memcpy( &writeLocationPtrX[memberSizeX * writeIndex],
                 &readLocationPtrX[memberSizeX * numSampWritten],
                 memberSizeX * numSampToWrite );
         memcpy( &writeLocationPtrY[memberSizeY * writeIndex],
                 &readLocationPtrY[memberSizeY * numSampWritten],
                 memberSizeY * numSampToWrite );

         numSampWritten += numSampToWrite;
         numSampToLeftToWrite -= numSampToWrite;
         writeIndex += numSampToWrite;
         if(writeIndex >= (int)plot->t_plotMem.i_numSamples)
         {
            writeIndex = 0;
         }
      }

      plot->i_writeIndex = writeIndex;


      // Never update plot if update size is greater than the plot size.
      if(plot->t_plotMem.i_numSamples >= (unsigned int)updateSize)
      {
         if( (numSampAlreadyInBuff + numSampWritten) >= (int)plot->t_plotMem.i_numSamples )
         {
            // More samples need to be updated than size of the plot, send Create message (which will
            // send all the samples in the plot).
            sendMemoryToPlot_Create2D(plot);

            // By sending the Create plot, all the data has been read. Set the read index to the write index.
            plot->i_readIndex = plot->i_writeIndex;
         }
         else if(numSampWritten >= numSampLeftForPlotSend)
         {
            sendMemoryToPlot(plot);
         }
      }

   }

}


void smartPlot_flush_all()
{
   if(gt_smartPlotList != NULL)
   {
      tSmartPlotListElem* smartPlotList = gt_smartPlotList;
      smartPlot_groupMsgStart(); // Send all flushed plots as one big message.
      do
      {
         if(smartPlotList->interleavedPair != NULL)
         {
            // This is an interleaved plot. Only plot interleaved as X, Y. Not Y, X.
            if(smartPlotList->interleaved_isXAxis)
            {
               smartPlot_flush_interleaved( smartPlotList->cur.pc_plotName,
                                            smartPlotList->cur.pc_curveName, // X Axis
                                            smartPlotList->interleavedPair->cur.pc_curveName ); // Y Axis
            }
         }
         else if(smartPlotList->cur.t_plotMem.e_plotDim == E_PLOT_2D)
         {
            // This is NOT an interleaved plot, flush as 2D.
            smartPlot_flush_2D(smartPlotList->cur.pc_plotName, smartPlotList->cur.pc_curveName);
         }
         else
         {
            // This is NOT an interleaved plot, flush as 1D.
            smartPlot_flush_1D(smartPlotList->cur.pc_plotName, smartPlotList->cur.pc_curveName);
         }
         smartPlotList = smartPlotList->next;
      }while(smartPlotList != gt_smartPlotList);// List is circular. When it wraps back to beginning of the list, stop looping.
      smartPlot_groupMsgEnd(); // Send the big group message with all the flushed plot messages.
   }
}


void smartPlot_groupMsgStart()
{
   plotMsgGroupStart();
}

void smartPlot_groupMsgEnd()
{
   plotMsgGroupEnd();
}


void smartPlot_deallocate( const char* plotName,
                           const char* curveName )
{
   tSmartPlotListElem* listElem = NULL;
   plotThreading_mutexLock(&gt_smartPlotList_mutex);

   listElem = smartPlot_findListElem(plotName, curveName);

   if(listElem != NULL)
   {
      if(listElem->next == listElem)
      {
         // This is the only entry in the list.
         gt_smartPlotList = NULL;
      }
      else
      {
         tSmartPlotListElem* oldPrev = listElem->prev;
         tSmartPlotListElem* oldNext = listElem->next;
         oldPrev->next = oldNext;
         oldNext->prev = oldPrev;
         if(gt_smartPlotList == listElem)
         {
            // This was the first element in the list.
            gt_smartPlotList = oldNext;
         }
      }

      // Close the TCP Socket
      if(listElem->cur.i_tcpSocketFd >= 0)
         sendTCPPacket_close(listElem->cur.i_tcpSocketFd);

      // Free the memory allocated for the current plot / curve, but make sure not to free it twice.
      // If this is an interleaved plot and this is the Y Axis, do not free the memory.
      if(listElem->interleavedPair == NULL || listElem->interleaved_isXAxis)
      {
         free(listElem->cur.t_plotMem.pc_memory);
         if(listElem->cur.t_plotMem.e_plotDim == E_PLOT_2D)
         {
            // Need to free Y axis of 2D plot.
            free(listElem->cur.t_plotMem_separateYAxis.pc_memory);
         }
      }
      free(listElem);
   }

   plotThreading_mutexUnlock(&gt_smartPlotList_mutex);
}

void smartPlot_deallocate_interleaved( const char* plotName,
                                       const char* curveName_x,
                                       const char* curveName_y)
{
   smartPlot_deallocate(plotName, curveName_x);
   smartPlot_deallocate(plotName, curveName_y);
}

void smartPlot_createFlushThread(unsigned int sleepBetweenFlush_ms)
{
   // Allocate new memory. No need to worry about deleting since this is exected to exist for the entire lifetime of the execuatable.
   unsigned int* newThread_sleepBetweenFlush_ms = (unsigned int*)malloc(sizeof(unsigned int));
   if(newThread_sleepBetweenFlush_ms != NULL)
   {
      *newThread_sleepBetweenFlush_ms = sleepBetweenFlush_ms;

      plotThreading_createNewThread(smartPlot_flushThread, newThread_sleepBetweenFlush_ms);
   }
}

void smartPlot_createFlushThread_withPriorityPolicy(unsigned int sleepBetweenFlush_ms, int priority, int policy)
{
   // Allocate new memory. No need to worry about deleting since this is exected to exist for the entire lifetime of the execuatable.
   unsigned int* newThread_sleepBetweenFlush_ms = (unsigned int*)malloc(sizeof(unsigned int));
   if(newThread_sleepBetweenFlush_ms != NULL)
   {
      *newThread_sleepBetweenFlush_ms = sleepBetweenFlush_ms;

      plotThreading_createNewThread_withPriorityPolicy(smartPlot_flushThread, newThread_sleepBetweenFlush_ms, priority, policy);
   }
}

void smartPlot_getTime(tSmartPlotTime* pTime)
{
#ifdef TIME_PLOT_WINDOWS
   static int needsInit = 1;
   static LARGE_INTEGER winTimeSpec_ticksPerSec;
   if(needsInit)
   {
      WSADATA wsda;
      needsInit = 0;
      WSAStartup(0x0101, &wsda);
      QueryPerformanceFrequency(&winTimeSpec_ticksPerSec);
   }

   LARGE_INTEGER curTick;
   QueryPerformanceCounter(&curTick);
   pTime->tv_sec = (curTick.QuadPart / winTimeSpec_ticksPerSec.QuadPart);
   pTime->tv_nsec = (((curTick.QuadPart - (pTime->tv_sec*winTimeSpec_ticksPerSec.QuadPart)) * 1000000000) / winTimeSpec_ticksPerSec.QuadPart);
#else
   clock_gettime(CLOCK_MONOTONIC, pTime);
#endif
}




