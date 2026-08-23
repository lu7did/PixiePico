# testOLED

Proyecto autónomo para Waveshare RP2040-Zero que conserva el blink del WS2812
y la consola USB CDC, y muestra un texto fijo en un OLED SSD1306 I2C de 128x32.

## Instalación

Descomprimir el ZIP directamente en:

```text
C:\Users\pedro\github\PixiePico\src
```

Debe quedar:

```text
C:\Users\pedro\github\PixiePico\src\testOLED
```

Abrir en VS Code exactamente la carpeta `testOLED`, no `src` ni `PixiePico`.
El proyecto no depende de `src\blink` ni de un CMake superior.

## Cableado

El display debe funcionar a 3,3 V:

| OLED | RP2040-Zero |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO0 |
| SCL | GPIO1 |

La dirección configurada es `0x3C` y se utiliza `i2c0` a 400 kHz. No alimentar
las líneas GPIO con 5 V. Muchos módulos ya incorporan resistencias pull-up; el
programa habilita además las pull-up internas, que son débiles.

## Texto configurable

Editar al comienzo de `testOLED.c`:

```c
#define DISPLAY_TEXT "PIXIE PICO"
#define DISPLAY_TEXT_SCALE 2
```

La fuente admite letras A-Z (también convierte minúsculas a mayúsculas), números,
espacio, punto, guion, guion bajo y dos puntos. Un texto demasiado largo será
recortado por los límites físicos de 128 píxeles.

## Configurar y compilar en VS Code

1. `Ctrl+Shift+P`.
2. Ejecutar **Raspberry Pi Pico: Import Pico Project** si todavía no fue importado.
3. Seleccionar Waveshare RP2040-Zero, plataforma RP2040 y el SDK instalado.
4. Ejecutar **Raspberry Pi Pico: Configure CMake**.
5. Confirmar en Output:

   ```text
   Target board (PICO_BOARD) is 'waveshare_rp2040_zero'
   ```

6. Ejecutar **Raspberry Pi Pico: Compile Pico Project**.

El resultado será:

```text
build\testOLED.uf2
```

Para una compilación limpia ejecutar **Terminal > Run Task > testOLED: borrar
build**, configurar CMake nuevamente y compilar.

## Compilar desde la terminal integrada

Si `build` ya fue configurado:

```powershell
cmake --build build --parallel
```

Si `cmake` no está en PATH, usar el Ninja administrado por la extensión (ajustar
la versión si fuera diferente):

```powershell
& "$env:USERPROFILE\.pico-sdk\ninja\v1.13.2\ninja.exe" -C build
```

## Cargar y probar

1. Conectar la placa manteniendo BOOT presionado.
2. Copiar `build\testOLED.uf2` a la unidad `RPI-RP2`.
3. Reconectar normalmente o dejar que reinicie.
4. El OLED mostrará `PIXIE PICO` y el WS2812 alternará verde/apagado cada 500 ms.
5. Abrir el nuevo puerto COM a 115200, 8N1, sin control de flujo.
6. Presionar RESET para ver los mensajes iniciales.

Salida esperada:

```text
testOLED iniciado en Waveshare RP2040-Zero
LED WS2812: GPIO 16
OLED: SSD1306 128x32, I2C0, SDA=GPIO0, SCL=GPIO1, addr=0x3C
OLED detectado; texto: PIXIE PICO
Ciclo 1: LED encendido
```

## Diagnóstico

- Si el LED y el puerto COM funcionan pero el OLED no, revisar primero VCC/GND,
  SDA/SCL cruzados y dirección. Algunos módulos usan `0x3D`; en ese caso cambiar
  `OLED_I2C_ADDRESS` en `testOLED.c`.
- Si el monitor dice `OLED NO detectado`, el SSD1306 no respondió durante la
  inicialización: es casi siempre alimentación, cableado o dirección.
- Si el texto aparece invertido o espejado, el módulo usa una orientación física
  distinta. Cambiar `0xA1` por `0xA0` o `0xC8` por `0xC0` en `ssd1306.c`.
- Si aparece texto sólo en una fracción del panel, confirmar que sea realmente
  128x32 y no 128x64.
- Si no hay consola, identificar el COM que aparece después de cargar el UF2,
  abrirlo y presionar RESET. El programa espera hasta 15 segundos por el monitor
  y hace `fflush(stdout)` después de cada mensaje.
