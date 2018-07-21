#------------------------------------------
#!/bin/bash
#------------------------------------------
# You must generate a 'build' directory for each subproject
# and configure each one so it is buildable
# before using this script. CMake is the recommended was to do so.
# This script just builds them all in order, as a shortcut
# after you have set it up.
#------------------------------------------

DIRS=" minwater  agenda  reformpri  smp  comsel  pmatrix econOptim"

for d in $DIRS
do
  pushd $d/build ; nice make -k all ; sleep 1; popd
done

#------------------------------------------
