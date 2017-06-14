# --------------------------------------------
# Copyright KAPSARC. Open source MIT License.
# --------------------------------------------

#--------------------------------------
# macros related to list
MACRO(CAR var)
SET(${var} ${ARGV1})
ENDMACRO(CAR)

MACRO(CDR var junk)
SET(${var} ${ARGN})
ENDMACRO(CDR)

MACRO(LIST_INDEX var index)
SET(list . ${ARGN})
FOREACH(i RANGE 1 ${index})
CDR(list ${list})
ENDFOREACH(i)
CAR(${var} ${list})
ENDMACRO(LIST_INDEX)

#--------------------------------------
# to find Qt Root Directory

MACRO(SUBDIRLIST result curdir)

FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)

SET(dirlist "")

FOREACH(child ${children})
if(IS_DIRECTORY ${curdir}/${child})
  string(REGEX MATCH "^Qt([0-9].)*.*" QTVER ${child}) # RE to match Qt5.6.0 or Qt or any Qt Version
  LIST(APPEND dirlist ${QTVER})
endif()
ENDFOREACH()

SET(${result} ${dirlist})

ENDMACRO()

if(WIN32)
  set(HOME_DIR "C:/Qt/")
  SUBDIRLIST(QT_ROOT_DIR "C:/Qt/")   #possible path - windows default installation
  message(STATUS "QT_ROOT_DIR_LIST :" ${QT_ROOT_DIR})

  if(NOT QT_ROOT_DIR)
    set(QT_ROOT_DIR "")
    set(HOME_DIR "D:/Qt/")
    SUBDIRLIST(QT_ROOT_DIR "D:/Qt/")   #possible path
    message(STATUS "QT_ROOT_DIR_LIST :" ${QT_ROOT_DIR})
  endif()

  if(NOT QT_ROOT_DIR)
    set(QT_ROOT_DIR "")
    set(HOME_DIR "E:/Qt/")
    SUBDIRLIST(QT_ROOT_DIR "E:/Qt/")  #possible path
    message(STATUS "QT_ROOT_DIR_LIST :" ${QT_ROOT_DIR})
  endif()
endif(WIN32)

if(UNIX)

  set(HOME_DIR $ENV{HOME})
  SUBDIRLIST(QT_ROOT_DIR $ENV{HOME})     #possible path - ubuntu default installation /home/username
  message(STATUS "QT_ROOT_DIR_LIST :" ${QT_ROOT_DIR})

  if(NOT QT_ROOT_DIR)
    set(HOME_DIR "/usr/lib")
    set(QT_ROOT_DIR "")
    SUBDIRLIST(QT_ROOT_DIR /usr/lib)     #possible path
    message(STATUS "QT_ROOT_DIR_LIST :" ${QT_ROOT_DIR})
  endif()

  if(NOT QT_ROOT_DIR)
    set(HOME_DIR "/usr/local")
    set(QT_ROOT_DIR "")
    SUBDIRLIST(QT_ROOT_DIR /usr/local)   #possible path
    message(STATUS "QT_ROOT_DIR_LIST :" ${QT_ROOT_DIR})
  endif()

  if(NOT QT_ROOT_DIR)
    set(HOME_DIR "/usr/bin")
    set(QT_ROOT_DIR "")
    SUBDIRLIST(QT_ROOT_DIR /usr/bin)   #possible path
    message(STATUS "QT_ROOT_DIR_LIST :" ${QT_ROOT_DIR})
  endif()

  if(NOT QT_ROOT_DIR)
    set(HOME_DIR "/opt")
    set(QT_ROOT_DIR "")
    SUBDIRLIST(QT_ROOT_DIR /opt)     #possible path
    message(STATUS "QT_ROOT_DIR_LIST :" ${QT_ROOT_DIR})
  endif()
endif(UNIX)

if(QT_ROOT_DIR)

  LIST(LENGTH QT_ROOT_DIR len)
  MESSAGE(STATUS "Number of Qt Versions:" ${len})

  LIST_INDEX(QT_PREFIX ${len} ${QT_ROOT_DIR})
  message(STATUS "QT DIR USED :" ${QT_PREFIX})

  #--------------------------------------
  # to find Qt sub version from that directory
  MACRO(QT_SUBDIRLIST result1 curdir1)

  FILE(GLOB children1 RELATIVE ${curdir1} ${curdir1}/*)

  SET(dirlist1 "")
  FOREACH(child1 ${children1})
  if(IS_DIRECTORY ${curdir1}/${child1})
    string(REGEX MATCH "[Qt]*[0-9].+[0-9]*" QTSUBVER ${child1}) # RE to match 5.6 or Qt5.6.0 or any Qt Version
    LIST(APPEND dirlist1 ${QTSUBVER})
  endif()
  ENDFOREACH()
  SET(${result1} ${dirlist1})
  ENDMACRO()

  QT_SUBDIRLIST(QT_VER_DIR ${HOME_DIR}/${QT_PREFIX})

  message(STATUS "QT_VER_DIR_LIST :" ${QT_VER_DIR})

  LIST(LENGTH QT_VER_DIR len)
  MESSAGE(STATUS "Number of Qt Sub Versions:" ${len})

  LIST_INDEX(QT_VER_PREFIX ${len} ${QT_VER_DIR})
  message(STATUS "QT VER DIR USED :" ${QT_VER_PREFIX})

  #--------------------------------------
  # to find Qt choose compiler directory
  MACRO(QT_COMPILER_LIST result2 curdir2)

  FILE(GLOB children2 RELATIVE ${curdir2} ${curdir2}/*)

  SET(dirlist2 "")

  FOREACH(child2 ${children2})
  if(IS_DIRECTORY ${curdir2}/${child2})
    if(WIN32)
      string(REGEX MATCH "^msvc[_0-9]*" QTCOMP ${child2})  #RE to match MSVC compiled libraries by default
    endif(WIN32)
    if(UNIX)
      string(REGEX MATCH "^gcc[_0-9]*" QTCOMP ${child2})  #RE to match GCC compiled libraries by default (64 bit)
    endif(UNIX)

    LIST(APPEND dirlist2 ${QTCOMP})
  endif()
  ENDFOREACH()

  SET(${result2} ${dirlist2})

  ENDMACRO()

  QT_COMPILER_LIST(QT_COMP_DIR ${HOME_DIR}/${QT_PREFIX}/${QT_VER_PREFIX})

  message(STATUS "QT_COMP_DIR_LIST :" ${QT_COMP_DIR})

  LIST(LENGTH QT_COMP_DIR len)
  MESSAGE(STATUS "Number of Qt Compilers:" ${len})

  LIST_INDEX(QT_COM_PATH ${len} ${QT_COMP_DIR})   #len specifies which compiler from the list to use
  message(STATUS "QT COMP DIR USED:" ${QT_COM_PATH})

  if(WIN32)
    set(POSSIBLE_PREFIX_PATH  ${HOME_DIR}${QT_PREFIX}/${QT_VER_PREFIX}/${QT_COM_PATH}/lib/cmake)
    message(STATUS "QT POSSIBLE_PREFIX_PATH:" ${POSSIBLE_PREFIX_PATH})
  endif(WIN32)

  if(UNIX)
    set(POSSIBLE_PREFIX_PATH ${HOME_DIR}/${QT_PREFIX}/${QT_VER_PREFIX}/${QT_COM_PATH}/lib/cmake)
    message(STATUS "QT POSSIBLE_PREFIX_PATH:" ${POSSIBLE_PREFIX_PATH})
  endif(UNIX)

  find_path(CMAKE_DIR Qt5Config.cmake PATHS ${POSSIBLE_PREFIX_PATH}/Qt5)

endif()

#--------------------------------------------------------------
#if root directory is not found
if(NOT ${CMAKE_DIR})
  if(WIN32)
    #set other possible paths here
    set(PREFIX_PATH   ${POSSIBLE_PREFIX_PATH}
      )
  endif(WIN32)

  if(UNIX)
    #set other possible paths here
    set(PREFIX_PATH     ${POSSIBLE_PREFIX_PATH}
      )
  endif(UNIX)
endif()

# --------------------------------------------
# Copyright KAPSARC. Open source MIT License.
# --------------------------------------------
