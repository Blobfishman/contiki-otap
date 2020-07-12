cmake_minimum_required(VERSION 3.10)

# set the project name
project(creator)

# add the executable
add_executable(Creator ./../minidiff/bsdiff.c ./../FastLZ/fastlz.c creator.c)