cmake_minimum_required(VERSION 2.7)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_TYPE Release)


include_directories(
    simulink/dds_reader_writer_model
    simulink/dds_reader_writer_model/testing_grt_rtw
    extern/include
    simulink/include
    rtw/c/src
    rtw/c/src/ext_mode/common
)

add_executable(testing ${SOURCES})
option(USE_PTHREAD_LIB "Libraries to link application against" OFF)
option(USE_RT_PTHREAD_LIB "Libraries to link application against" ON)
if(USE_PTHREAD_LIB)
target_link_libraries( testing ${CMAKE_THREAD_LIBS_INIT}
pthread
)
elseif(USE_RT_PTHREAD_LIB)
target_link_libraries(testing ${CMAKE_THREAD_LIBS_INIT}
rt
pthread
)
else()
target_link_libraries(testing ${CMAKE_THREAD_LIBS_INIT}
)
endif()
unset(USE_PTHREAD_LIB CACHE)
unset(USE_RT_PTHREAD_LIB CACHE)
