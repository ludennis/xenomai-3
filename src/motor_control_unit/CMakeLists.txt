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
set(MATLAB_CODE_DIR "${PROJECT_SOURCE_DIR}/src/matlab_code")
set(MODEL_CODE_DIR "${PROJECT_SOURCE_DIR}/src/model_code")
set(CONTROLLER_CODE_DIR "${PROJECT_SOURCE_DIR}/src/controller_code")
set(XENOMAI_CODE_DIR "${PROJECT_SOURCE_DIR}/src/xenomai_code")
set(PICKERING_CODE_DIR "${PROJECT_SOURCE_DIR}/src/pickering_code")
set(SIMULINK_GENERATED_CODE_DIR "${PROJECT_SOURCE_DIR}/src/simulink_generated_code")
set(PEAK_CAN_CODE_DIR "${PROJECT_SOURCE_DIR}/src/peak_can_code")
set(NI_CODE_DIR "${PROJECT_SOURCE_DIR}/src/ni_code")

set(PICKERING_INCLUDE_DIR "/usr/local/lib")
set(PEAK_CAN_INCLUDE_DIR "/usr/local/include")
set(PEAK_CAN_LIB_DIR "/usr/local/lib")

# ni include directories and source files
set(NI_INCLUDE_DIRS
  ${NI_CODE_DIR}/nimhddk
  ${NI_CODE_DIR}/nixseries
  ${NI_CODE_DIR}/nixseries/CHInCh
  ${NI_CODE_DIR}/nixseries/ChipObjects
  ${NI_CODE_DIR}/nixseries/Examples
  ${NI_CODE_DIR}/nixseries/Examples/inTimer
  ${NI_CODE_DIR}/nixseries/Examples/outTimer
)

set(NI_SOURCES
  ${NI_CODE_DIR}/nimhddk/LinuxKernel/osiUserCode.cpp
  ${NI_CODE_DIR}/nimhddk/osiBus.cpp
  ${NI_CODE_DIR}/nixseries/ChipObjects/tXSeries.cpp
  ${NI_CODE_DIR}/nixseries/ChipObjects/tCHInCh.cpp
  ${NI_CODE_DIR}/nixseries/ChipObjects/tAI.cpp
  ${NI_CODE_DIR}/nixseries/ChipObjects/tAO.cpp
  ${NI_CODE_DIR}/nixseries/ChipObjects/tDI.cpp
  ${NI_CODE_DIR}/nixseries/ChipObjects/tDO.cpp
  ${NI_CODE_DIR}/nixseries/ChipObjects/tCounter.cpp
  ${NI_CODE_DIR}/nixseries/ChipObjects/tBusInterface.cpp
  ${NI_CODE_DIR}/nixseries/ChipObjects/tBrdServices.cpp
  ${NI_CODE_DIR}/nixseries/ChipObjects/tInTimer.cpp
  ${NI_CODE_DIR}/nixseries/ChipObjects/tOutTimer.cpp
  ${NI_CODE_DIR}/nixseries/ChipObjects/tDMAController.cpp
  ${NI_CODE_DIR}/nixseries/ChipObjects/tTriggers.cpp
  ${NI_CODE_DIR}/nixseries/ChipObjects/tStreamCircuitRegMap.cpp
  ${NI_CODE_DIR}/nixseries/ChipObjects/tSimultaneousControl.cpp
  ${NI_CODE_DIR}/nixseries/Examples/dataHelper.cpp
  ${NI_CODE_DIR}/nixseries/Examples/devices.cpp
  ${NI_CODE_DIR}/nixseries/Examples/dio/dioHelper.cpp
  ${NI_CODE_DIR}/nixseries/Examples/dio/pfiDioHelper.cpp
  ${NI_CODE_DIR}/nixseries/Examples/eepromHelper.cpp
  ${NI_CODE_DIR}/nixseries/Examples/simultaneousInit.cpp
  ${NI_CODE_DIR}/nixseries/Examples/streamHelper.cpp
  ${NI_CODE_DIR}/nixseries/Examples/inTimer/diHelper.cpp
  ${NI_CODE_DIR}/nixseries/Examples/inTimer/inTimerHelper.cpp
  ${NI_CODE_DIR}/nixseries/Examples/inTimer/inTimerParams.cpp
  ${NI_CODE_DIR}/nixseries/Examples/outTimer/doHelper.cpp
  ${NI_CODE_DIR}/nixseries/Examples/outTimer/outTimerHelper.cpp
  ${NI_CODE_DIR}/nixseries/CHInCh/tCHInChDMAChannelController.cpp
  ${NI_CODE_DIR}/nixseries/CHInCh/tCHInChDMAChannel.cpp
  ${NI_CODE_DIR}/nixseries/CHInCh/tCHInChSGLChunkyLink.cpp
  ${NI_CODE_DIR}/nixseries/CHInCh/tCHInChSGL.cpp
  ${NI_CODE_DIR}/nixseries/CHInCh/tDMABuffer.cpp
  ${NI_CODE_DIR}/nixseries/CHInCh/tLinearDMABuffer.cpp
  ${NI_CODE_DIR}/nixseries/CHInCh/tScatterGatherDMABuffer.cpp
)

# ni compile options
set(NI_COMPILE_OPTIONS
  -DkLittleEndian=1
  -DkGNU=1
  -Dk64BitKernel=1
  -DkBAR0Only=1
  -fpermissive
  -g
)

# motor_model_v2
add_executable(motor
  ${MODEL_CODE_DIR}/ert_main.cpp
  ${MODEL_CODE_DIR}/generated_model.cpp
  ${MODEL_CODE_DIR}/input_interface.cpp
  ${MODEL_CODE_DIR}/output_interface.cpp
  ${MODEL_CODE_DIR}/rtGetInf.cpp
  ${MODEL_CODE_DIR}/rtGetNaN.cpp
  ${MODEL_CODE_DIR}/rt_nonfinite.cpp
)

target_include_directories(motor
  PUBLIC
  ${MATLAB_CODE_DIR}/extern/include
  ${MATLAB_CODE_DIR}/rtw/c/ert
  ${MATLAB_CODE_DIR}/rtw/c/src
  ${MATLAB_CODE_DIR}/rtw/c/src/ext_mode/common
  ${MATLAB_CODE_DIR}/simulink/include
  ${MODEL_CODE_DIR}
  ${PROJECT_SOURCE_DIR}/src
  ${XENOMAI_INCLUDE_DIRS}
  ${XENOMAI_CODE_DIR}
)

target_link_libraries(motor
  ${XENOMAI_LIBRARIES}
)

target_compile_options(motor
  PUBLIC
  -Wall
  -fpermissive
)

# controller
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(controller
  ${CONTROLLER_CODE_DIR}/controller_main.cpp
)

target_link_libraries(controller
  ${XENOMAI_LIBRARIES}
  Threads::Threads
)

target_include_directories(controller
  SYSTEM PUBLIC
  ${PROJECT_SOURCE_DIR}/src
  ${XENOMAI_INCLUDE_DIRS}
  ${XENOMAI_CODE_DIR}
)

target_compile_options(controller
  PUBLIC
  -Wall
)

# pickering rt task
link_directories("/usr/local/lib")

add_executable(non_precision_resistor_controller
  ${PICKERING_CODE_DIR}/non_precision_resistor_controller_main.cpp
  ${PICKERING_CODE_DIR}/RtResistanceTask.cpp
  ${PICKERING_CODE_DIR}/RtResistanceTask.h
  ${PICKERING_CODE_DIR}/RtSharedArray.cpp
  ${PICKERING_CODE_DIR}/RtSharedArray.h
)

target_include_directories(non_precision_resistor_controller
  SYSTEM PUBLIC
  ${PICKERING_CODE_DIR}
  ${PICKERING_INCLUDE_DIRS}
  ${XENOMAI_INCLUDE_DIRS}
)

target_link_libraries(non_precision_resistor_controller
  pilpxi64
  ${XENOMAI_LIBRARIES}
)

# simulink generated code for resistance card
add_executable(resistance_testing
  ${PICKERING_CODE_DIR}/PxiCardTask.cpp
  ${PICKERING_CODE_DIR}/RtGenerateResistanceArrayTask.cpp
  ${PICKERING_CODE_DIR}/RtResistanceTask.cpp
  ${PICKERING_CODE_DIR}/RtSharedArray.cpp
  ${SIMULINK_GENERATED_CODE_DIR}/resistance_testing_grt_rtw/testing.cpp
  ${SIMULINK_GENERATED_CODE_DIR}/resistance_testing_grt_rtw/testing_data.cpp
  ${SIMULINK_GENERATED_CODE_DIR}/resistance_testing_grt_rtw/testing_main.cpp
  ${XENOMAI_CODE_DIR}/RtPeriodicTask.cpp
)

target_include_directories(resistance_testing
  PUBLIC
  ${MATLAB_CODE_DIR}/extern/include
  ${MATLAB_CODE_DIR}/rtw/c/src
  ${MATLAB_CODE_DIR}/rtw/c/src/ext_mode/common
  ${MATLAB_CODE_DIR}/simulink/include
  ${PICKERING_CODE_DIR}
  ${PICKERING_INCLUDE_DIRS}
  ${SIMULINK_GENERATED_CODE_DIR}
  ${SIMULINK_GENERATED_CODE_DIR}/resistance_testing_grt_rtw
  ${XENOMAI_CODE_DIR}
  ${XENOMAI_INCLUDE_DIRS}
)

target_link_libraries(resistance_testing
  pilpxi64
  ${XENOMAI_LIBRARIES}
)

# simulink generated code for switching card
add_executable(switching_testing
  ${PICKERING_CODE_DIR}/PxiCardTask.cpp
  ${PICKERING_CODE_DIR}/RtGenerateStateTask.cpp
  ${PICKERING_CODE_DIR}/RtSharedState.cpp
  ${PICKERING_CODE_DIR}/RtSwitchTask.cpp
  ${SIMULINK_GENERATED_CODE_DIR}/switching_testing_grt_rtw/testing.cpp
  ${SIMULINK_GENERATED_CODE_DIR}/switching_testing_grt_rtw/testing_data.cpp
  ${SIMULINK_GENERATED_CODE_DIR}/switching_testing_grt_rtw/testing_main.cpp
  ${XENOMAI_CODE_DIR}/RtPeriodicTask.cpp
)

target_include_directories(switching_testing
  PUBLIC
  ${MATLAB_CODE_DIR}/extern/include
  ${MATLAB_CODE_DIR}/rtw/c/src
  ${MATLAB_CODE_DIR}/rtw/c/src/ext_mode/common
  ${MATLAB_CODE_DIR}/simulink/include
  ${PICKERING_CODE_DIR}
  ${PICKERING_INCLUDE_DIRS}
  ${SIMULINK_GENERATED_CODE_DIR}
  ${SIMULINK_GENERATED_CODE_DIR}/switching_testing_grt_rtw
  ${XENOMAI_CODE_DIR}
  ${XENOMAI_INCLUDE_DIRS}
)

target_link_libraries(switching_testing
  pilpxi64
  ${XENOMAI_LIBRARIES}
)

# peak can
add_executable(peak_can_transmit
  ${PEAK_CAN_CODE_DIR}/peak_can_transmit.cpp
  ${PEAK_CAN_CODE_DIR}/PeakCanTask.cpp
  ${PEAK_CAN_CODE_DIR}/RtPeakCanTransmitTask.cpp
  ${XENOMAI_CODE_DIR}/RtPeriodicTask.cpp
)

target_include_directories(peak_can_transmit
  PUBLIC
  ${PEAK_CAN_CODE_DIR}
  ${PEAK_CAN_INCLUDE_DIR}
  ${PICKERING_CODE_DIR}
  ${XENOMAI_CODE_DIR}
  ${XENOMAI_INCLUDE_DIRS}
)

target_link_libraries(peak_can_transmit
  -lpcan
  -lpcanfd
  ${XENOMAI_LIBRARIES}
)

add_executable(peak_can_receive
  ${PEAK_CAN_CODE_DIR}/peak_can_receive.cpp
  ${PEAK_CAN_CODE_DIR}/PeakCanTask.cpp
  ${PEAK_CAN_CODE_DIR}/RtPeakCanReceiveTask.cpp
  ${XENOMAI_CODE_DIR}/RtPeriodicTask.cpp
)

target_include_directories(peak_can_receive
  PUBLIC
  ${PEAK_CAN_CODE_DIR}
  ${PEAK_CAN_INCLUDE_DIR}
  ${PICKERING_CODE_DIR}
  ${XENOMAI_INCLUDE_DIRS}
  ${XENOMAI_CODE_DIR}
)

target_link_libraries(peak_can_receive
  -lpcan
  -lpcanfd
  ${XENOMAI_LIBRARIES}
)

# pwm_input
add_executable(pwm_input
  ${NI_CODE_DIR}/pwm_input.cpp
  ${NI_CODE_DIR}/pwm_input_main.cpp
  ${NI_CODE_DIR}/nixseries/Examples/inTimer/aiHelper.cpp
  ${NI_CODE_DIR}/nixseries/Examples/counterResetHelper.cpp
  ${NI_CODE_DIR}/nimhddk/osiBus.cpp
  ${NI_SOURCES}
)

target_include_directories(pwm_input
  PUBLIC
  ${NI_INCLUDE_DIRS}
)

target_compile_options(pwm_input
  PUBLIC
  ${NI_COMPILE_OPTIONS}
)

set_target_properties(pwm_input
  PROPERTIES
  LINKER_LANGUAGE
  CXX
)

# pwm_output
add_executable(pwm_output
  ${NI_CODE_DIR}/pwm_output.cpp
  ${NI_CODE_DIR}/pwm_output_main.cpp
  ${NI_CODE_DIR}/nixseries/Examples/inTimer/aiHelper.cpp
  ${NI_CODE_DIR}/nixseries/Examples/counterResetHelper.cpp
  ${NI_CODE_DIR}/nixseries/Examples/pfiRtsiResetHelper.cpp
  ${NI_CODE_DIR}/nimhddk/osiBus.cpp
  ${NI_SOURCES}
)

target_include_directories(pwm_output
  PUBLIC
  ${NI_INCLUDE_DIRS}
)

target_compile_options(pwm_output
  PUBLIC
  ${NI_COMPILE_OPTIONS}
)

set_target_properties(pwm_output
  PROPERTIES
  LINKER_LANGUAGE
  CXX
)

# motor monitor
add_executable(motor_monitor
  ${XENOMAI_CODE_DIR}/motor_monitor.cpp
)

target_include_directories(motor_monitor
  PUBLIC
  ${XENOMAI_INCLUDE_DIRS}
  ${XENOMAI_CODE_DIR}
)

target_link_libraries(motor_monitor
  ${XENOMAI_LIBRARIES}
)

# dds pipe
add_executable(dds_pipe
  ${XENOMAI_CODE_DIR}/dds_pipe.cpp
)

target_include_directories(dds_pipe
  PUBLIC
  ${XENOMAI_INCLUDE_DIRS}
  ${XENOMAI_CODE_DIR}
)

target_link_libraries(dds_pipe
  ${XENOMAI_LIBRARIES}
)

# install
install(TARGETS
  motor
  controller
  DESTINATION bin/${PROJECT_NAME}
)
