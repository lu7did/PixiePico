#*------------------------------------------------------------------------------------------------
#* ADX-ddsPIO (build chain)
#* (c) Dr. Pedro E. Colla (LU7DZ) <pedro.colla@gmail.com>
#* 
#* new generation rp2040 ADX based digital transceiver 
#* 
#* This is mainly an integration effort with some new code developed for this project
#*  
#* This is a test bench to develop an rp2040 Si4732 support library based on the C/C++ SDK
#*
#* The integration effort is being built on top of previous work from many parties,
#* including myself as follows:

VERSION="1.2"
LIBPATH="/Users/PCOLLA/Documents/GitHub/ADX-ddsPIO/src/ADX-ddsPIO_V"$VERSION
export PICO_SDK_PATH=/Users/PCOLLA/Documents/GitHub/pico/pico-sdk


clear
cd $LIBPATH
unset CMAKE_ARGS

cmake -S . -B build -DFAMILY=rp2040 -DPICO_SDK_PATH=/Users/PCOLLA/Documents/GitHub/pico/pico-sdk

cmake --build build -j
