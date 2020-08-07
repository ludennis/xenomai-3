#!/bin/bash

if [ $# -eq 0 -o $1 == "-h" ]; then
    echo "Usage: update_motor_model.sh [path_to_cmake_proj]"
    exit -1
fi

path_to_cmake_proj=$1

# compile & make the cmake proj to build model library
if [ -f $path_to_cmake_proj ]; then
    echo "Error: $path_to_cmake_proj is not a directory"
    exit -1
fi

if [ -f $path_to_cmake_proj/CMakeLists.txt ]; then
    echo "Found CMakeLists.txt in directory $path_to_cmake_proj"
else
    echo "Error: $path_to_cmake_proj doesn't contain CMakeLists.txt"
    exit -1
fi

if [ ! -d $path_to_cmake_proj/build ]; then
    mkdir $path_to_cmake_proj/build
fi

cd $path_to_cmake_proj/build
cmake ..
make

# check if rt-hil-simulation has been installed
rt_hil_simulation_installed_path=/usr/local/rt-hil-simulation
if [ ! -d $rt_hil_simulation_installed_path/ ]; then
    echo "Error: rt-hil-simulation cannot be found in $rt_hil_simulation_installed_path"
    exit -1
else
    echo "Found rt-hil-simulation installed in $rt_hil_simulation_installed_path"
fi

# updates motor model dynamic library, libmotor_model_lib.so
libraries=$rt_hil_simulation_installed_path/lib/*
motor_model_lib_name=libmotor_model_lib.so

for library in $libraries
do
    if [ $(basename $library) == $motor_model_lib_name ]; then
        echo "Found motor model library: $library"
        if [ -f $library.old ]; then
            sudo rm -f $library.old
        fi

        sudo mv -f --verbose $library $library.old

        if [ -f $path_to_cmake_proj/build/$motor_model_lib_name ]; then
            echo "Ensured that $path_to_cmake_proj/build/$motor_model_lib_name exists"
        fi

        sudo cp -f --verbose $path_to_cmake_proj/build/$motor_model_lib_name \
                $rt_hil_simulation_installed_path/lib/$motor_model_lib_name
    fi
done
