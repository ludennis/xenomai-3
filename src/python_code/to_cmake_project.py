#!/usr/bin/env python3

import os
import re
import shutil
import sys

def getSize(path):
    total_size = 0
    for dirpath, dirnames, filenames in os.walk(path):
        for filename in filenames:
            file_path = os.path.join(dirpath, filename)
            if not os.path.islink(file_path):
                total_size += os.path.getsize(file_path)

    return total_size


def removeBadCharactersFromPath(path):
    path = path.lstrip(' ')
    path = removeEndlineFromPath(path)
    path = removeBackSlashFromPath(path)
    return path

def removeBackSlashFromPath(path):
    return path.replace('\\', '')
    path = removeWhitespacesFromPath(path)

def removeLetterFromPath(path):
    return '/'.join(re.split('/', path)[1:])

def removeEndlineFromPath(path):
    return path.replace('\n', '')

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: {} [path to CMakeLists file]".format(__file__))

    found_include_directories = False

    # open directory for copying includes from CMakeLists.txt
    current_path = os.getcwd()
    cmake_include_files_path = current_path + '/cmake_include_files/'
    if not os.path.isdir(cmake_include_files_path):
        os.mkdir(cmake_include_files_path)

    with open(sys.argv[1], 'r') as cmake_file:
        for line in cmake_file:
            file_path = removeBadCharactersFromPath(line)
            if ")" in file_path:
                found_include_directories = False
            elif found_include_directories:
                print("copying {} Kbytes from {}".format(getSize(file_path) // 1000, file_path))
                src_file_dir = file_path
                dst_file_dir = cmake_include_files_path + removeLetterFromPath(file_path)
                if os.path.isdir(dst_file_dir):
                    shutil.rmtree(dst_file_dir)
                shutil.copytree(src_file_dir, dst_file_dir)
            elif "include_directories" in file_path:
                # get the include directories
                # stop with a )
                found_include_directories = True
