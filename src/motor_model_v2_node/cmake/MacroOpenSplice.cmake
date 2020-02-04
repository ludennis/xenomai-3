MACRO(DEFINE_OpenSplice_SOURCES idlfilename)
  SET(outsources)
  GET_FILENAME_COMPONENT(it ${idlfilename} ABSOLUTE)
  GET_FILENAME_COMPONENT(idl_dir ${idlfilename} DIRECTORY)
  GET_FILENAME_COMPONENT(nfile ${idlfilename} NAME_WE)
  SET(outsources ${outsources} ${idl_dir}/gen/${nfile}.cpp ${idl_dir}/gen/${nfile}.h)
  SET(outsources ${outsources} ${idl_dir}/gen/${nfile}Dcps.cpp ${idl_dir}/gen/${nfile}Dcps.h)
  SET(outsources ${outsources} ${idl_dir}/gen/${nfile}Dcps_impl.cpp ${idl_dir}/gen/${nfile}Dcps_impl.h)
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
    ARGS -S -l isocpp2 -d ${PROJECT_SOURCE_DIR}/src/idl/gen ${idlfilename}
    DEPENDS ${it}
    COMMENT "${outsources} have been produced in directory gen"
  )
ENDMACRO(OpenSplice_IDLGEN)