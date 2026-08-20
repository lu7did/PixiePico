#*------------------------------------------------------------------------------------------------
#* ADX-ddsPIO (build chain)
#* (c) Dr. Pedro E. Colla (LU7DZ) <pedro.colla@gmail.com>
#* 
#* new generation rp2040 ADX based digital transceiver 
#* 
#* This is mainly an integration effort with some new code developed for this project
#*  
#* The integration effort is being built on top of previous work from many parties,
#* including myself as follows:

VERSION="1.2"
LIBPATH="/Users/PCOLLA/Documents/GitHub/ADX-ddsPIO/src/ADX-ddsPIO_V"$VERSION
export PICO_SDK_PATH=/Users/PCOLLA/Documents/GitHub/pico/pico-sdk


clear
cd $LIBPATH
unset CMAKE_ARGS

#*  CMake para Raspberry Pi Pico or rp2040Z (convencional)
cmake -S . -B build -DFAMILY=rp2040 -DPICO_SDK_PATH=/Users/PCOLLA/Documents/GitHub/pico/pico-sdk

#* CMake para RaspBerry Pi Pico W (Wireless)
#*  cmake -S . -B build -DPICO_BOARD=pico_w -DFAMILY=rp2040 -DPICO_SDK_PATH=/Users/PCOLLA/Documents/GitHub/pico/pico-sdk

make -C build -j

