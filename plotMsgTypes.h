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
#ifndef plotMsgTypes_h
#define plotMsgTypes_h

#if (defined(_WIN32) || defined(__WIN32__))
#define PLOTTER_WINDOWS_BUILD
#endif

// Boolean Definition
typedef char PLOTTER_BOOL;
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

typedef signed char PLOTTER_SCHAR;
typedef unsigned char PLOTTER_UCHAR;
typedef signed char PLOTTER_INT_8;
typedef unsigned char PLOTTER_UINT_8;
typedef signed short PLOTTER_INT_16;
typedef unsigned short PLOTTER_UINT_16;
typedef signed int PLOTTER_INT_32;
typedef unsigned int PLOTTER_UINT_32;
typedef signed long long PLOTTER_INT_64;
typedef unsigned long long PLOTTER_UINT_64;
typedef float PLOTTER_FLOAT_32;
typedef double PLOTTER_FLOAT_64;

#endif
