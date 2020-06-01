cmake_minimum_required(VERSION 3.5)
project(dds-motor-controller)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(osmacros)
include(MacroOpenSplice)

find_package(Xenomai 3 REQUIRED)

# OS environment to find OpenSplice package
if(WIN32)
  # adds WholeProgramOptimization
#  add_compile_options(/GL)
  # adds ClCompile Optimization MaxSpeed
  add_compile_options(/O2)
  # adds IntrinsicFunctions
  add_compile_options(/Oi)
  # adds MultiThreadedDLL for RuntimeLibrary
  add_compile_options(/MD)
  # adds RuntimeTypeInfo
  add_compile_options(/GR)
  # adds warning level3
  add_compile_options(/W3)
  # suppress C4251 and C4355 warnings
  add_compile_options(/wd4251 /wd4355)
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

set(CMAKE_VERBOSE_MAKEFILE = 1)
find_package(OpenSplice REQUIRED)

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

add_library(${DATAMODEL} STATIC ${OpenSplice_DATAMODEL})

target_link_libraries(${DATAMODEL}
  ${OpenSplice_LIBRARIES}
)

target_include_directories(${DATAMODEL}
  SYSTEM PUBLIC
  ${PROJECT_SOURCE_DIR}
  ${OpenSplice_INCLUDE_DIRS}
)

# adds linker options GenerateDebugInformation, OptimizeReferences, and EnableCOMDATFolding
if(MSVC)
  set_property(TARGET ${DATAMODEL}
    APPEND PROPERTY
    LINK_FLAGS
    /DEBUG /OPT /OPT:ICF
  )
endif(MSVC)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

#if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
#  add_compile_options(-Wall -Wextra -Wpedantic)
#endif()
add_compile_options(-O3)

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

# Set path
set(MATLAB_DIR "${PROJECT_SOURCE_DIR}/src/matlab_code")
set(MODEL_DIR "${PROJECT_SOURCE_DIR}/src/model_code")
set(CONTROLLER_DIR "${PROJECT_SOURCE_DIR}/src/controller_code")
set(XENOMAI_DIR "${PROJECT_SOURCE_DIR}/src/xenomai_code")
set(PICKERING_DIR "${PROJECT_SOURCE_DIR}/src/pickering_code")
set(SIMULINK_GENERATED_DIR "${PROJECT_SOURCE_DIR}/src/simulink_generated_code")
set(PEAK_CAN_DIR "${PROJECT_SOURCE_DIR}/src/peak_can_code")

set(PICKERING_INCLUDE_DIR "/usr/local/lib")
set(PEAK_CAN_INCLUDE_DIR "/usr/local/include")
set(PEAK_CAN_LIB_DIR "/usr/local/lib")

# motor_model_v2
add_executable(motor
  ${MODEL_DIR}/generated_model.cpp
  ${MODEL_DIR}/rtGetInf.cpp
  ${MODEL_DIR}/rtGetNaN.cpp
  ${MODEL_DIR}/rt_nonfinite.cpp
  ${MODEL_DIR}/ert_main.cpp
  ${MODEL_DIR}/input_interface.cpp
  ${MODEL_DIR}/output_interface.cpp
)

target_include_directories(motor
  PUBLIC
  ${MODEL_DIR}
  ${MATLAB_DIR}/extern/include
  ${MATLAB_DIR}/simulink/include
  ${MATLAB_DIR}/rtw/c/src
  ${MATLAB_DIR}/rtw/c/src/ext_mode/common
  ${MATLAB_DIR}/rtw/c/ert
  ${PROJECT_SOURCE_DIR}/src
  SYSTEM PUBLIC
  ${OpenSplice_INCLUDE_DIRS}
)

target_link_libraries(motor
  ${OpenSplice_LIBRARIES}
  ${DATAMODEL}
)

target_compile_options(motor
  PUBLIC
  -Wall
  -Wextra
  -Wpedantic
  -Werror
)

# controller
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(controller
  ${CONTROLLER_DIR}/controller_main.cpp
)

target_link_libraries(controller
  ${OpenSplice_LIBRARIES}
  ${DATAMODEL}
  Threads::Threads
  ${XENOMAI_LIBRARIES}
)

target_include_directories(controller
  SYSTEM PUBLIC
  ${PROJECT_SOURCE_DIR}/src
  ${OpenSplice_INCLUDE_DIRS}
  ${XENOMAI_INCLUDE_DIRS}
)

target_compile_options(controller
  PUBLIC
  -Wall
  -Wextra
  -Wpedantic
  -Werror
)

# pickering rt task
link_directories("/usr/local/lib")

add_executable(non_precision_resistor_controller
  ${PICKERING_DIR}/non_precision_resistor_controller_main.cpp
  ${PICKERING_DIR}/RtResistanceTask.cpp
  ${PICKERING_DIR}/RtResistanceTask.h
  ${PICKERING_DIR}/RtSharedArray.cpp
  ${PICKERING_DIR}/RtSharedArray.h
)

target_include_directories(non_precision_resistor_controller
  SYSTEM PUBLIC
  ${XENOMAI_INCLUDE_DIRS}
  ${PICKERING_INCLUDE_DIRS}
  ${PICKERING_DIR}
)

target_link_libraries(non_precision_resistor_controller
  pilpxi64
  ${XENOMAI_LIBRARIES}
)

# simulink generated code for resistance card
add_executable(resistance_testing
  ${SIMULINK_GENERATED_DIR}/resistance_testing_grt_rtw/testing_main.cpp
  ${SIMULINK_GENERATED_DIR}/resistance_testing_grt_rtw/testing.cpp
  ${SIMULINK_GENERATED_DIR}/resistance_testing_grt_rtw/testing_data.cpp
  ${PICKERING_DIR}/RtResistanceTask.cpp
  ${PICKERING_DIR}/RtSharedArray.cpp
  ${PICKERING_DIR}/PxiCardTask.cpp
  ${PICKERING_DIR}/RtPeriodicTask.cpp
  ${PICKERING_DIR}/RtGenerateResistanceArrayTask.cpp
#  ${MATLAB_DIR}/rtw/c/src/common/rt_cppclass_main.cpp
)

target_include_directories(resistance_testing
  PUBLIC
  ${SIMULINK_GENERATED_DIR}
  ${SIMULINK_GENERATED_DIR}/resistance_testing_grt_rtw
  ${MATLAB_DIR}/extern/include
  ${MATLAB_DIR}/simulink/include
  ${MATLAB_DIR}/rtw/c/src
  ${MATLAB_DIR}/rtw/c/src/ext_mode/common
  ${PICKERING_DIR}
  ${PICKERING_INCLUDE_DIRS}
  ${XENOMAI_INCLUDE_DIRS}
#  ${MATLAB_DIR}/rtw/c/ert
#  ${PROJECT_SOURCE_DIR}/src
#  SYSTEM PUBLIC
#  ${OpenSplice_INCLUDE_DIRS}
)

target_link_libraries(resistance_testing
  pilpxi64
  ${XENOMAI_LIBRARIES}
)

# simulink generated code for switching card
add_executable(switching_testing
  ${SIMULINK_GENERATED_DIR}/switching_testing_grt_rtw/testing_main.cpp
  ${SIMULINK_GENERATED_DIR}/switching_testing_grt_rtw/testing.cpp
  ${SIMULINK_GENERATED_DIR}/switching_testing_grt_rtw/testing_data.cpp
  ${PICKERING_DIR}/RtSharedState.cpp
  ${PICKERING_DIR}/RtSwitchTask.cpp
  ${PICKERING_DIR}/RtPeriodicTask.cpp
  ${PICKERING_DIR}/PxiCardTask.cpp
  ${PICKERING_DIR}/RtGenerateStateTask.cpp
)

target_include_directories(switching_testing
  PUBLIC
  ${SIMULINK_GENERATED_DIR}
  ${SIMULINK_GENERATED_DIR}/switching_testing_grt_rtw
  ${MATLAB_DIR}/extern/include
  ${MATLAB_DIR}/simulink/include
  ${MATLAB_DIR}/rtw/c/src
  ${MATLAB_DIR}/rtw/c/src/ext_mode/common
  ${PICKERING_DIR}
  ${PICKERING_INCLUDE_DIRS}
  ${XENOMAI_INCLUDE_DIRS}
)

target_link_libraries(switching_testing
  pilpxi64
  ${XENOMAI_LIBRARIES}
)

# peak can
add_executable(peak_can_transmit
  ${PEAK_CAN_DIR}/peak_can_transmit.cpp
  ${PEAK_CAN_DIR}/PeakCanTransmitTask.cpp
  ${PEAK_CAN_DIR}/PeakCanTask.cpp
  ${PEAK_CAN_DIR}/RtPeakCanTransmitTask.cpp
  ${PICKERING_DIR}/RtPeriodicTask.cpp
)

target_include_directories(peak_can_transmit
  PUBLIC
  ${PEAK_CAN_DIR}
  ${PEAK_CAN_INCLUDE_DIR}
  ${PICKERING_DIR}
  ${XENOMAI_INCLUDE_DIRS}
)

target_link_libraries(peak_can_transmit
  -lpcan
  -lpcanfd
  ${XENOMAI_LIBRARIES}
)

add_executable(peak_can_receive
  ${PEAK_CAN_DIR}/peak_can_receive.cpp
  ${PEAK_CAN_DIR}/PeakCanReceiveTask.cpp
  ${PEAK_CAN_DIR}/PeakCanTask.cpp
)

target_include_directories(peak_can_receive
  PUBLIC
  ${PEAK_CAN_DIR}
  ${PEAK_CAN_INCLUDE_DIR}
)

target_link_libraries(peak_can_receive
  -lpcan
  -lpcanfd
)

# install
install(TARGETS
  motor
  controller
  DESTINATION bin/${PROJECT_NAME}
)
