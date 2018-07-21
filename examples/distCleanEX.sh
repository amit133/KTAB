#!/bin/bash
#
# This wipes out all the "build" directories, so you
# will have to run CMake individually on each to
# rebuild them
#
#------------------------------------------

DIRS=" minwater  agenda  reformpri  smp  comsel  pmatrix econOptim"

for d in $FILES
do
pushd $d/build; make clean; popd
pushd $d; rm -rf build ; sleep 1; popd
done
#------------------------------------------
