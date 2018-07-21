#------------------------------------------
#!/bin/bash

set -e

#------------------------------------------
# This script configures and build the base components
# of KTAB in the correct order. It will generate
# "Unix Makefiles", unless the first argument is non-null.
#------------------------------------------

DIRS=" minwater  agenda  reformpri  smp  comsel  pmatrix econOptim"

DQ="\""
DEFGEN="Unix Makefiles"

if [ "$1" == "" ]
 then GEN=$DEFGEN
 else GEN="$1"
fi


# note that two double-quotes are part of the string
DQGEN=$DQ$GEN$DQ

echo "Generator:  $DQGEN"

for d in $DIRS
do
  if [ -a $d/build ]
    then
      echo "Deleting old directory $d/build"
      rm -rf $d/build
  fi
  echo "Configuring $d/build" ;
  pushd $d ; mkdir build ; cd build ;  cmake .. -G "Unix Makefiles" ; make clean; nice make -k -j3 all ; sleep 1; popd
done

#------------------------------------------
