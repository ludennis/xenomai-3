cmake_minimum_required(VERSION 3.5)
project(motor_model_v2_node)

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

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(data_interfaces REQUIRED)
# uncomment the following section in order to fill in
# further dependencies manually.
# find_package(<dependency> REQUIRED)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # uncomment the line when a copyright and license is not present in all source files
  #set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # uncomment the line when this package is not in a git repo
  #set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

# Set path
set(MATLAB_ROOT "src/matlab_code")
set(START_DIR "src/model_code")

# Set include path
include_directories(${START_DIR})
include_directories(${MATLAB_ROOT}/extern/include)
include_directories(${MATLAB_ROOT}/simulink/include)
include_directories(${MATLAB_ROOT}/rtw/c/src)
include_directories(${MATLAB_ROOT}/rtw/c/src/ext_mode/common)
include_directories(${MATLAB_ROOT}/rtw/c/ert)

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

# Set executable
add_executable(motor_model_v2
${START_DIR}/generated_model.cpp
${START_DIR}/rtGetInf.cpp
${START_DIR}/rtGetNaN.cpp
${START_DIR}/rt_nonfinite.cpp
${START_DIR}/ert_main.cpp
${START_DIR}/input_interface.cpp
${START_DIR}/output_interface.cpp)
ament_target_dependencies(motor_model_v2 rclcpp std_msgs data_interfaces)

install(TARGETS
# Set install
motor_model_v2
  DESTINATION lib/${PROJECT_NAME})

ament_package()
