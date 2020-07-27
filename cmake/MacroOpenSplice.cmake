MACRO(DEFINE_OpenSplice_SOURCES idlfilename)
  SET(outsources)
  message("idlfilename: ${idlfilename}")
  GET_FILENAME_COMPONENT(it ${idlfilename} ABSOLUTE)
  GET_FILENAME_COMPONENT(idl_dir ${idlfilename} DIRECTORY)
  message("idl_dir: ${idl_dir}")
  GET_FILENAME_COMPONENT(nfile ${idlfilename} NAME_WE)
  SET(outsources ${outsources} ${idl_dir}/gen/${nfile}.cpp ${idl_dir}/gen/${nfile}.h)
  SET(outsources ${outsources} ${idl_dir}/gen/${nfile}_DCPS.cpp ${idl_dir}/gen/${nfile}_DCPS.h)
  SET(outsources ${outsources} ${idl_dir}/gen/${nfile}SplDcps.cpp ${idl_dir}/gen/${nfile}SplDcps.h)
  SET(outsources ${outsources} ${idl_dir}/gen/ccpp_${nfile}.h)
ENDMACRO(DEFINE_OpenSplice_SOURCES)

MACRO(OpenSplice_IDLGEN idlfilename)
  GET_FILENAME_COMPONENT(it ${idlfilename} ABSOLUTE)
  GET_FILENAME_COMPONENT(idlfilename ${idlfilename} NAME)
  DEFINE_OpenSplice_SOURCES(${ARGV})

  ADD_CUSTOM_COMMAND (
    PRE_BUILD
    OUTPUT ${outsources}
    COMMAND ${OpenSplice_IDLGEN_BINARY}
    ARGS -l isocpp2 -d ${idl_dir}/gen ${idlfilename}
    DEPENDS ${it}
    COMMENT "${outsources} have been produced in directory ${idl_dir}/gen"
  )
ENDMACRO(OpenSplice_IDLGEN)
