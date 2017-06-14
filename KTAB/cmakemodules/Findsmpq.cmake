# --------------------------------------------
# Copyright KAPSARC. Open source MIT License.
# --------------------------------------------

set(SMPQ_FOUND "NO")

#------------------------------------------------------------------
set(SMPQ_POSSIBLE_PATHS
  ${CMAKE_CURRENT_SOURCE_DIR}/SMPQ/
  ${CMAKE_CURRENT_SOURCE_DIR}/../SMPQ/
  ${CMAKE_CURRENT_SOURCE_DIR}/../examples/SMPQ/
  ${CMAKE_CURRENT_SOURCE_DIR}/../../SMPQ/
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../SMPQ/
  ${PROJECT_SOURCE_DIR}/qcustomplot/
  ${PROJECT_SOURCE_DIR}/../qcustomplot/
  ${PROJECT_SOURCE_DIR}/../../qcustomplot/
  ${PROJECT_SOURCE_DIR}/../../../qcustomplot/
  )

#------------------------------------------------------------------
# Finding SMPQ directory - with mainwindow.h header as refrence
find_path(SMPQ_SOURCE_DIR colorpickerdialog.h PATHS ${SMPQ_POSSIBLE_PATHS})

if(SMPQ_SOURCE_DIR)
  message(STATUS "Found SMPQ source directory: ${SMPQ_SOURCE_DIR}")

  #manually adding files to the variables
  set (QtProjectLib_src
    ${SMPQ_SOURCE_DIR}/mainwindow.cpp
    ${SMPQ_SOURCE_DIR}/csv.cpp
    ${SMPQ_SOURCE_DIR}/database.cpp
    ${SMPQ_SOURCE_DIR}/bargraph.cpp
    ${SMPQ_SOURCE_DIR}/linegraph.cpp
    ${SMPQ_SOURCE_DIR}/runsmp.cpp
    ${SMPQ_SOURCE_DIR}/quadmap.cpp
    ${SMPQ_SOURCE_DIR}/xmlwidget.cpp
    ${SMPQ_SOURCE_DIR}/xmlparser.cpp
    ${SMPQ_SOURCE_DIR}/colorpickerdialog.cpp
    ${SMPQ_SOURCE_DIR}/popupwidget.cpp
    ${SMPQ_SOURCE_DIR}/databasedialog.cpp)

  set (QtProjectLib_hdr
    ${SMPQ_SOURCE_DIR}/mainwindow.h
    ${SMPQ_SOURCE_DIR}/csv.h
    ${SMPQ_SOURCE_DIR}/database.h
    ${SMPQ_SOURCE_DIR}/xmlparser.h
    ${SMPQ_SOURCE_DIR}/colorpickerdialog.h
    ${SMPQ_SOURCE_DIR}/popupwidget.h
    ${SMPQ_SOURCE_DIR}/databasedialog.h)

  set (QtProjectLib_ui  ${SMPQ_SOURCE_DIR}/mainwindow.ui ${SMPQ_SOURCE_DIR}/popupwidget.ui ${SMPQ_SOURCE_DIR}/databasedialog.ui)
  set (QtProjectRsc_qrc ${SMPQ_SOURCE_DIR}/dockwidgets.qrc)
  set (QtProjectBin_src ${SMPQ_SOURCE_DIR}/main.cpp)

endif(SMPQ_SOURCE_DIR)

if(NOT SMPQ_SOURCE_DIR)
  message(FATAL_ERROR "Could not find SMPQ source directory")
endif(NOT SMPQ_SOURCE_DIR)

#------------------------------------------------------------------
# Finding QCustomPlot directory - with qcustomplot.h header as refrence
find_path(QCUSTOMPLOT_DIR qcustomplot.h  PATHS ${SMPQ_POSSIBLE_PATHS})

if(QCUSTOMPLOT_DIR)
  message(STATUS "Found QCUSTOMPLOT source directory: ${QCUSTOMPLOT_DIR}")
  set (QtProjectLib_src  ${QtProjectLib_src} ${QCUSTOMPLOT_DIR}/qcustomplot.cpp)
  set (QtProjectLib_hdr  ${QtProjectLib_hdr} ${QCUSTOMPLOT_DIR}/qcustomplot.h)
endif(QCUSTOMPLOT_DIR)

#csv parser file from smp
set (QtProjectLib_src  ${QtProjectLib_src})

if(NOT QCUSTOMPLOT_DIR)
  message(FATAL_ERROR "Could not find QCUSTOMPLOT directory")
endif(NOT QCUSTOMPLOT_DIR)

#------------------------------------------------------------------
# Checking availibity of all required resources
if(SMPQ_SOURCE_DIR AND QCUSTOMPLOT_DIR )
  set(SMPQ_FOUND "YES")
endif(SMPQ_SOURCE_DIR AND QCUSTOMPLOT_DIR)

# if not found, stop immediately
if(NOT SMPQ_FOUND)
  message(FATAL_ERROR "Required Source files are not found for SMPQ")
endif(NOT SMPQ_FOUND)

# --------------------------------------------
# Copyright KAPSARC. Open source MIT License.
# --------------------------------------------
