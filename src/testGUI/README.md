# testGUI

Proyecto autónomo para Waveshare RP2040-Zero. Parte de la última versión de
`testOLED` y agrega un encoder rotativo KY-040 con pulsador.

## Instalación

Descomprimir el ZIP directamente en:

```text
C:\Users\pedro\github\PixiePico\src
```

Debe quedar `C:\Users\pedro\github\PixiePico\src\testGUI`. Abrir exactamente
esa carpeta en Visual Studio Code e importar el proyecto como
`waveshare_rp2040_zero`.

## Cableado

Todo el sistema trabaja a 3,3 V. No aplicar 5 V a ningún GPIO.

| Dispositivo | Señal | RP2040-Zero |
|---|---|---|
| SSD1306 | SDA | GPIO0 |
| SSD1306 | SCL | GPIO1 |
| KY-040 | CLK / A | GPIO29 |
| KY-040 | DT / B | GPIO28 |
| KY-040 | SW | GPIO27 |
| KY-040 | + | 3V3 |
| KY-040 | GND | GND |

El firmware habilita pull-ups internas en A, B y SW. El módulo KY-040 debe
alimentarse a 3,3 V porque algunos módulos conectan sus pull-ups a `+`.

## API del encoder

```c
void rotaryTurn(int direction);
void SWclick(bool pressed);
```

- Cada detent horario llama `rotaryTurn(+1)`.
- Cada detent antihorario llama `rotaryTurn(-1)`.
- Una presión estable llama `SWclick(true)`.
- La liberación estable llama `SWclick(false)`.

La demostración modifica la frecuencia en 10 Hz por detent y presenta TX
mientras SW permanece presionado. Estas dos funciones son el punto destinado a
la lógica de aplicación y pueden modificarse sin tocar el driver.

El decodificador A/B funciona por interrupciones en ambos flancos y valida la
secuencia cuadratura completa. Sólo acumula eventos dentro de la interrupción;
las funciones de usuario se ejecutan en el bucle principal, donde es seguro
actualizar el OLED y usar `printf()`. SW se muestrea con antirrebote de 20 ms.

## API heredada del OLED

```c
void displayMode(const char *mode);
void displayVFO(const char *vfo);
void displayTX(bool transmitting);
void displayLED(unsigned level);
void displayFreq(long frequency_hz);
```

Se conservan el OLED SSD1306 128x32, la consola USB CDC, el WS2812 en GPIO16 y
el contador visual 0-5 asociado a cada transición del blink.

## Configurar y compilar

La configuración contiene la ruta ya verificada:

```text
C:/Users/pedro/.pico-sdk/cmake/v4.3.4/bin/cmake.exe
```

En VS Code:

1. Ejecutar **CMake: Configure**.
2. Confirmar `PICO_BOARD=waveshare_rp2040_zero`.
3. Ejecutar **CMake: Build**.

Desde PowerShell, después de configurar:

```powershell
& "C:\Users\pedro\.pico-sdk\cmake\v4.3.4\bin\cmake.exe" `
    --build build --parallel
```

El firmware será `build\testGUI.uf2`.

## Prueba

1. Cargar `testGUI.uf2` mediante BOOT/RPI-RP2.
2. Abrir el puerto COM a 115200, 8N1, sin control de flujo.
3. Girar un detent horario: la frecuencia aumenta 10 Hz y aparece `+1` por serie.
4. Girar antihorario: disminuye 10 Hz y aparece `-1`.
5. Presionar SW: la pantalla pasa a TX.
6. Soltar SW: la pantalla vuelve a RX.

Si el sentido resulta invertido por una variante física del KY-040, intercambiar
las definiciones `ENCODER_CLK_PIN` y `ENCODER_DT_PIN` en `testGUI.c`, o invertir
los signos en `rotaryTurn()`.
