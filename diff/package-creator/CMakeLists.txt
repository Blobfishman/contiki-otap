cmake_minimum_required(VERSION 3.10)

# set the project name
project(creator)

# add the executable
add_executable(Creator ./../minidiff/bsdiff.c ./../FastLZ/fastlz.c creator.c)
add_executable(Creator-fastlz ./../minidiff/bsdiff.c ./../FastLZ/fastlz.c creator-fastlz.c)
add_executable(Creator-xor ./../minidiff/bsdiff.c ./../FastLZ/fastlz.c creator-xor.c)