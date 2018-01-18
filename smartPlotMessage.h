/**************************************************************************
 * Copyright 2017 Dan Williams. All Rights Reserved.
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
 * NAME: smartPlotMessage.h
 *
 * DESCRIPTION: Stores samples to send to the plot GUI and sends the stored
 * samples to the plot GUI.
 *
 **************************************************************************/
#ifndef smartPlotMessage_h
#define smartPlotMessage_h

// Note: This file should be able to be included without access to any other plotter header files.
// As such, any plotter specific types, etc must be defined in this file.
#include <stddef.h> /* for NULL */


#if (defined(_WIN32) || defined(__WIN32__)) && (!defined inline)
   #define inline __inline // Windows doesn't use the inline keyword, instead it uses __inline
#endif


//*****************************************************************************
// Types
//*****************************************************************************
typedef enum
{
   E_INT_8,
   E_UINT_8,
   E_INT_16,
   E_UINT_16,
   E_INT_32,
   E_UINT_32,
   E_INT_64,
   E_UINT_64,
   E_FLOAT_32,
   E_FLOAT_64,
   E_TIME_STRUCT_64,
   E_TIME_STRUCT_128,
   E_INVALID_DATA_TYPE
}ePlotDataTypes;


#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************
Function:     smartPlot_networkConfigure

Description:  This function configures network parameters. If you are planning on
              using non-default values, call this function prior to plotting data.

Arguments:    hostName - Host name or IP address of the PlotGUI server.

              port - TCP port of the PlotGUI server.

Returns:      None.
*/
void smartPlot_networkConfigure(const char *hostName, const unsigned short port);

/**************************************************************************
Function:     smartPlot_interleaved

Description:  Plots interleaved samples. The data type of both samples in the
              interleaved pair needs to be the same.

Arguments:    inDataToPlot - Pointer to the samples to plot. If there are no
              new samples to plot this can be NULL, but inDataSize must be 0.

              inDataType - Enumeration that specifies the data type of the
              samples to plot. This value is only used the first time a new
              Plot Name / Curve Name combination is passed in.

              inDataSize - The number of new samples contained in inDataToPlot
              to plot.

              plotSize - The number of data points in the entire GUI plot.
              This value is only used the first time a new Plot Name /
              Curve Name combination is passed in.

              updateSize - The number of samples to send in each plot message to
              the plot GUI. Setting this value to 0 will flush the buffer (i.e
              send all samples to the plot that haven't already been sent).
              Setting this value to a negative number will ensure no message to
              the plot GUI will be sent.

              plotName - The Plot Name.

              curveName_x - The Curve Name of the first value in the interleaved pair.

              curveName_y - The Curve Name of the second value in the interleaved pair.

Returns:      None.
*/
void smartPlot_interleaved( const void* inDataToPlot,
                            ePlotDataTypes inDataType,
                            int inDataSize,
                            int plotSize,
                            int updateSize,
                            const char* plotName,
                            const char* curveName_x,
                            const char* curveName_y );


/**************************************************************************
Function:     smartPlot_1D

Description:  This function will plot data passed in. The data passed in
              will be sent to the plot GUI to create a simple 1D plot.

Arguments:    inDataToPlot - Pointer to the samples to plot. If there are no
              new samples to plot this can be NULL, but inDataSize must be 0.

              inDataType - Enumeration that specifies the data type of the
              samples to plot. This value is only used the first time a new
              Plot Name / Curve Name combination is passed in.

              inDataSize - The number of new samples contained in inDataToPlot
              to plot.

              plotSize - The number of data points in the entire GUI plot.
              This value is only used the first time a new Plot Name /
              Curve Name combination is passed in.

              updateSize - The number of samples to send in each plot message to
              the plot GUI. Setting this value to 0 will flush the buffer (i.e
              send all samples to the plot that haven't already been sent).
              Setting this value to a negative number will ensure no message to
              the plot GUI will be sent.

              plotName - The Plot Name.

              curveName - The Curve Name.

Returns:      None.
*/
void smartPlot_1D( const void* inDataToPlot,
                   ePlotDataTypes inDataType,
                   int inDataSize,
                   int plotSize,
                   int updateSize,
                   const char* plotName,
                   const char* curveName );


/**************************************************************************
Function:     smartPlot_groupMsgStart

Description:  This function is used to group a bunch of separate plot
              messages into one big message. Under some circumstances, it
              is helpful to have multiple plot messages hit the plot GUI at
              the same time.

              Once this function is called, all plot messages that would
              normally be sent right away will be stored off until
              smartPlot_groupMsgEnd is called, at which point all the
              separate plot messages will be sent as one big message.

*/
void smartPlot_groupMsgStart();

/**************************************************************************
Function:     smartPlot_groupMsgEnd

Description:  This function will send all the grouped messages that have been
              stored off to the plot GUI. After this function is called, all
              subsequent plot messages will be sent as normal until
              smartPlot_groupMsgStart is called again.

*/
void smartPlot_groupMsgEnd();



/**************************************************************************
Function:     smartPlot_flush_interleaved

Description:  This function will send a plot message that contains all the
              samples in the buffer.

              A plot message will be sent within this function, unless there
              are no samples in the buffer.

Arguments:    These arguments are simply a subset of smartPlot_interleaved,
              see the documentation for smartPlot_interleaved above.

Returns:      None.
*/
static inline void smartPlot_flush_interleaved( const char* plotName,
                                                const char* curveName_x,
                                                const char* curveName_y)
{
   smartPlot_interleaved(NULL, E_INVALID_DATA_TYPE, 0, 0, 0, plotName, curveName_x, curveName_y);
}



/**************************************************************************
Function:     smartPlot_flush_1D

Description:  This function will send a plot message that contains all the
              samples in the buffer.

              A plot message will be sent within this function, unless there
              are no samples in the buffer.

Arguments:    These arguments are simply a subset of smartPlot_1D,
              see the documentation for smartPlot_1D above.

Returns:      None.
*/
static inline void smartPlot_flush_1D( const char* plotName,
                                       const char* curveName )
{
   smartPlot_1D(NULL, E_INVALID_DATA_TYPE, 0, 0, 0, plotName, curveName);
}

/**************************************************************************
Function:     smartPlot_flush_all

Description:  Similar to smartPlot_flush_1D, but this function will flush all
              plot / curve name combinations.

*/
void smartPlot_flush_all();

/**************************************************************************
Function:     smartPlot_deallocate_1D

Description:  If you know you are done with a plot / curve name,
              you can call this function to free up some memory.

              Using this function is probably only necessary if you are
              using many plot / curve name combinations.

*/
void smartPlot_deallocate_1D( const char* plotName,
                              const char* curveName );

/**************************************************************************
Function:     smartPlot_deallocate_interleaved

Description:  If you know you are done with a plot / curve name,
              you can call this function to free up some memory.

              Using this function is probably only necessary if you are
              using many plot / curve name combinations.

*/
void smartPlot_deallocate_interleaved( const char* plotName,
                                       const char* curveName_x,
                                       const char* curveName_y);


/**************************************************************************
Function:     smartPlot_createFlushThread

Description:  Creates a thread that will send plot messages to the PlotGUI.
              This is useful because there is some overhead associated with
              sending plot messages. If the code location where the plot
              data is written is time critical, it is beneficial to actually
              send the plot messages in a background thread. This function
              will create that background thread. The background thread
              will wake up periodically and send all available plot data
              to the PlotGUI.

Arguments:    sleepBetweenFlush_ms - How often to wake up the thread and
              send all available plot data to the PlotGUI. In milliseconds.

Returns:      None.
*/
void smartPlot_createFlushThread(unsigned int sleepBetweenFlush_ms);

/**************************************************************************
Function:     smartPlot_createFlushThread_withPriorityPolicy

Description:  See smartPlot_createFlushThread description. This provides
              the same functionality at smartPlot_createFlushThread, but it
              adds the ability to set the thread priority and policy for the
              newly created thread. Setting thread priority / policy might
              not be valid for all situations.

Arguments:    sleepBetweenFlush_ms - How often to wake up the thread and
              send all available plot data to the PlotGUI. In milliseconds.

              priority - The thread priority of the thread that will be created
              in this function.

              policy - The thread policy to use for the thread the will be created
              in this function.

Returns:      None.
*/
void smartPlot_createFlushThread_withPriorityPolicy(unsigned int sleepBetweenFlush_ms, int priority, int policy);

#ifdef __cplusplus
}
#endif

#endif



/**************************************************
 * END FILE: smartPlotMessage.h
 **************************************************/
