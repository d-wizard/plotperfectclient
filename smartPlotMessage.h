/**************************************************************************
 * Copyright 2017 - 2019, 2021 - 2022 Dan Williams. All Rights Reserved.
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


#if (defined(_WIN32) || defined(__WIN32__)) && (!defined inline) && (!defined __cplusplus)
   #define inline __inline // Windows doesn't use the inline keyword, instead it uses __inline
#endif

#if (defined(_WIN32) || defined(__WIN32__))
#define TIME_PLOT_WINDOWS
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

// Time struct may not always exist. Abstract it with tSmartPlotTime.
#ifdef TIME_PLOT_WINDOWS
typedef struct
{
   long long tv_sec;
   long long tv_nsec;
}tSmartPlotTime;
#else
typedef struct timespec tSmartPlotTime;
#endif
#define E_TIME_STRUCT_AUTO (sizeof(tSmartPlotTime) <= 8 ? E_TIME_STRUCT_64 : E_TIME_STRUCT_128) // This can be used when size of timespec is unknown.

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
Function:     smartPlot_forceBackgroundThread

Description:  Forces plot messages to be send from a background thread. This
              is useful to reduce the impact of plotting data from high priority
              threads.

              This does the same thing as defining PLOTTER_FORCE_BACKGROUND_THREAD
              at compile time. 

Arguments:    None.

Returns:      None.
*/
void smartPlot_forceBackgroundThread();

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
Function:     smartPlot_2D

Description:  Pretty much the same as smartPlot_1D, but this function takes
              in separate input data to plot for both X and Y axes and
              generates a 2D plot.
*/
void smartPlot_2D( const void* inDataToPlotX,
                   ePlotDataTypes inDataTypeX,
                   const void* inDataToPlotY,
                   ePlotDataTypes inDataTypeY,
                   int inDataSize,
                   int plotSize,
                   int updateSize,
                   const char* plotName,
                   const char* curveName );


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
Function:     smartPlot_flush_2D

Description:  This function will send a plot message that contains all the
              samples in the buffer.

              A plot message will be sent within this function, unless there
              are no samples in the buffer.

Arguments:    These arguments are simply a subset of smartPlot_2D,
              see the documentation for smartPlot_1D above.

Returns:      None.
*/
static inline void smartPlot_flush_2D( const char* plotName,
                                       const char* curveName )
{
   smartPlot_2D(NULL, E_INVALID_DATA_TYPE, NULL, E_INVALID_DATA_TYPE, 0, 0, 0, plotName, curveName);
}

/**************************************************************************
Function:     smartPlot_flush_all

Description:  Similar to smartPlot_flush_1D, but this function will flush all
              plot / curve name combinations.

*/
void smartPlot_flush_all();

/**************************************************************************
Function:     smartPlot_deallocate

Description:  If you know you are done with a plot / curve name,
              you can call this function to free up some memory.

              Using this function is probably only necessary if you are
              using many plot / curve name combinations.

*/
void smartPlot_deallocate( const char* plotName,
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

/**************************************************************************
Function:     smartPlot_getTime

Description:  Fills in the structure passed in with the current time.
              This is Monotonic / Steady Clock time (i.e. time since boot).

Arguments:    pTime (Out) - Pointer to the time struct that will be filled in.

Returns:      None.
*/
void smartPlot_getTime(tSmartPlotTime* pTime);

#ifdef __cplusplus
}
#endif

#endif



/**************************************************
 * END FILE: smartPlotMessage.h
 **************************************************/
