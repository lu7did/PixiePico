clear
cd /Users/PCOLLA/Documents/GitHub/ADX-ddsPIO/src/QP-7C/qp7c_rp2040_cat
unset CMAKE_ARGS
cmake -S . -B build -DFAMILY=rp2040 -DPICO_SDK_PATH=/Users/PCOLLA/Documents/GitHub/pico/pico-sdk
make -C build -j

