#!/bin/sh

BUILDTYPE="debug"
CMAKE_BUILD_TYPE="Debug"
CMAKE_GENERATOR="Ninja"

if [ "$#" -ne "2" ]; then
  echo "Usage cmake_linux.sh <gcc|clang|clang-libc++> <debug|release|reldeb>"
  exit 1
fi

if [ "$1" = "gcc" ]; then
  export CXX="g++"
  export CC="gcc"
  DIRECTORY="gcc"
elif [ "$1" = "clang" ]; then
  export CXX="clang++"
  export CC="clang"
  DIRECTORY="clang"
  CMAKE_CXX_FLAGS="-stdlib=libstdc++"
elif [ "$1" = "clang-libc++" ]; then
  export CXX="clang++"
  export CC="clang"
  DIRECTORY="clang-libc++"
  CMAKE_CXX_FLAGS="-stdlib=libc++"
else
  echo "Usage cmake_linux.sh <gcc|clang|clang-libc++> <debug|release|reldeb>"
  exit 1
fi

if [ "$2" = "debug" ]; then
  BUILDTYPE="debug"
  CMAKE_BUILD_TYPE="Debug"
elif [ "$2" = "release" ]; then
  BUILDTYPE="release"
  CMAKE_BUILD_TYPE="Release"
elif [ "$2" = "reldeb" ]; then
  BUILDTYPE="relwithdebinfo"
  CMAKE_BUILD_TYPE="RelWithDebInfo"
else
  echo "Usage cmake_linux.sh <gcc|clang|clang-libc++> <debug|release|reldeb>"
  exit 1
fi

BUILDDIR=build/$DIRECTORY/$BUILDTYPE

mkdir -p $BUILDDIR
cd $BUILDDIR

cmake -G"$CMAKE_GENERATOR" -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE -DCMAKE_CXX_FLAGS="$CMAKE_CXX_FLAGS" ../../../src
