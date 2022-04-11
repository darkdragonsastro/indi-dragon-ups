#!/bin/bash
IFS='.' read -r -a version <<< "$1"

sed -i 's/set(VERSION_MAJOR [[:digit:]]\+)/set(VERSION_MAJOR '"${version[0]}"')/' ./CMakeLists.txt
sed -i 's/set(VERSION_MINOR [[:digit:]]\+)/set(VERSION_MINOR '"${version[1]}"')/' ./CMakeLists.txt
sed -i 's/set(VERSION_PATCH [[:digit:]]\+)/set(VERSION_PATCH '"${version[2]}"')/' ./CMakeLists.txt
