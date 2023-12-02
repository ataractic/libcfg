#!/bin/bash

if [ -d "build" ]; then
  rm -rf "build"
fi

mkdir -p build
make shared
mv ./libcfg.so ./build/tmp
make clean
mv ./build/tmp ./build/libcfg.so