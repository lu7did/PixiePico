# blink — proyecto autónomo para RP2040-Zero

Este directorio es un proyecto CMake completamente independiente. No utiliza
archivos de `PixiePico`, de `PixiePico/src` ni de ningún otro subproyecto.

La carpeta debe quedar exactamente en:

```text
C:\Users\pedro\github\PixiePico\src\blink
```

## 1. Contenido final

```text
blink\
|-- .vscode\
|   |-- extensions.json
|   |-- settings.json
|   `-- tasks.json
|-- CMakeLists.txt
|-- pico_sdk_import.cmake
|-- blink.c
|-- ws2812.pio
|-- .gitignore
`-- README.md
```

Cada futuro subproyecto deberá repetir esta idea:

```text
PixiePico\src\blink
PixiePico\src\oled
PixiePico\src\usb_hid
```

Cada uno tendrá su propio `CMakeLists.txt`, `pico_sdk_import.cmake`, `.vscode`,
fuentes y `build`. Para trabajar en uno se abre su carpeta individual en VS
Code. No se abre `PixiePico` ni `PixiePico\src` como workspace.

## 2. Instalación inicial en Windows 11

### 2.1 Instalar Visual Studio Code

1. Descargar e instalar Visual Studio Code de 64 bits.
2. Durante la instalación conviene habilitar:
   - `Add to PATH`.
   - `Open with Code` para carpetas.
3. Abrir VS Code y verificar que esté actualizado. La extensión oficial actual
   requiere VS Code 1.105.1 o posterior.

### 2.2 Instalar la extensión oficial

1. Abrir **Extensions** con `Ctrl+Shift+X`.
2. Buscar **Raspberry Pi Pico**.
3. Instalar la extensión cuyo identificador es:

   ```text
   raspberry-pi.raspberry-pi-pico
   ```

4. Reiniciar VS Code si lo solicita.

La extensión administra Pico SDK, CMake, Ninja, Python, Git, OpenOCD y el
compilador ARM. En una instalación normal no hay que configurar manualmente
esas rutas ni crear una variable global `PICO_SDK_PATH`.

### 2.3 Instalar las herramientas del SDK

1. Abrir la paleta con `Ctrl+Shift+P`.
2. Ejecutar **Raspberry Pi Pico: New Pico Project** una vez, sólo para permitir
   que la extensión descargue el SDK y las herramientas. Se puede cancelar la
   creación después de completar la instalación.
3. Elegir una versión estable reciente del Pico SDK.
4. Esperar a que todas las descargas terminen sin errores.

## 3. Instalar este subproyecto

1. Crear, si todavía no existen, estos directorios:

   ```text
   C:\Users\pedro\github\PixiePico\src
   ```

2. Descomprimir el ZIP directamente dentro de `src`. El ZIP ya contiene la
   carpeta raíz `blink`; no crear otra carpeta `blink` manualmente.
3. Verificar que no haya una duplicación como `src\blink\blink`.
4. El archivo que debe existir es:

   ```text
   C:\Users\pedro\github\PixiePico\src\blink\CMakeLists.txt
   ```

## 4. Abrir e importar el proyecto

1. En VS Code seleccionar **File > Open Folder...**.
2. Abrir exactamente:

   ```text
   C:\Users\pedro\github\PixiePico\src\blink
   ```

3. Marcar la carpeta como confiable si VS Code lo pregunta.
4. La presencia de `pico_sdk_import.cmake` activa la extensión oficial.
5. Si aparece **Import Project**, aceptarlo. En caso contrario ejecutar desde
   `Ctrl+Shift+P`:

   ```text
   Raspberry Pi Pico: Import Pico Project
   ```

6. Seleccionar:
   - Board: `waveshare_rp2040_zero` / Waveshare RP2040-Zero.
   - Platform: RP2040.
   - Build type: Debug para comenzar.
   - SDK: la versión estable instalada.

El `CMakeLists.txt` fija también `PICO_BOARD=waveshare_rp2040_zero`. Esto es
deliberado: impide que una caché nueva seleccione silenciosamente la placa
`pico`, cuya configuración de LED no corresponde a la RP2040-Zero.

## 5. Construir desde cero

### Primera compilación

1. Abrir `Ctrl+Shift+P`.
2. Ejecutar:

   ```text
   Raspberry Pi Pico: Configure CMake
   ```

3. Comprobar en la salida que aparezca una línea equivalente a:

   ```text
   Target board (PICO_BOARD) is 'waveshare_rp2040_zero'
   ```

4. Ejecutar:

   ```text
   Raspberry Pi Pico: Compile Pico Project
   ```

El firmware esperado queda en:

```text
C:\Users\pedro\github\PixiePico\src\blink\build\blink.uf2
```

También se generan `blink.elf`, `blink.bin`, `blink.hex` y `blink.map`.

### Reconstrucción completamente limpia

1. Ejecutar **Terminal > Run Task... > blink: borrar build**.
2. Ejecutar nuevamente **Raspberry Pi Pico: Configure CMake**.
3. Confirmar otra vez la placa Waveshare.
4. Ejecutar **Raspberry Pi Pico: Compile Pico Project**.

No se debe reutilizar el directorio `build` de otro subproyecto.

## 6. Cargar `blink.uf2`

1. Desconectar la RP2040-Zero del USB.
2. Mantener presionado el botón **BOOT**.
3. Conectar el cable USB al PC mientras se mantiene BOOT.
4. Soltar BOOT cuando Windows monte la unidad `RPI-RP2`.
5. Copiar `build\blink.uf2` a `RPI-RP2`.
6. La placa reiniciará y la unidad desaparecerá automáticamente.

El LED RGB integrado debe alternar verde/apagado cada 500 ms.

## 7. Consola serie USB

Este programa usa USB CDC mediante el mismo conector USB de la placa. No usa
UART y no requiere adaptador externo.

1. Esperar a que Windows cree el puerto COM.
2. Buscarlo en **Administrador de dispositivos > Puertos (COM y LPT)**.
3. Abrir un monitor serie en VS Code y seleccionar ese COM.
4. Usar 115200 bit/s, 8 bits, sin paridad, 1 bit de parada, sin control de
   flujo. USB CDC ignora realmente el baud rate, pero 115200 es la convención.
5. Presionar RESET para volver a ver los mensajes iniciales.

Salida esperada:

```text
blink iniciado en Waveshare RP2040-Zero
LED WS2812 conectado a GPIO 16
Ciclo 1: LED encendido
Ciclo 2: LED apagado
```

## 8. Por qué no se usa el blink convencional de GPIO25

La RP2040-Zero no incorpora el LED simple conectado a GPIO25 que tiene el
Raspberry Pi Pico. Su LED integrado es un WS2812 direccionable conectado a
GPIO16. Por eso son necesarios `ws2812.pio` y `hardware_pio`. Escribir solamente
`gpio_put(25, ...)` compilaría en algunos contextos, pero no haría parpadear el
LED integrado de esta placa.

## 9. Diagnóstico

- **No se activa la extensión:** comprobar que se abrió `src\blink` como carpeta
  raíz y que contiene `pico_sdk_import.cmake`.
- **PICO_SDK_PATH no está definido:** ejecutar **Import Pico Project** o
  **Switch Pico SDK**. No lanzar CMake desde un PowerShell común sin el entorno
  de la extensión.
- **CMake muestra la placa `pico`:** borrar `build`, configurar nuevamente y
  verificar que el `CMakeLists.txt` no haya sido modificado.
- **No aparece `build\blink.uf2`:** revisar la pestaña Output de la extensión;
  la ausencia del UF2 es consecuencia de un error anterior, no un problema de
  copia.
- **No aparece el puerto COM:** probar otro cable USB; muchos cables sólo sirven
  para alimentación. Revisar también el Administrador de dispositivos.
- **Hay texto serie pero no parpadeo:** confirmar que la placa sea realmente una
  Waveshare RP2040-Zero con WS2812 en GPIO16.
- **Hay parpadeo pero no texto:** abrir el COM correcto y presionar RESET. Los
  primeros mensajes pueden perderse si el monitor se conecta tarde.
