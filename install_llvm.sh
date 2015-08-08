#!/bin/bash

############################################################################
# Install a recent CMake
############################################################################
if [[ ! -d "cmake" ]]; then
    URL_CMAKE=http://www.cmake.org/files/v3.2/cmake-3.2.1-Linux-x86_64.tar.gz
    mkdir cmake
    wget --quiet -O - ${URL_CMAKE} | tar --strip-components=1 -xz -C cmake
    export PATH=${PWD}/cmake/bin:${PATH}
fi

############################################################################
# Install and compile the compiler runtime for clang
############################################################################
FLAGS_LLVM="-DCMAKE_C_COMPILER=${CC} -DCMAKE_CXX_COMPILER=${CXX} -DCMAKE_BUILD_TYPE:STRING=Release"

echo "Checking llvm"

if [[ ! -d "llvm" ]]; then
    echo "..cloning"

    #get llvm
    git clone -b release_36 --single-branch --depth 1 --recursive --quiet https://github.com/llvm-mirror/llvm.git
    #get compiler-rt
    (cd llvm/projects && git clone -b release_36 --single-branch --depth 1 --recursive --quiet https://github.com/llvm-mirror/compiler-rt.git)
    #build it
    (mkdir -p llvm/build && cd llvm/build && cmake -Wno-dev .. ${FLAGS_LLVM} && make compiler-rt -j2)

    #clean up
    find . -name "*.o" -type f -delete
    exit 0
fi

BUILD=0

cd llvm

LOCAL=$(git rev-parse @)
REMOTE=$(git rev-parse @{u})
BASE=$(git merge-base @ @{u})

if [[ $LOCAL != $REMOTE ]]; then
    echo "..updating llvm"
    git pull origin master
    BUILD=1
fi

cd projects/compiler-rt

LOCAL=$(git rev-parse @)
REMOTE=$(git rev-parse @{u})
BASH=$(git merge-base @ @{u})

if [[ $LOCAL != $REMOTE ]]; then
    echo "..updating llvm compiler-rt"
    git pull origin master
    BUILD=1
fi

cd ../../..

if [[ $BUILD != 0 ]]; then
    echo "..updating"
    (cd llvm/build && cmake .. ${FLAGS_LLVM} && make compiler-rt -j2)
else
    echo "..using cache"
fi
