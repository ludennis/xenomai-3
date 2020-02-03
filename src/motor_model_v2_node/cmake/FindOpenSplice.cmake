FIND_PATH(OpenSplice_INCLUDE_DIR
  NAMES
    make_files.py
  PATHS
    $ENV{OSPL_HOME}/include
)

SET(OpenSplice_INCLUDE_DIRS
  $ENV{OSPL_HOME}/include
  $ENV{OSPL_HOME}/include/sys
  $ENV{OSPL_HOME}/include/dcps/C++/isocpp2
  $ENV{OSPL_HOME}/include/dcps/C++/SACPP
)

# find libraries
FIND_LIBRARY(KERNEL_LIBRARY
  NAMES
    ddskernel
  PATHS
    $ENV{OSPL_HOME}/lib
)

FIND_LIBRARY(DCPSISOCPP_LIBRARY
  NAMES
    dcpsisocpp2
  PATHS
    $ENV{OSPL_HOME}/lib
)

message("DCPSCPP_LIBRARY: ${DCPSCPP_LIBRARY}")

SET(OpenSplice_LIBRARIES
  ${KERNEL_LIBRARY}
  ${DCPSISOCPP_LIBRARY}
  ${DCPSCPP_LIBRARY}
)

# Binary for the IDL compiler
SET(OpenSplice_IDLGEN_BINARY $ENV{OSPL_HOME}/bin/idlpp)
IF(OpenSplice_INCLUDE_DIRS AND OpenSplice_LIBRARIES)
  SET(OpenSplice_FOUND TRUE)
ENDIF(OpenSplice_INCLUDE_DIRS AND OpenSplice_LIBRARIES)

IF(OpenSplice_FOUND)
  MESSAGE(STATUS "Found OpenSplice DDS libraries: ${OpenSplice_LIBRARIES}")
ELSE(OpenSplice_FOUND)
  IF(OpenSplice_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find OpenSplice DDS")
  ENDIF(OpenSplice_FIND_REQUIRED)
ENDIF(OpenSplice_FOUND)

MARK_AS_ADVANCED(OpenSplice_INCLUDE_DIRS OpenSplice_LIBRARIES OpenSplice_IDLGEN_BINARY)
INCLUDE(MacroOpenSplice)