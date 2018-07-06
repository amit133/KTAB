# --------------------------------------------
# Copyright KAPSARC. Open source MIT License.
# --------------------------------------------
# EXPRTK_FOUND
# EXPRTK_INCLUDE_DIR

set(EXPRTK_FOUND "NO")

# Places to look, under Linux and Windows.
set (EXPRTK_INSTALL_DIR)
if (WIN32)
  set (EXPRTK_INSTALL_DIR
  c:/local/exprtk
  )
endif(WIN32)
if (UNIX)
  set (EXPRTK_INSTALL_DIR
  /usr/local/exprtk
  )
endif(UNIX)

set(EXPRTK_PREFIX
  ${EXPRTK_INSTALL_DIR} 
  ) 

set(EXPRTK_POSSIBLE_PATHS
  ${EXPRTK_PREFIX}/include/
  )

# try to find the header which holds the complete source code of exprtk library
find_path(EXPRTK_INCLUDE_DIR exprtk.hpp
  PATHS ${EXPRTK_POSSIBLE_PATHS}
  )

if(EXPRTK_INCLUDE_DIR) 
  message(STATUS "Found Exprtk header: ${EXPRTK_INCLUDE_DIR}/exprtk.hpp")
  set(EXPRTK_FOUND "YES")
endif(EXPRTK_INCLUDE_DIR)


# if not found, stop immediately
if(NOT EXPRTK_FOUND)
  message(FATAL_ERROR "Could not find Exprtk header file")
endif(NOT EXPRTK_FOUND)	

# --------------------------------------------
# Copyright KAPSARC. Open source MIT License.
# --------------------------------------------
