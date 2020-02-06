cmake_minimum_required(VERSION 3.5)
project(motor_model_v2_node)

# OS environment to find OpenSplice package
if(WIN32)
  # suppress C4251 warnings
  add_compile_options(/wd4251)
  # settings for dll
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS on)
  if(DEBUG)
    set(DEFINITIONS "-DNDEBUG")
  endif(DEBUG)
else(WIN32)
  if(DEBUG)
    set(DEFINITIONS "-ansi -g -pg -pipe -std=c++98")
  else(DEBUG)
    if(PEDANTIC)
      set(DEFINITIONS "-ansi -g -pg -pipe -Wall -Wstrict-null-sentinel -Weffc++ -Wold-style-cast -pedantic")
      set(DEBUG 1)
    else(PEDANTIC)
      set(DEFINITIONS "-DNDEBUG -O0 -pipe")
    endif(PEDANTIC)
  endif(DEBUG)
endif(WIN32)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(osmacros)
include(MacroOpenSplice)

set(CMAKE_VERBOSE_MAKEFILE = 1)
find_package(OpenSplice REQUIRED)

#  settings for opensplice dds
include_directories(
  ${PROJECT_SOURCE_DIR}
  ${OpenSplice_INCLUDE_DIRS}
)

add_definitions(
  ${OpenSplice_DEFINITIONS}
  ${DEFINITIONS}
)

# idl settings
set(idls
  ${PROJECT_SOURCE_DIR}/src/idl/MotorControllerUnitModule.idl
)

set(idl_targets)
set(OpenSplice_DATAMODEL)
foreach(idl ${idls})
  # generate idl source files with idlpp.exe
  OpenSplice_IDLGEN(${idl})
  GET_FILENAME_COMPONENT(idl_filename ${idl} NAME_WE)

  # OpenSplice_DATAMODEL should only contain .cpp .h and _DCPS.h files
  foreach(outsource ${outsources})
    GET_FILENAME_COMPONENT(outsource_filename ${outsource} NAME)
    if("${outsource_filename}" STREQUAL "${idl_filename}.cpp" OR
       "${outsource_filename}" STREQUAL "${idl_filename}.h" OR
       "${outsource_filename}" STREQUAL "${idl_filename}_DCPS.hpp" OR
       "${outsource_filename}" STREQUAL "${idl_filename}SplDcps.cpp" OR
       "${outsource_filename}" STREQUAL "${idl_filename}SplDcps.h")
      set(OpenSplice_DATAMODEL ${OpenSplice_DATAMODEL} ${outsource})
    endif()
  endforeach(outsource)

  add_custom_target("${idl_filename}_generated_sources"
    PRE_BUILD
    ALL
    DEPENDS ${outsources}
    COMMENT "custom_target has been built"
  )
endforeach(idl)

set(DATAMODEL datamodel)
add_library(${DATAMODEL} SHARED ${OpenSplice_DATAMODEL})

target_link_libraries(${DATAMODEL}
  ${OpenSplice_LIBRARIES}
)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()
add_compile_options(-O3)

# Set path
set(MATLAB_DIR "src/matlab_code")
set(MODEL_DIR "src/model_code")
set(CONTROLLER_DIR "src/controller_code")

# Set include path for project
include_directories(${MODEL_DIR})
include_directories(${MATLAB_DIR}/extern/include)
include_directories(${MATLAB_DIR}/simulink/include)
include_directories(${MATLAB_DIR}/rtw/c/src)
include_directories(${MATLAB_DIR}/rtw/c/src/ext_mode/common)
include_directories(${MATLAB_DIR}/rtw/c/ert)

# Set define
add_definitions(-DCLASSIC_INTERFACE=0)
add_definitions(-DALLOCATIONFCN=0)
add_definitions(-DTERMFCN=1)
add_definitions(-DONESTEPFCN=1)
add_definitions(-DMAT_FILE=0)
add_definitions(-DMULTI_INSTANCE_CODE=0)
add_definitions(-DINTEGER_CODE=0)
add_definitions(-DMT=0)
add_definitions(-DTID01EQ=1)
add_definitions(-DMODEL=generated_model)
add_definitions(-DNUMST=2)
add_definitions(-DNCSTATES=5)
add_definitions(-DHAVESTDIO)
add_definitions(-DMODEL_HAS_DYNAMICALLY_LOADED_SFCNS=0)

# motor_model_v2
add_executable(motor_model_v2
  ${MODEL_DIR}/generated_model.cpp
  ${MODEL_DIR}/rtGetInf.cpp
  ${MODEL_DIR}/rtGetNaN.cpp
  ${MODEL_DIR}/rt_nonfinite.cpp
  ${MODEL_DIR}/ert_main.cpp
  ${MODEL_DIR}/input_interface.cpp
  ${MODEL_DIR}/output_interface.cpp
)

target_include_directories(motor_model_v2
  PUBLIC
  ${OpenSplice_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(motor_model_v2
  ${OpenSplice_LIBRARIES}
  ${DATAMODEL}
)

# controller
add_executable(controller
  ${CONTROLLER_DIR}/controller_main.cpp
)

target_link_libraries(controller
  ${OpenSplice_LIBRARIES}
  ${DATAMODEL}
)

target_include_directories(controller
  PUBLIC
  ${OpenSplice_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}/src
)

# install
install(TARGETS
  motor_model_v2
  controller
  DESTINATION lib/${PROJECT_NAME}
)