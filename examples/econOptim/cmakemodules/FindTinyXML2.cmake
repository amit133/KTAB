# --------------------------------------------
# Copyright KAPSARC. Open source MIT License.
# --------------------------------------------
# TINYXML2_FOUND
# TINYXML2_INCLUDE_DIR
# TINYXML2_LIBRARIES

set(TINYXML2_FOUND "NO")

# Places to look, under Linux and Windows.
set (TINYXML2_INSTALL_DIR)
if (WIN32)
  set (TINYXML2_INSTALL_DIR
  c:/local/tinyxml2
  )
endif(WIN32)
if (UNIX)
  set (TINYXML2_INSTALL_DIR
  /usr/local/tinyxml2
  )
endif(UNIX)

set(TINYXML2_PREFIX
  ${TINYXML2_INSTALL_DIR} 
  ) 

set(TINYXML2_POSSIBLE_PATHS
  ${TINYXML2_PREFIX}/include/
  ${TINYXML2_PREFIX}/lib/
  )

# try to find a key header
find_path(TINYXML2_INCLUDE_DIR tinyxml2.h 
  PATHS ${TINYXML2_POSSIBLE_PATHS}
  )


# try to find the compiled library object
find_library (TINYXML2_LIBRARY NAMES tinyxml2 
  PATHS ${TINYXML2_POSSIBLE_PATHS}
  )

# if the library object was found, record it.
# may need to add other system files later.
if(TINYXML2_LIBRARY)
  set(TINYXML2_LIBRARIES  ${TINYXML2_LIBRARY})
  message(STATUS "Found TinyXML2 library: ${TINYXML2_LIBRARY}")
endif(TINYXML2_LIBRARY)


if(TINYXML2_INCLUDE_DIR) 
  message(STATUS "Found TinyXML2 headers: ${TINYXML2_INCLUDE_DIR}")
endif(TINYXML2_INCLUDE_DIR)

if(TINYXML2_LIBRARIES AND TINYXML2_INCLUDE_DIR)
  set(TINYXML2_FOUND "YES")
endif(TINYXML2_LIBRARIES AND TINYXML2_INCLUDE_DIR)

# if not found, stop immediately
if(NOT TINYXML2_FOUND)
  message(FATAL_ERROR "Could not find both TinyXML2 library and headers")
endif(NOT TINYXML2_FOUND)	

# --------------------------------------------
# Copyright KAPSARC. Open source MIT License.
# --------------------------------------------
