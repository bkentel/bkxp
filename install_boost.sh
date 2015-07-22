#!/bin/bash

download() {
    echo "Checking $1/$2"

    if [[ ! -d "$2" ]]; then
        echo "..cloning"
        git clone --depth 1 --recursive --quiet https://github.com/$1/$2.git
        return 0
    fi

    cd "$2"
    
    local LOCAL=$(git rev-parse @)
    local REMOTE=$(git rev-parse @{u})
    local BASE=$(git merge-base @ @{u})
    
    if [[ $LOCAL != $REMOTE ]]; then
        echo "..updating"
        git pull origin master
    fi
    
    echo "..using cache"
    cd ..;
}

############################################################################
# Install boost, catch, pcg-random, and rapidjson dependencies
############################################################################
download boostorg core
download boostorg config
download boostorg preprocessor
download boostorg assert
download boostorg static_assert
download boostorg type_traits
download boostorg mpl
download boostorg integer
download boostorg throw_exception
download boostorg smart_ptr
download boostorg exception
download boostorg predef
download boostorg utility
download boostorg tuple
download boostorg random

download philsquared Catch
download imneme pcg-cpp
download miloyip rapidjson
