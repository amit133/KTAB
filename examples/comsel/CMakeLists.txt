# ===========================================
# Copyright KAPSARC. Open Source MIT License 
# ===========================================

project(comsel)

cmake_minimum_required (VERSION 2.8)
cmake_policy(VERSION 2.8)

set (LOCAL_MODULE_DIR
  ${PROJECT_SOURCE_DIR}/../../KTAB/cmakemodules)

set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  ${LOCAL_MODULE_DIR}
  )


set(CMAKE_BUILD_TYPE Debug)

set(LIBRARY_OUTPUT_PATH      ${PROJECT_SOURCE_DIR}/)
set(EXECUTABLE_OUTPUT_PATH   ${PROJECT_SOURCE_DIR}/)

if (UNIX)
  set (ENABLE_EFFCPP false CACHE  BOOL "Check Effective C++ Guidelines")
  set (ENABLE_EFENCE false CACHE  BOOL "Use Electric Fence memory debugger")
endif(UNIX)

# -------------------------------------------------

if (ENABLE_EFENCE)
  find_package(efence)
  if (NOT EFENCE_FOUND)
    message(FATAL_ERROR "Could not find Electric Fence memory debugger")
  endif (NOT EFENCE_FOUND)
endif (ENABLE_EFENCE)

# -------------------------------------------------
 
find_package(TinyXML2)
if (NOT TINYXML2_FOUND)
  message(FATAL_ERROR "Could not find TinyXML2")
endif (NOT TINYXML2_FOUND) 

# --------------------------------------------------------
find_package(Sqlite)
if (NOT SQLITE_FOUND)
  message(FATAL_ERROR "Could not find SQLite")
endif (NOT SQLITE_FOUND)

#--------------------------------------------------
find_package(qtlibs)
set(CMAKE_PREFIX_PATH ${PREFIX_PATH})
message(STATUS "CMAKE_PREFIX_PATH" ${CMAKE_PREFIX_PATH})

find_package(Qt5 REQUIRED COMPONENTS Core Sql)

# --------------------------------------------------------

set (COMSEL_XLIBS  )

if(UNIX)
  find_package(X11)
  if (NOT X11_FOUND)
    message (FATAL_ERROR "Couldn't find X11: required for building csg")
  endif (NOT X11_FOUND)
  set (COMSEL_XLIBS
    #           Xaw Xmu Xpm Xt Xft X11 Xinerama dl fontconfig
    # Xext Xft fontconfig Xinerama dl m X11
    Xext Xft fontconfig dl m X11  Xfixes 
    # Xft dl m X11
    )
endif (UNIX)



# --------------------------------------------------------

if (ENABLE_EFENCE)
  find_package(efence)
  if (NOT EFENCE_FOUND)
    message(FATAL_ERROR "Could not find Electric Fence memory debugger")
  endif (NOT EFENCE_FOUND)
endif (ENABLE_EFENCE)

# ------------------------------------------------- 
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
# Ditto for "Findsmp.cmake"
find_package(smp)
if(NOT SMP_FOUND)
  message(FATAL_ERROR "Could not find smp")
endif(NOT SMP_FOUND)
# -------------------------------------------------
# files to be built into a library go in libsrc/
# any application or testing code goes in src

include_directories (
  ${KUTILS_INCLUDE_DIR}
  ${KMODEL_INCLUDE_DIR}
  ${SMP_INCLUDE_DIR}
  ${PROJECT_SOURCE_DIR}/libsrc/
  ${PROJECT_SOURCE_DIR}/src/
  ${SQLITE_INCLUDE_DIR}
  ${TINYXML2_INCLUDE_DIR}
  )


set(COMSELLIB_SRCS
  ${PROJECT_SOURCE_DIR}/libsrc/comsel.cpp
  )

add_library(comsel STATIC ${COMSELLIB_SRCS})

target_link_libraries (comsel
  Qt5::Core
  Qt5::Sql
  )

# -------------------------------------------------

set (CSG_SRCS
  ${PROJECT_SOURCE_DIR}/src/democomsel.cpp
  )

set (CSG_LIBS
    ${COMSEL_XLIBS}
  )

add_executable (csg 
  ${CSG_SRCS}
)
target_link_libraries(csg
  comsel
  ${CSG_LIBS}
  ${SMP_LIBRARIES}
  ${KMODEL_LIBRARIES}
  ${KUTILS_LIBRARIES}
  ${SQLITE_LIBRARIES}
  ${EFENCE_LIBRARIES}
  ${TINYXML2_LIBRARIES}
  )
  
if (WIN32)  
  if (MSVC)  
    set_target_properties(csg PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
    message (STATUS "Using /SUBSYSTEM:CONSOLE") 
  endif(MSVC) 
endif(WIN32)

# -------------------------------------------------
# show some useful status/debugging information
message (STATUS "Using PROJECT_SOURCE_DIR: " ${PROJECT_SOURCE_DIR})
message (STATUS "SMP_INCLUDE_DIR: " ${SMP_INCLUDE_DIR})
message (STATUS "KMODEL_INCLUDE_DIR: " ${KMODEL_INCLUDE_DIR})
message (STATUS "KUTILS_INCLUDE_DIR: " ${KUTILS_INCLUDE_DIR})

#message (STATUS "SMP_LIBRARIES: " ${SMP_LIBRARIES})
#message (STATUS "KMODEL_LIBRARIES: " ${KMODEL_LIBRARIES})
#message (STATUS "KUTILS_LIBRARIES: " ${KUTILS_LIBRARIES})

# -------------------------------------------------
# As of early 2014, C++11 is still not the default
# for g++, so I have to provide it here.
if (UNIX OR MINGW)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++11 ")
  if (ENABLE_EFFCPP)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weffc++ ")
  endif (ENABLE_EFFCPP)
endif(UNIX OR MINGW)

# ===========================================
# Copyright KAPSARC. Open Source MIT License 
# ===========================================
