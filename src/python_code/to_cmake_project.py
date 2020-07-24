#!/usr/bin/env python3

import os
import sys

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: {} [path to CMakeLists file]".format(__file__))

    found_include_directories = False

    with open(sys.argv[1], 'r') as cmake_file:
        for line in cmake_file:
            if ")" in line:
                found_include_directories = False
            elif found_include_directories:
                print(line)
            elif "include_directories" in line:
                # get the include directories
                # stop with a )
                found_include_directories = True
