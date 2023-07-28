cmake_minimum_required(VERSION 3.11)

set(projName plotperfectclient)
project(${projName})

# Flags for C and C++
set(c_cppFlags
   -O2
   -Wall
   -Werror
   -fdiagnostics-color=always)

# Flags for just C++
set(cppOnlyFlags
   -std=c++11)

# Pre-processor directives
set(defines
   )

# Include paths
set(includes
   )

# Source files
set(source
   sendMemoryToPlot.cpp
   smartPlotMessage.cpp)

# Build the library
add_library(${projName} SHARED ${source})

# Specify Flags, defines, and includes
target_compile_options(${projName} PRIVATE ${c_cppFlags})
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${cppOnlyFlags})
target_compile_definitions(${projName} PRIVATE ${defines})
target_include_directories(${projName} PRIVATE ${includes})

