#!/bin/bash

COMPILER=clang++
#COMPILER=g++

make CXX=${COMPILER} CXXFLAGS="-I/home/daniele/libs/boost_1_63_0/install/x86/include/ -isystem /home/daniele/libs/beast/Beast/include/" LDFLAGS="-L/home/daniele/libs/boost_1_63_0/install/x86/lib/"


