# Pico UAC2 Loopback + Frequency meter (RP2040)

pico_uac2_loopback_freq/
├─ CMakeLists.txt
├─ pico_sdk_import.cmake
├─ src/
│  ├─ main.c
│  ├─ tusb_config.h
│  ├─ usb_descriptors.c
│  └─ usb_descriptors.h
└─ README.md

Raspberry Pi Pico como dispositivo de audio USB (UAC2) tipo "headset":
- Speaker (OUT host->Pico) a 48 kHz 16-bit estéreo
- Microphone (IN Pico->host) a 48 kHz 16-bit mono

Hace loopback (estéreo->mono) y calcula frecuencia + RMS sobre la señal entrante.

## Requisitos
- Pico SDK instalado
- TinyUSB habilitado (viene en Pico SDK)
- Exportar PICO_SDK_PATH

## Build
```bash
export PICO_SDK_PATH=~/pico/pico-sdk
mkdir build
cd build
cmake ..
make -j

