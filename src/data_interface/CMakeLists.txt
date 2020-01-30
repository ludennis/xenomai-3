cmake_minimum_required(VERSION 3.5)
project(data_interfaces)

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

# find dependencies
find_package(ament_cmake REQUIRED)
# find the ROS message code generators
find_package(rosidl_default_generators REQUIRED)

# declare the message files to generate code for
set(msg_files
# msg files
msg/MsgMcuOutput.msg
msg/MsgDynoCmd.msg
msg/MsgMotorOutput.msg
msg/MsgDynoSensing.msg
)

# declare the service files to generate code for
set(srv_files
# srv files
) 

rosidl_generate_interfaces(${PROJECT_NAME}
  ${msg_files}
  ${srv_files}
)

ament_export_dependencies(rosidl_default_runtime)

ament_package()
