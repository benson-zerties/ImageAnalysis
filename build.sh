#!/bin/sh

BUILD_DIR="build"
cmake -S . -B "$BUILD_DIR"
cmake --build "$BUILD_DIR"
