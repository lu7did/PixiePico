# testDDS - PixiePico

Proyecto autónomo para Waveshare RP2040-Zero, derivado de testGUI.
Conserva OLED SSD1306, encoder KY-040, WS2812 y USB CDC, y agrega un
generador DDS de salida simple en GPIO14.

## Estructura

- src/main.c: integración GUI, encoder, USB y API DDS.
- src/dds.c, src/dds.h: solver PLL/PIO, control de reloj, DMA y API.
- src/dds_square.pio: salida cuadrada con decisión Bresenham por
  semiperíodo.
- src/ssd1306.*, src/rotary_encoder.*, src/ws2812.pio: módulos de testGUI.
- CMakeLists.txt: genera ambos headers PIO y enlaza clocks, PIO, DMA,
  watchdog y VREG.

## API

    bool dds_init(dds_t *dds, const dds_config_t *config);
    bool dds_solve(const dds_t *dds, uint32_t target_hz,
                   dds_solution_t *solution);
    bool dds_start(dds_t *dds, const dds_solution_t *solution);
    void dds_stop(dds_t *dds);
    dds_setfreq_result_t dds_setfreq(dds_t *dds, uint32_t target_hz,
                                     dds_solution_t *solution_out);

dds_solve() enumera los PLL_SYS alcanzables y validados por
check_sys_clock_hz() dentro de DDS_SYS_CLK_MIN_HZ y
DDS_SYS_CLK_MAX_HZ. Para cada reloj busca exhaustivamente el divisor PIO
16.8 y el número de extensiones Bresenham de un patrón de 1024
semiperíodos. No impone un error máximo: devuelve la combinación de menor
error medio representable por esta arquitectura.

La frecuencia estimada es:

    fout = 128 * fsys * L / (N * (2*L + unos))

donde L=1024, N=256*CLKDIV y cada bit uno agrega un ciclo PIO al
semiperíodo correspondiente.

## Orden de arranque obligatorio

main.c ejecuta dds_init(), dds_solve() y dds_start() antes de
stdio_init_all(). Esto aplica PLL_SYS antes de enumerar USB. No mover ese
bloque debajo de la inicialización USB.

En operación, dds_setfreq() aplica inmediatamente la nueva solución si
mantiene PLL_SYS. Si la solución global requiere otro PLL, guarda la
frecuencia en los scratch registers 6/7 del watchdog y reinicia. El nuevo
arranque recupera la frecuencia antes de iniciar USB.

## Recursos predeterminados

- DDS: PIO1, state machine reclamada dinámicamente, DMA dinámico, GPIO14.
- WS2812: PIO0/SM0, GPIO16.
- OLED: I2C0, SDA GPIO0, SCL GPIO1.
- Encoder: GPIO29/28/27.
- PLL_SYS: 125-250 MHz.

dds_t contiene un buffer DMA alineado de 8192 bytes. Debe declararse
estático/global, como se hace en main.c; no debe colocarse como variable
automática en una pila pequeña.

## Importar y compilar en Visual Studio Code

1. Descomprimir testDDS.zip.
2. En VS Code ejecutar Raspberry Pi Pico: Import Pico Project.
3. Seleccionar la carpeta testDDS.
4. Confirmar placa waveshare_rp2040_zero y Pico SDK 2.3.0.
5. Ejecutar Compile Project.
6. Cargar build/testDDS.uf2.

También puede ejecutarse build.cmd desde la terminal integrada configurada
por la extensión de Raspberry Pi Pico.

## Validación necesaria en hardware

La compilación valida la integración de SDK, pero no prueba integridad
espectral. Antes de transmitir se debe medir GPIO14 con osciloscopio,
frecuencímetro calibrado y analizador de espectro. El error calculado no
incluye tolerancia del cristal de 12 MHz, deriva térmica, ruido de fase ni
espurias periódicas del patrón.

