# Provide an interface for calling Smart Plot Message functionality from the Plot Perfect Client library.
from ctypes import *
import os
import struct

################################################################################
E_INT_8             = 0
E_UINT_8            = 1
E_INT_16            = 2
E_UINT_16           = 3
E_INT_32            = 4
E_UINT_32           = 5
E_INT_64            = 6
E_UINT_64           = 7
E_FLOAT_32          = 8
E_FLOAT_64          = 9
E_TIME_STRUCT_64    = 10
E_TIME_STRUCT_128   = 11
E_INVALID_DATA_TYPE = 12

################################################################################
plotLib = None

################################################################################
def _strToBytes(input):
   return bytes(input, 'utf-8')

################################################################################

def _plotterInit():
   global plotLib
   if plotLib != None:
      return
   else:
      plotterLibFileName = 'libplotperfectclient.so'

      # define function for attempting to load the library.
      def tryToLoadLib(pathToLib):
         global plotLib
         try:
            print(pathToLib)
            plotLib = cdll.LoadLibrary(pathToLib)
         except:
            pass

      # Check if the libary is properly installed.
      if plotLib == None:
         tryToLoadLib(plotterLibFileName) # try to find the library in a standard location
      
      # If not properly installed, check some obvious locations.
      if plotLib == None:
         thisFileDir = os.path.dirname(os.path.realpath(__file__))
         altPathsToTry = [
            os.getenv('PLOTTER_LIB_DIR'), # try to find it in a directory specified via environment variable
            os.path.join(thisFileDir, '.build'), # try to find it in a common CMake build location
            os.path.join(thisFileDir, 'build'), # try to find it in another common CMake build location
            os.path.join('~', '.plotter'),  # try to find it in the .plotter folder in the user directory
            os.path.join(thisFileDir), # try to find it in the same directory as this file is in.
            os.path.join(os.getcwd()), # try to find it in the cwd
            '.', # try to find it in this directory
            os.getenv('HOME'), # try to find it in the home directory
            '..', # try to find it up 1 directory
            os.path.join('..', '..') # try to find it up 2 directories
         ]
         for altPath in altPathsToTry:
            try:
               tryToLoadLib(os.path.join(altPath, plotterLibFileName))
            except:
               pass
            if plotLib != None:
               return
   
   if plotLib == None:
      raise Exception("Failed to load plotter library.")

################################################################################

def networkConfigure(hostName: str, port: int):
   global plotLib
   _plotterInit()
   plotLib.smartPlot_networkConfigure(_strToBytes(hostName), port)

################################################################################

def forceBackgroundThread():
   global plotLib
   _plotterInit()
   plotLib.smartPlot_forceBackgroundThread()

################################################################################

def plotInterleaved( 
            inDataToPlot: bytes,
            inDataType: int,
            inDataSize: int,
            plotSize: int,
            updateSize: int,
            plotName: str,
            curveName_x: str,
            curveName_y: str):
   global plotLib
   _plotterInit()
   plotLib.smartPlot_interleaved(
      bytes(inDataToPlot),
      int(inDataType),
      int(inDataSize),
      int(plotSize),
      int(updateSize),
      _strToBytes(plotName),
      _strToBytes(curveName_x),
      _strToBytes(curveName_y)
   )

################################################################################

def plot1D( 
            inDataToPlot: bytes,
            inDataType: int,
            inDataSize: int,
            plotSize: int,
            updateSize: int,
            plotName: str,
            curveName: str):
   global plotLib
   _plotterInit()
   plotLib.smartPlot_1D(
      bytes(inDataToPlot),
      int(inDataType),
      int(inDataSize),
      int(plotSize),
      int(updateSize),
      _strToBytes(plotName),
      _strToBytes(curveName)
   )

################################################################################

def plot2D( 
            inDataToPlotX: bytes,
            inDataTypeX: int,
            inDataToPlotY: bytes,
            inDataTypeY: int,
            inDataSize: int,
            plotSize: int,
            updateSize: int,
            plotName: str,
            curveName: str):
   global plotLib
   _plotterInit()
   plotLib.smartPlot_2D(
      bytes(inDataToPlotX),
      int(inDataTypeX),
      bytes(inDataToPlotY),
      int(inDataTypeY),
      int(inDataSize),
      int(plotSize),
      int(updateSize),
      _strToBytes(plotName),
      _strToBytes(curveName)
   )

################################################################################

def groupMsgStart():
   global plotLib
   _plotterInit()
   plotLib.smartPlot_groupMsgStart()

################################################################################

def groupMsgEnd():
   global plotLib
   _plotterInit()
   plotLib.smartPlot_groupMsgEnd()

################################################################################

def flush_interleaved(
            plotName: str,
            curveName_x: str,
            curveName_y: str):
   global plotLib
   _plotterInit()
   plotLib.smartPlot_interleaved(
      0,
      E_INVALID_DATA_TYPE,
      0,
      0,
      0,
      _strToBytes(plotName),
      _strToBytes(curveName_x),
      _strToBytes(curveName_y)
   )

################################################################################

def flush_1D( 
            plotName: str,
            curveName: str):
   global plotLib
   _plotterInit()
   plotLib.smartPlot_1D(
      0,
      E_INVALID_DATA_TYPE,
      0,
      0,
      0,
      _strToBytes(plotName),
      _strToBytes(curveName)
   )

################################################################################

def flush_2D( 
            plotName: str,
            curveName: str):
   global plotLib
   _plotterInit()
   plotLib.smartPlot_2D(
      0,
      E_INVALID_DATA_TYPE,
      0,
      E_INVALID_DATA_TYPE,
      0,
      0,
      0,
      _strToBytes(plotName),
      _strToBytes(curveName)
   )

################################################################################

def flush_all():
   global plotLib
   _plotterInit()
   plotLib.smartPlot_flush_all()

################################################################################

def deallocate( 
            plotName: str,
            curveName: str):
   global plotLib
   _plotterInit()
   plotLib.smartPlot_deallocate(
      _strToBytes(plotName),
      _strToBytes(curveName)
   )

################################################################################

def deallocate_interleaved(
            plotName: str,
            curveName_x: str,
            curveName_y: str):
   global plotLib
   _plotterInit()
   plotLib.smartPlot_deallocate_interleaved(
      _strToBytes(plotName),
      _strToBytes(curveName_x),
      _strToBytes(curveName_y)
   )

################################################################################

def createFlushThread(sleepBetweenFlush_ms: int):
   global plotLib
   _plotterInit()
   plotLib.smartPlot_createFlushThread(sleepBetweenFlush_ms)

################################################################################

def createFlushThread_withPriorityPolicy(sleepBetweenFlush_ms: int, priority: int, policy: int = 1):
   global plotLib
   _plotterInit()
   plotLib.smartPlot_createFlushThread_withPriorityPolicy(sleepBetweenFlush_ms)

################################################################################

def plotList_1D(plotList, plotSize: int, updateSize: int, plotName: str, curveName: str):
   if len(plotList) > 0:
      if type(plotList[0]) is int:
         plotBytes = bytes()
         for val in plotList:
            plotBytes += val.to_bytes(8, 'little', signed=True) # Store as signed 64 bit
         plot1D(plotBytes, E_INT_64, len(plotList), plotSize, updateSize, plotName, curveName)
      elif type(plotList[0]) is float:
         plotBytes = bytes()
         for val in plotList:
            plotBytes += struct.pack('d', val)
         plot1D(plotBytes, E_FLOAT_64, len(plotList), plotSize, updateSize, plotName, curveName)

