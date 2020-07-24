#!/usr/bin/env python3

import os
import re
import shutil
import sys

MAX_DIRECTORY_SIZE = 20e6 # 20 Mbytes

current_path = os.getcwd()
target_path = current_path + '/cmake_project/'
if not os.path.isdir(target_path):
    os.mkdir(target_path)

def GetSize(path):
    total_size = 0
    if os.path.isfile(path):
        return os.path.getsize(path)
    else:
        for dirpath, dirnames, filenames in os.walk(path):
            for filename in filenames:
                file_path = os.path.join(dirpath, filename)
                if not os.path.islink(file_path):
                    total_size += os.path.getsize(file_path)

    return total_size


def removeBadCharactersFromPath(path):
    path = path.lstrip(' ')
    path = path.replace('\n', '')
    path = path.replace('\\', '')
    return path


def removeLetterFromPath(path):
    return '/'.join(re.split('/', path)[1:])


def FindCmakeParameters(cmake_file_path):
    cmake_parameters = dict()
    with open(cmake_file_path, 'r') as cmake_file:
        for line in cmake_file:
            if "cmake_minimum_required" in line:
                cmake_parameters["cmake_minimum_required"] = \
                    re.split('\(|\)', line)[1]
            if "include_directories" in line:
                include_directories = list()
                line = next(cmake_file)
                while ")" not in line:
                    include_directories.append(removeBadCharactersFromPath(line))
                    line = next(cmake_file)
                cmake_parameters["include_directories"] = include_directories
            if "set(SOURCES" in line:
                sources = list()
                line = next(cmake_file)
                while ")" not in line:
                    sources.append(removeBadCharactersFromPath(line))
                    line = next(cmake_file)
                cmake_parameters["sources"] = sources

    return cmake_parameters


def CopyIncludeDirectories(directories):
    for directory in directories:
        size = GetSize(directory)
        if size > MAX_DIRECTORY_SIZE:
            print("Warning: copying large directory")
        print("Copying directory with size {} kbytes from {}" \
            .format(size // 1000, directory))
        src_dir = directory
        dst_dir = target_path + removeLetterFromPath(directory)
        if os.path.isdir(dst_dir):
            shutil.rmtree(dst_dir)
        shutil.copytree(src_dir, dst_dir)


def CopySources(sources):
    for source in sources:
        size = GetSize(source)
        if size > MAX_DIRECTORY_SIZE:
            print("Warning: copying large file")
        print("Copying file with size {} kbytes from {}" \
            .format(size // 1000, source))
        src_name = source
        dst_name = target_path + removeLetterFromPath(source)
        shutil.copy2(src_name, dst_name)

def WriteCmakeFile(parameters):
    with open(target_path + 'CMakeLists.txt', 'w') as cmake_file:
        cmake_file.write("cmake_minimum_required({})\n" \
            .format(parameters["cmake_minimum_required"]))

        cmake_file.write("project(simulink_generated_code)\n")

        cmake_file.write("add_executable(main\n")
        for source in parameters["sources"]:
            cmake_file.write('  ' + removeLetterFromPath(source) + '\n')
        cmake_file.write(")\n")

        cmake_file.write("target_include_directories(main\n")
        cmake_file.write("  PUBLIC\n")
        for directory in parameters["include_directories"]:
            cmake_file.write('  ' + removeLetterFromPath(directory) + '\n')
        cmake_file.write(")\n")


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: {} [path to CMakeLists file]".format(__file__))

    cmake_parameters = FindCmakeParameters(sys.argv[1])

    CopyIncludeDirectories(cmake_parameters["include_directories"])
    CopySources(cmake_parameters["sources"])
    WriteCmakeFile(cmake_parameters)
