#------------------------------------------
#!/bin/bash
#------------------------------------------
# You must generate a 'build' directory for each subproject
# before using this script. CMake is the recommended was to do so.
#
#------------------------------------------

DIRS=" minwater  agenda  reformpri  smp  comsel  pmatrix econOptim"

for d in $DIRS
do
pushd $d/build ; nice make clean ; sleep 1; popd
done
#------------------------------------------
