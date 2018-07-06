# --------------------------------------------
# Copyright KAPSARC. Open source MIT License.
# --------------------------------------------
# ALGLIB_FOUND
# ALGLIB_INCLUDE_DIR
# ALGLIB_LIBRARIES

set(ALGLIB_FOUND "NO")

# Places to look, under Linux and Windows.
set (ALGLIB_INSTALL_DIR)
if (WIN32)
  set (ALGLIB_INSTALL_DIR
  c:/local/alglib
  )
endif(WIN32)
if (UNIX)
  set (ALGLIB_INSTALL_DIR
  /usr/local/alglib
  )
endif(UNIX)

set(ALGLIB_PREFIX
  ${ALGLIB_INSTALL_DIR} 
  ) 

set(ALGLIB_POSSIBLE_PATHS
  ${ALGLIB_PREFIX}/include/
  ${ALGLIB_PREFIX}/lib/
  )

set(ALGLIB_INCLUDE_DIR
  ${ALGLIB_PREFIX}/include/
)

# try to find the compiled library object
find_library (ALGLIB_LIBRARY NAMES alglib 
  PATHS ${ALGLIB_POSSIBLE_PATHS}
  )

# if the library object was found, record it.
# may need to add other system files later.
if(ALGLIB_LIBRARY)
  set(ALGLIB_LIBRARIES  ${ALGLIB_LIBRARY})
  message(STATUS "Found Alglib library: ${ALGLIB_LIBRARY}")
endif(ALGLIB_LIBRARY)

if(ALGLIB_LIBRARIES)
  set(ALGLIB_FOUND "YES")
endif(ALGLIB_LIBRARIES)

# if not found, stop immediately
if(NOT ALGLIB_FOUND)
  message(FATAL_ERROR "Could not find Alglib library")
endif(NOT ALGLIB_FOUND)	

# --------------------------------------------
# Copyright KAPSARC. Open source MIT License.
# --------------------------------------------
