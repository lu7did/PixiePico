/*
Project PixiePico
ddsvco.c/ddsvco.h

Pixie based digital transceiver firmware
DDS component implementing the optimal solution to achieve the
required frequency with the minimum possible error.

Copyright Dr. Pedro E. Colla LU7DZ (2026)
For non-profit uses only
================================================================
Este programa  disponible es hecho público
bajo la licencia Creative Commons Attribution-ShareAlike 4.0
International (CC BY-SA 4.0).

*/

#include "ddsvco.h"

#include <float.h>
#include <math.h>
#include <string.h>

#include "ddsvco_square.pio.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pll.h"
#include "hardware/regs/clocks.h"
#include "hardware/vreg.h"
#include "hardware/watchdog.h"
#include "pico/stdlib.h"

#define DDSVCO_PLL_REF_MIN_HZ 5000000u
#define DDSVCO_PLL_VCO_MIN_HZ 750000000u
#define DDSVCO_PLL_VCO_MAX_HZ 1600000000u
#define DDSVCO_PLL_FBDIV_MIN 16u
#define DDSVCO_PLL_FBDIV_MAX 320u
#define DDSVCO_PLL_REFDIV_MAX 63u
#define DDSVCO_PLL_POSTDIV_MAX 7u
#define DDSVCO_PIO_DIV_Q8_MIN 256u
#define DDSVCO_PIO_DIV_Q8_MAX 0x00ffffffu

typedef struct {
    bool valid;
    uint64_t abs_error_num;
    uint64_t error_den;
    uint32_t vco_hz;
    uint32_t sys_hz_rounded;
    uint32_t divider_q8;
} candidate_rank_t;

static uint32_t round_ratio_u64(uint64_t numerator, uint64_t denominator) {
    if (denominator == 0u) {
        return 0u;
    }
    uint64_t value = (numerator + denominator / 2u) / denominator;
    return value > UINT32_MAX ? UINT32_MAX : (uint32_t)value;
}

static bool same_pll(const ddsvco_solution_t *a,
                     const ddsvco_solution_t *b) {
    return a->pll_refdiv == b->pll_refdiv &&
           a->pll_fbdiv == b->pll_fbdiv &&
           a->pll_postdiv1 == b->pll_postdiv1 &&
           a->pll_postdiv2 == b->pll_postdiv2;
}

/* Compara n1/d1 con n2/d2 exactamente, usando solamente uint64_t.
 * Devuelve -1, 0 o +1. La expansión en fracción continua evita el producto
 * cruzado de 128 bits, que no está disponible en Cortex-M0+. */
static int compare_u64_fractions(uint64_t n1, uint64_t d1,
                                 uint64_t n2, uint64_t d2) {
    int direction = 1;

    while (true) {
        const uint64_t q1 = n1 / d1;
        const uint64_t q2 = n2 / d2;
        if (q1 < q2) {
            return -direction;
        }
        if (q1 > q2) {
            return direction;
        }

        const uint64_t r1 = n1 % d1;
        const uint64_t r2 = n2 % d2;
        if (r1 == 0u || r2 == 0u) {
            if (r1 == 0u && r2 == 0u) {
                return 0;
            }
            return r1 == 0u ? -direction : direction;
        }

        /* Comparar r1/d1 con r2/d2 equivale a comparar d2/r2 con
         * d1/r1 en el sentido inverso. */
        n1 = d1;
        d1 = r1;
        n2 = d2;
        d2 = r2;
        direction = -direction;
    }
}

static bool rank_is_better(uint64_t abs_num, uint64_t den,
                           uint32_t vco_hz, uint32_t sys_hz,
                           uint32_t divider_q8,
                           const candidate_rank_t *best) {
    if (!best->valid) {
        return true;
    }

    const int error_order = compare_u64_fractions(
        abs_num, den, best->abs_error_num, best->error_den);
    if (error_order != 0) {
        return error_order < 0;
    }

    /* Empates: mayor VCO (menor jitter), menor SYS, menor divisor. */
    if (vco_hz != best->vco_hz) {
        return vco_hz > best->vco_hz;
    }
    if (sys_hz != best->sys_hz_rounded) {
        return sys_hz < best->sys_hz_rounded;
    }
    return divider_q8 < best->divider_q8;
}

static void evaluate_candidate(uint32_t target_hz,
                               uint refdiv, uint fbdiv,
                               uint postdiv1, uint postdiv2,
                               uint32_t vco_hz,
                               uint32_t divider_q8,
                               candidate_rank_t *rank,
                               ddsvco_solution_t *best) {
    if (divider_q8 < DDSVCO_PIO_DIV_Q8_MIN ||
        divider_q8 > DDSVCO_PIO_DIV_Q8_MAX) {
        return;
    }

    /* fout = A/B = 128*XOSC*FBDIV /
     * (REFDIV*POSTDIV1*POSTDIV2*N). */
    const uint64_t output_num =
        (uint64_t)128u * DDSVCO_XOSC_HZ * fbdiv;
    const uint64_t output_den =
        (uint64_t)refdiv * postdiv1 * postdiv2 * divider_q8;
    const uint64_t target_scaled = (uint64_t)target_hz * output_den;
    const int64_t error_num = output_num >= target_scaled
        ? (int64_t)(output_num - target_scaled)
        : -(int64_t)(target_scaled - output_num);
    const uint64_t abs_error_num = error_num >= 0
        ? (uint64_t)error_num : (uint64_t)(-error_num);

    const uint64_t sys_num = (uint64_t)DDSVCO_XOSC_HZ * fbdiv;
    const uint32_t sys_den = refdiv * postdiv1 * postdiv2;
    const uint32_t sys_hz = round_ratio_u64(sys_num, sys_den);

    if (!rank_is_better(abs_error_num, output_den, vco_hz, sys_hz,
                        divider_q8, rank)) {
        return;
    }

    const long double achieved =
        (long double)output_num / (long double)output_den;
    const long double error = achieved - (long double)target_hz;

    memset(best, 0, sizeof(*best));
    best->target_hz = target_hz;
    best->pll_refdiv = (uint8_t)refdiv;
    best->pll_fbdiv = (uint16_t)fbdiv;
    best->pll_postdiv1 = (uint8_t)postdiv1;
    best->pll_postdiv2 = (uint8_t)postdiv2;
    best->pll_ref_hz = DDSVCO_XOSC_HZ / refdiv;
    best->pll_vco_hz = vco_hz;
    best->sys_clk_num = sys_num;
    best->sys_clk_den = sys_den;
    best->sys_clk_hz = sys_hz;
    best->pio_divider_q8 = divider_q8;
    best->pio_divider_int = (uint16_t)(divider_q8 >> 8u);
    best->pio_divider_frac = (uint8_t)(divider_q8 & 0xffu);
    best->achieved_hz = (double)achieved;
    best->error_hz = (double)error;
    best->error_ppm = (double)(error * 1000000.0L / target_hz);

    rank->valid = true;
    rank->abs_error_num = abs_error_num;
    rank->error_den = output_den;
    rank->vco_hz = vco_hz;
    rank->sys_hz_rounded = sys_hz;
    rank->divider_q8 = divider_q8;
}

bool ddsvco_init(ddsvco_t *vco, const ddsvco_config_t *config) {
    if (vco == NULL || config == NULL || config->pio == NULL ||
        config->output_gpio >= NUM_BANK0_GPIOS ||
        config->sys_clk_min_hz == 0u ||
        config->sys_clk_min_hz > config->sys_clk_max_hz) {
        return false;
    }

    memset(vco, 0, sizeof(*vco));
    vco->config = *config;
    vco->state_machine = config->state_machine;
    if (vco->state_machine < 0) {
        vco->state_machine = (int)pio_claim_unused_sm(config->pio, true);
    } else {
        pio_sm_claim(config->pio, (uint)vco->state_machine);
    }

    if (!pio_can_add_program(config->pio, &ddsvco_square_program)) {
        return false;
    }
    vco->program_offset =
        pio_add_program(config->pio, &ddsvco_square_program);

    gpio_init(config->output_gpio);
    gpio_set_dir(config->output_gpio, GPIO_OUT);
    gpio_put(config->output_gpio, false);
    vco->initialized = true;
    return true;
}

bool ddsvco_solve(const ddsvco_t *vco, uint32_t target_hz,
                  ddsvco_solution_t *solution) {
    if (vco == NULL || !vco->initialized || solution == NULL ||
        target_hz == 0u) {
        return false;
    }

    candidate_rank_t rank = {0};
    ddsvco_solution_t best = {0};

    for (uint refdiv = 1u; refdiv <= DDSVCO_PLL_REFDIV_MAX; ++refdiv) {
        if (DDSVCO_XOSC_HZ / refdiv < DDSVCO_PLL_REF_MIN_HZ) {
            continue;
        }
        if (DDSVCO_XOSC_HZ % refdiv != 0u) {
            continue;
        }
        const uint32_t ref_hz = DDSVCO_XOSC_HZ / refdiv;

        for (uint fbdiv = DDSVCO_PLL_FBDIV_MIN;
             fbdiv <= DDSVCO_PLL_FBDIV_MAX; ++fbdiv) {
            const uint64_t vco64 = (uint64_t)ref_hz * fbdiv;
            if (vco64 < DDSVCO_PLL_VCO_MIN_HZ ||
                vco64 > DDSVCO_PLL_VCO_MAX_HZ) {
                continue;
            }
            const uint32_t vco_hz = (uint32_t)vco64;
            if (ref_hz > vco_hz / DDSVCO_PLL_FBDIV_MIN) {
                continue;
            }

            for (uint postdiv1 = 1u;
                 postdiv1 <= DDSVCO_PLL_POSTDIV_MAX; ++postdiv1) {
                for (uint postdiv2 = 1u;
                     postdiv2 <= postdiv1; ++postdiv2) {
                    const uint32_t pll_den =
                        refdiv * postdiv1 * postdiv2;
                    const uint64_t sys_num =
                        (uint64_t)DDSVCO_XOSC_HZ * fbdiv;

                    if (sys_num <
                            (uint64_t)vco->config.sys_clk_min_hz * pll_den ||
                        sys_num >
                            (uint64_t)vco->config.sys_clk_max_hz * pll_den) {
                        continue;
                    }

                    /* N ideal = 128*XOSC*FBDIV /
                     * (target*REFDIV*POSTDIV1*POSTDIV2). */
                    const uint64_t ideal_num =
                        (uint64_t)128u * DDSVCO_XOSC_HZ * fbdiv;
                    const uint64_t ideal_den =
                        (uint64_t)target_hz * pll_den;
                    const uint64_t floor_n = ideal_num / ideal_den;

                    if (floor_n <= UINT32_MAX) {
                        evaluate_candidate(target_hz, refdiv, fbdiv,
                                           postdiv1, postdiv2, vco_hz,
                                           (uint32_t)floor_n, &rank, &best);
                    }
                    if (floor_n < UINT32_MAX) {
                        evaluate_candidate(target_hz, refdiv, fbdiv,
                                           postdiv1, postdiv2, vco_hz,
                                           (uint32_t)(floor_n + 1u),
                                           &rank, &best);
                    }
                }
            }
        }
    }

    if (!rank.valid || !isfinite(best.achieved_hz) ||
        !isfinite(best.error_hz)) {
        return false;
    }
    *solution = best;
    return true;
}

static bool apply_system_clock(const ddsvco_solution_t *solution) {
    if (solution == NULL) {
        return false;
    }

    /* pll_init calcula FBDIV a partir de VCO/ref. Verificación defensiva. */
    const uint32_t ref_hz = DDSVCO_XOSC_HZ / solution->pll_refdiv;
    if (ref_hz * solution->pll_fbdiv != solution->pll_vco_hz) {
        return false;
    }

    if (solution->sys_clk_hz > 200000000u) {
        vreg_set_voltage(VREG_VOLTAGE_1_20);
        sleep_ms(10);
    }

    /* Igual que set_sys_clock_pll(), pero REFDIV es parte de la solución.
     * PLL_USB permanece a 48 MHz y alimenta temporalmente clk_sys. */
    clock_configure_undivided(
        clk_sys,
        CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
        CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,
        USB_CLK_HZ);

    pll_init(pll_sys, solution->pll_refdiv, solution->pll_vco_hz,
             solution->pll_postdiv1, solution->pll_postdiv2);

    clock_configure_undivided(
        clk_sys,
        CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
        CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS,
        solution->sys_clk_hz);

    /* Mantener periféricos en PLL_USB evita alterar USB/UART/I2C por clk_peri. */
    clock_configure_undivided(
        clk_peri, 0,
        CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,
        USB_CLK_HZ);

    return clock_get_hz(clk_sys) == solution->sys_clk_hz;
}

static void configure_pio(ddsvco_t *vco,
                          const ddsvco_solution_t *solution) {
    const PIO pio = vco->config.pio;
    const uint sm = (uint)vco->state_machine;
    pio_sm_config config =
        ddsvco_square_program_get_default_config(vco->program_offset);
    sm_config_set_sideset_pins(&config, vco->config.output_gpio);
    sm_config_set_clkdiv_int_frac8(&config,
                                   solution->pio_divider_int,
                                   solution->pio_divider_frac);

    pio_gpio_init(pio, vco->config.output_gpio);
    pio_sm_set_consecutive_pindirs(pio, sm, vco->config.output_gpio, 1u, true);
    pio_sm_init(pio, sm, vco->program_offset, &config);
    pio_sm_restart(pio, sm);
}

bool ddsvco_start(ddsvco_t *vco, const ddsvco_solution_t *solution) {
    if (vco == NULL || !vco->initialized || solution == NULL ||
        solution->pio_divider_q8 < DDSVCO_PIO_DIV_Q8_MIN ||
        solution->pio_divider_q8 > DDSVCO_PIO_DIV_Q8_MAX) {
        return false;
    }

    ddsvco_stop(vco);
    if (!vco->has_active_solution ||
        !same_pll(&vco->active_solution, solution)) {
        if (!apply_system_clock(solution)) {
            return false;
        }
    }

    configure_pio(vco, solution);
    vco->active_solution = *solution;
    vco->has_active_solution = true;
    vco->running = true;
    pio_sm_set_enabled(vco->config.pio, (uint)vco->state_machine, true);
    return true;
}

void ddsvco_stop(ddsvco_t *vco) {
    if (vco == NULL || !vco->initialized) {
        return;
    }

    vco->running = false;
    pio_sm_set_enabled(vco->config.pio, (uint)vco->state_machine, false);
    pio_sm_restart(vco->config.pio, (uint)vco->state_machine);
    gpio_set_function(vco->config.output_gpio, GPIO_FUNC_SIO);
    gpio_set_dir(vco->config.output_gpio, GPIO_OUT);
    gpio_put(vco->config.output_gpio, false);
}

ddsvco_setfreq_result_t ddsvco_setfreq(ddsvco_t *vco, uint32_t target_hz,
                                       ddsvco_solution_t *solution_out) {
    ddsvco_solution_t solution;
    if (!ddsvco_solve(vco, target_hz, &solution)) {
        return DDSVCO_SETFREQ_FAILED;
    }
    if (solution_out != NULL) {
        *solution_out = solution;
    }

    if (vco->has_active_solution &&
        !same_pll(&vco->active_solution, &solution)) {
        if (vco->config.reboot_on_pll_change) {
            watchdog_hw->scratch[6] = DDSVCO_PENDING_MAGIC;
            watchdog_hw->scratch[7] = target_hz;
            watchdog_reboot(0u, 0u, 10u);
            while (true) {
                tight_loop_contents();
            }
        }
        /* Sin reinicio: ddsvco_start() conmuta clk_sys a PLL_USB, configura
         * pll_sys con los nuevos divisores y vuelve a conectarlo. clk_usb y
         * clk_peri continúan a 48 MHz durante toda la operación. */
    }

    return ddsvco_start(vco, &solution)
        ? DDSVCO_SETFREQ_APPLIED : DDSVCO_SETFREQ_FAILED;
}

bool ddsvco_take_pending_frequency(uint32_t *target_hz) {
    if (target_hz == NULL ||
        watchdog_hw->scratch[6] != DDSVCO_PENDING_MAGIC) {
        return false;
    }
    const uint32_t pending = watchdog_hw->scratch[7];
    watchdog_hw->scratch[6] = 0u;
    watchdog_hw->scratch[7] = 0u;
    if (pending == 0u) {
        return false;
    }
    *target_hz = pending;
    return true;
}

const char *ddsvco_setfreq_result_name(ddsvco_setfreq_result_t result) {
    switch (result) {
        case DDSVCO_SETFREQ_APPLIED:
            return "aplicada";
        case DDSVCO_SETFREQ_REBOOT_REQUIRED:
            return "requiere reinicio";
        case DDSVCO_SETFREQ_REBOOTING:
            return "reiniciando";
        case DDSVCO_SETFREQ_FAILED:
        default:
            return "error";
    }
}
