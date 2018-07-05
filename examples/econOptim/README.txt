## Build

mkdir build
cd build

# Note: Value of -G flag depends on the version of visual studio. The appropriate value can be checked by running "cmake --help" command
cmake .. -G "Visual Studio 15 2017 Win64"

cmake --build .

## Run
## A directory Debug at the same level of the build directory gets created and the binaries get copied there.
## copy the sampleinput.xml file in the Debug directory before running the program

econOptim.exe > logs.txt

