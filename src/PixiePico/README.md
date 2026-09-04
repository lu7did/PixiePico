# testVCO - PixiePico

Proyecto autónomo para Waveshare RP2040-Zero, derivado de `testDDS`.
Conserva OLED SSD1306, encoder KY-040, WS2812 y USB CDC. Sustituye la
modulación Bresenham/DMA anterior por un VCO PIO estático en GPIO14 y un
optimizador conjunto de PLL y divisor PIO.

## Modelo

La PIO ejecuta dos instrucciones por período y su divisor es Q16.8:

```
fout = 128 * XOSC * FBDIV /
       (REFDIV * POSTDIV1 * POSTDIV2 * N)

N = 256 * CLKDIV_INT + CLKDIV_FRAC
```

`ddsvco_solve()` enumera exhaustivamente:

- REFDIV permitido por `XOSC/REFDIV >= 5 MHz`;
- FBDIV 16..320;
- VCO 750..1600 MHz;
- POSTDIV1/POSTDIV2 1..7, con POSTDIV1 >= POSTDIV2;
- PLL_SYS dentro del rango configurado, por defecto 125..250 MHz;
- los dos enteros `N` adyacentes al divisor PIO ideal.

La comparación de errores utiliza solamente enteros de 64 bits y un algoritmo
de fracciones continuas que evita productos cruzados con overflow. No se
impone un error máximo: siempre se devuelve el mínimo estático representable. La
frecuencia alcanzada, el error en Hz y el error en ppm son teóricos y no
incluyen tolerancia ni deriva del cristal.

## API

```c
bool ddsvco_init(ddsvco_t *vco, const ddsvco_config_t *config);
bool ddsvco_solve(const ddsvco_t *vco, uint32_t target_hz,
                  ddsvco_solution_t *solution);
bool ddsvco_start(ddsvco_t *vco, const ddsvco_solution_t *solution);
void ddsvco_stop(ddsvco_t *vco);
ddsvco_setfreq_result_t ddsvco_setfreq(
    ddsvco_t *vco, uint32_t target_hz,
    ddsvco_solution_t *solution_out);
```

## Orden de arranque y USB

`main.c` recupera primero una frecuencia pendiente y ejecuta `init`, `solve`
y `start` antes de `stdio_init_all()`. Esto evita reconfigurar `PLL_SYS`
mientras TinyUSB está enumerado.

Durante la operación, `setfreq()` aplica directamente un nuevo divisor si la
solución conserva la misma configuración PLL. Si la solución óptima exige otro
PLL, el comportamiento predeterminado (`reboot_on_pll_change=false`) realiza el
cambio en vivo: conecta temporalmente `clk_sys` a `PLL_USB`, reconfigura
`pll_sys` y vuelve a seleccionarlo. `clk_usb` y `clk_peri` permanecen a 48 MHz,
por lo que el USB CDC no debe resetearse ni volver a enumerarse.

El reinicio por watchdog queda disponible como alternativa de diagnóstico
estableciendo `reboot_on_pll_change=true`. En ese modo se guarda la frecuencia
en los scratch registers 6/7 y el nuevo arranque aplica el PLL antes de iniciar
USB.

## Recursos predeterminados

- VCO: PIO1, state machine dinámica, GPIO14.
- WS2812: PIO0/SM0, GPIO16.
- OLED: I2C0, SDA GPIO0, SCL GPIO1.
- Encoder: GPIO29/28/27.
- PLL_SYS: 125..250 MHz, parametrizable mediante
  `DDSVCO_SYS_CLK_MIN_HZ` y `DDSVCO_SYS_CLK_MAX_HZ`.

## Importar y compilar en Visual Studio Code

1. Descomprimir `testVCO.zip`.
2. Ejecutar **Raspberry Pi Pico: Import Pico Project**.
3. Seleccionar la carpeta `testVCO`.
4. Confirmar `waveshare_rp2040_zero` y Pico SDK 2.3.0.
5. Ejecutar **Compile Project**.
6. Cargar `build/testVCO.uf2`.

También puede ejecutarse `build.cmd` desde la terminal integrada configurada
por la extensión Raspberry Pi Pico.

## Verificación

`tools/verify_solver.py` reproduce el algoritmo en el host y comprueba los
resultados esperados para 7.074, 14.074 y 28.074 MHz.

La compilación no verifica estabilidad ni pureza espectral. Antes de transmitir
se debe medir GPIO14 con un frecuencímetro referenciado y un analizador de
espectro. Operar por encima de la especificación nominal del RP2040 requiere
validación térmica y funcional sobre cada placa.
