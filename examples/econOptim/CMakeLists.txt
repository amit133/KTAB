# --------------------------------------------
# Copyright KAPSARC. Open source MIT License.
# --------------------------------------------
# The MIT License (MIT)
# 
# Copyright (c) 2015 King Abdullah Petroleum Studies and Research Center
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software
# and associated documentation files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom 
# the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or
# substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING 
# BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
# DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# -------------------------------------------------

project(econOptim)

cmake_minimum_required(VERSION 2.8)
cmake_policy(VERSION 2.8)
set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  )

set (LOCAL_MODULE_DIR
    ${PROJECT_SOURCE_DIR}/../../KTAB/cmakemodules)

set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${LOCAL_MODULE_DIR}
    )

set(CMAKE_BUILD_TYPE Debug)

set(LIBRARY_OUTPUT_PATH      ${PROJECT_SOURCE_DIR}/)
set(EXECUTABLE_OUTPUT_PATH   ${PROJECT_SOURCE_DIR}/)

# -------------------------------------------------
# basic check that this is a supported compiler/OS
# -------------------------------------------------
if (WIN32)
  message (STATUS "OS: WIN32 (generic Windows)")
  if (MSVC)
    message (STATUS "Compiler: MSVC")
  elseif(CMAKE_COMPILER_IS_GNUCXX)
    message (STATUS "Compiler: GCC")
  else(MSVC)
    message(SEND_ERROR "Unsupported Windows compiler")
  endif()
elseif(UNIX)
  message (STATUS "OS: UNIX (generic *nix)")
else()
  message(SEND_ERROR "Unsupported OS")
endif(WIN32)

# -------------------------------------------------
find_package(TinyXML2)
if (NOT TINYXML2_FOUND)
    message(FATAL_ERROR "Could not find TinyXML2")
endif (NOT TINYXML2_FOUND) 

# -------------------------------------------------
find_package(Alglib)

# -------------------------------------------------
find_package(Exprtk)

#--------------------------------------------------
find_package(qtlibs)
set(CMAKE_PREFIX_PATH ${PREFIX_PATH})
message(STATUS "CMAKE_PREFIX_PATH" ${CMAKE_PREFIX_PATH})
find_package(Qt5 REQUIRED COMPONENTS Core Sql)


# --------------------------------------------------------
# See "Findkutils.cmake" in cmakemodules to figure
# out where it looks and what variables it sets
find_package(kutils)
if(NOT KUTILS_FOUND)
    message(FATAL_ERROR "Could not find kutils")
endif(NOT KUTILS_FOUND)


# Ditto for "Findkmodel.cmake"
find_package(kmodel)
if(NOT KMODEL_FOUND)
    message(FATAL_ERROR "Could not find kmodel")
endif(NOT KMODEL_FOUND)

# -------------------------------------------------
find_package(Easyloggingpp)

#Enable export of all classes and functions for windows platform
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# -------------------------------------------------
# files to be built into a library go in libsrc/
# any application or testing code goes in src

# -------------------------------------------------
# econOptim app program

include_directories(
  ${KUTILS_INCLUDE_DIR}
  ${KMODEL_INCLUDE_DIR}
  ${PROJECT_SOURCE_DIR}/libsrc/
  ${TINYXML2_INCLUDE_DIR}
  ${ALGLIB_INCLUDE_DIR}
  ${EXPRTK_INCLUDE_DIR}
  ${LOGGER_INCLUDE_DIR}
)

set(ECONOPTIM_LIB_SRCS
  ${PROJECT_SOURCE_DIR}/libsrc/EconOptimizer.cpp
  ${PROJECT_SOURCE_DIR}/libsrc/XMLParser.cpp
  ${PROJECT_SOURCE_DIR}/libsrc/EconOptimModel.cpp
 )

add_library(econoptim STATIC ${ECONOPTIM_LIB_SRCS})

target_link_libraries(econoptim
  ${KMODEL_LIBRARY}
  ${KUTILS_LIBRARY}
  ${ALGLIB_LIBRARIES}
  ${TINYXML2_LIBRARIES}
  ${LOGGER_LIBRARY}
    Qt5::Sql
 )

set(ECONOPTIM_DEMO_SRCS
  ${PROJECT_SOURCE_DIR}/src/minlm_d_v_example.cpp
 )

add_executable(demoEconOptim
  ${ECONOPTIM_DEMO_SRCS}
  )

target_link_libraries(demoEconOptim
  econoptim
  ${KMODEL_LIBRARY}
  ${KUTILS_LIBRARY}
  ${ALGLIB_LIBRARIES}
  ${TINYXML2_LIBRARIES}
  ${LOGGER_LIBRARY}
    Qt5::Sql
 )
# -------------------------------------------------
# show some useful status/debugging information

message(STATUS "CMAKE_SYSTEM_NAME: " ${CMAKE_SYSTEM_NAME} )
message(STATUS "Using PROJECT_SOURCE_DIR: " ${PROJECT_SOURCE_DIR})
message(STATUS "ALGLIB_INSTALL_DIR: " ${ALGLIB_INSTALL_DIR})

# -------------------------------------------------
# As of early 2014, C++11 is still not the default
# for g++, so I have to provide it here.
if (UNIX OR MINGW)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++11 -Dexprtk_disable_caseinsensitivity")
endif(UNIX OR MINGW)

if(WIN32)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj /Dexprtk_disable_caseinsensitivity")
endif(WIN32)

# --------------------------------------------
# Copyright KAPSARC. Open source MIT License.
# --------------------------------------------

