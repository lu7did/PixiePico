#ifndef PIXIEPICO_DDSVCO_H
#define PIXIEPICO_DDSVCO_H

#include <stdbool.h>
#include <stdint.h>

#include "hardware/pio.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DDSVCO_XOSC_HZ
#define DDSVCO_XOSC_HZ 12000000u
#endif

#ifndef DDSVCO_SYS_CLK_MIN_HZ
#define DDSVCO_SYS_CLK_MIN_HZ 125000000u
#endif

#ifndef DDSVCO_SYS_CLK_MAX_HZ
#define DDSVCO_SYS_CLK_MAX_HZ 250000000u
#endif

#ifndef DDSVCO_DEFAULT_GPIO
#define DDSVCO_DEFAULT_GPIO 14u
#endif

#define DDSVCO_PENDING_MAGIC 0x56434f31u

typedef enum {
    DDSVCO_SETFREQ_FAILED = 0,
    DDSVCO_SETFREQ_APPLIED,
    DDSVCO_SETFREQ_REBOOT_REQUIRED,
    DDSVCO_SETFREQ_REBOOTING
} ddsvco_setfreq_result_t;

typedef struct {
    PIO pio;
    int state_machine;             /* -1: reclamar una SM libre. */
    uint output_gpio;
    uint32_t sys_clk_min_hz;
    uint32_t sys_clk_max_hz;
    bool reboot_on_pll_change;
} ddsvco_config_t;

typedef struct {
    uint32_t target_hz;

    uint8_t pll_refdiv;
    uint16_t pll_fbdiv;
    uint8_t pll_postdiv1;
    uint8_t pll_postdiv2;
    uint32_t pll_ref_hz;
    uint32_t pll_vco_hz;

    /* Frecuencia exacta: sys_clk_num / sys_clk_den. */
    uint64_t sys_clk_num;
    uint32_t sys_clk_den;
    uint32_t sys_clk_hz;           /* Valor redondeado para reporte/SDK. */

    /* CLKDIV PIO Q16.8: D = pio_divider_q8 / 256. */
    uint32_t pio_divider_q8;
    uint16_t pio_divider_int;
    uint8_t pio_divider_frac;

    double achieved_hz;
    double error_hz;
    double error_ppm;
} ddsvco_solution_t;

typedef struct {
    ddsvco_config_t config;
    ddsvco_solution_t active_solution;
    uint program_offset;
    int state_machine;
    bool initialized;
    bool running;
    bool has_active_solution;
} ddsvco_t;

/* Inicializa GPIO, PIO y recursos. No cambia PLL_SYS ni inicia la salida. */
bool ddsvco_init(ddsvco_t *vco, const ddsvco_config_t *config);

/* Busca el óptimo global estático dentro del rango configurado. */
bool ddsvco_solve(const ddsvco_t *vco, uint32_t target_hz,
                  ddsvco_solution_t *solution);

/* Aplica una solución ya calculada e inicia la salida. En el arranque debe
 * llamarse antes de stdio_init_all() cuando la solución cambia PLL_SYS. */
bool ddsvco_start(ddsvco_t *vco, const ddsvco_solution_t *solution);

/* Detiene la PIO y fuerza GPIO a nivel bajo. */
void ddsvco_stop(ddsvco_t *vco);

/* Resuelve y aplica una frecuencia. Por defecto el PLL se cambia en vivo,
 * manteniendo clk_sys y clk_peri temporalmente sobre PLL_USB. Si
 * reboot_on_pll_change es true, se conserva el reinicio como alternativa. */
ddsvco_setfreq_result_t ddsvco_setfreq(ddsvco_t *vco, uint32_t target_hz,
                                       ddsvco_solution_t *solution_out);

/* Recupera una frecuencia pendiente después de un reinicio por watchdog. */
bool ddsvco_take_pending_frequency(uint32_t *target_hz);

const char *ddsvco_setfreq_result_name(ddsvco_setfreq_result_t result);

#ifdef __cplusplus
}
#endif

#endif
