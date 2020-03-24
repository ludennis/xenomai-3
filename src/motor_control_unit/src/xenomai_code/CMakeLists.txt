cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake)

find_package(Xenomai 3 REQUIRED)

# test_rt
add_executable(test_rt
  test_rt.cpp
)

target_link_libraries(test_rt
  ${XENOMAI_LIBRARIES}
)

target_include_directories(test_rt
  SYSTEM PUBLIC
  ${XENOMAI_INCLUDE_DIRS}
)

# five-tasks
add_executable(five-tasks
  five-tasks.cpp
)

target_link_libraries(five-tasks
  ${XENOMAI_LIBRARIES}
)

target_include_directories(five-tasks
  SYSTEM PUBLIC
  ${XENOMAI_INCLUDE_DIRS}
)

# semaphore
add_executable(semaphore
  semaphore.cpp
)

target_link_libraries(semaphore
  ${XENOMAI_LIBRARIES}
)

target_include_directories(semaphore
  SYSTEM PUBLIC
  ${XENOMAI_INCLUDE_DIRS}
)
