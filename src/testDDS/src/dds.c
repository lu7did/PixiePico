#include "dds.h"

#include <float.h>
#include <math.h>
#include <string.h>

#include "dds_square.pio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/vreg.h"
#include "hardware/watchdog.h"
#include "pico/stdlib.h"

#define DDS_XOSC_HZ 12000000u
#define DDS_PLL_VCO_MIN_HZ 750000000u
#define DDS_PLL_VCO_MAX_HZ 1600000000u
#define DDS_PIO_DIV_Q8_MIN 256u
#define DDS_PIO_DIV_Q8_MAX 0x00ffffffu

/* This module intentionally owns DMA IRQ 0. testDDS uses no other DMA client. */
static dds_t *dds_irq_owner;

static bool is_power_of_two_u32(uint32_t value) {
    return value != 0u && (value & (value - 1u)) == 0u;
}

static uint32_t rounded_div_u64(uint64_t numerator, uint64_t denominator) {
    if (denominator == 0u) {
        return 0u;
    }
    const uint64_t value = (numerator + denominator / 2u) / denominator;
    return value > UINT32_MAX ? UINT32_MAX : (uint32_t)value;
}

static void dds_dma_irq0(void) {
    dds_t *dds = dds_irq_owner;
    if (dds == NULL || dds->dma_channel < 0) {
        return;
    }

    const uint32_t mask = 1u << (uint)dds->dma_channel;
    if ((dma_hw->ints0 & mask) == 0u) {
        return;
    }
    dma_hw->ints0 = mask;

    if (!dds->running) {
        return;
    }

    dma_channel_set_read_addr((uint)dds->dma_channel,
                              dds->dma_stream, false);
    dma_channel_set_trans_count((uint)dds->dma_channel,
                                dma_encode_transfer_count(
                                    DDS_DMA_STREAM_WORDS),
                                true);
}

static void build_pattern(dds_t *dds, const dds_solution_t *solution) {
    memset(dds->pattern, 0, sizeof(dds->pattern));
    uint32_t accumulator = 0u;

    for (uint32_t bit_index = 0; bit_index < DDS_PATTERN_BITS; ++bit_index) {
        accumulator += solution->pattern_ones;
        if (accumulator >= DDS_PATTERN_BITS) {
            accumulator -= DDS_PATTERN_BITS;
            dds->pattern[bit_index / 32u] |= 1u << (bit_index % 32u);
        }
    }

    for (uint32_t i = 0; i < DDS_DMA_STREAM_WORDS; ++i) {
        dds->dma_stream[i] = dds->pattern[i % DDS_PATTERN_WORDS];
    }
}

static bool candidate_is_better(long double abs_error,
                                uint32_t sys_clk_hz,
                                uint32_t divider_q8,
                                uint16_t ones,
                                long double best_abs_error,
                                const dds_solution_t *best) {
    const long double epsilon = 1.0e-12L;
    if (abs_error + epsilon < best_abs_error) {
        return true;
    }
    if (fabsl(abs_error - best_abs_error) > epsilon || best == NULL) {
        return false;
    }
    /* Deterministic tie breakers: current/low clock, short correction density,
     * then the smaller divider. */
    if (sys_clk_hz != best->sys_clk_hz) {
        return sys_clk_hz < best->sys_clk_hz;
    }
    const uint16_t density = ones < DDS_PATTERN_BITS - ones
                                 ? ones
                                 : DDS_PATTERN_BITS - ones;
    const uint16_t best_density =
        best->pattern_ones < DDS_PATTERN_BITS - best->pattern_ones
            ? best->pattern_ones
            : DDS_PATTERN_BITS - best->pattern_ones;
    if (density != best_density) {
        return density < best_density;
    }
    return divider_q8 < best->pio_divider_q8;
}

static void evaluate_clock(uint32_t target_hz,
                           uint32_t sys_clk_hz,
                           uint32_t vco_hz,
                           uint postdiv1,
                           uint postdiv2,
                           long double *best_abs_error,
                           dds_solution_t *best,
                           bool *found) {
    /* achieved = 128*sys*L / (N*(2L+ones)); for a fixed number of
     * ones, the closest N is the rounded ideal integer. */
    const uint64_t numerator =
        (uint64_t)128u * sys_clk_hz * DDS_PATTERN_BITS;

    for (uint32_t ones = 0u; ones <= DDS_PATTERN_BITS; ++ones) {
        const uint32_t half_cycle_sum = 2u * DDS_PATTERN_BITS + ones;
        const uint64_t divisor = (uint64_t)target_hz * half_cycle_sum;
        uint32_t divider_q8 = rounded_div_u64(numerator, divisor);
        if (divider_q8 < DDS_PIO_DIV_Q8_MIN) {
            divider_q8 = DDS_PIO_DIV_Q8_MIN;
        } else if (divider_q8 > DDS_PIO_DIV_Q8_MAX) {
            divider_q8 = DDS_PIO_DIV_Q8_MAX;
        }

        /* For fixed PLL and 'ones', fout is monotonic in N; therefore the
         * rounded ideal N (clamped to hardware limits) is globally optimal. */
        const uint64_t output_denominator =
            (uint64_t)divider_q8 * half_cycle_sum;
        const long double achieved =
            (long double)numerator / (long double)output_denominator;
        const long double error = achieved - (long double)target_hz;
        const long double abs_error = fabsl(error);

        if (!*found || candidate_is_better(
                abs_error, sys_clk_hz, divider_q8, (uint16_t)ones,
                *best_abs_error, *found ? best : NULL)) {
            memset(best, 0, sizeof(*best));
            best->target_hz = target_hz;
            best->sys_clk_hz = sys_clk_hz;
            best->pll_vco_hz = vco_hz;
            best->pll_fbdiv = (uint16_t)(vco_hz / DDS_XOSC_HZ);
            best->pll_postdiv1 = (uint8_t)postdiv1;
            best->pll_postdiv2 = (uint8_t)postdiv2;
            best->pio_divider_q8 = divider_q8;
            best->pio_divider_int = (uint16_t)(divider_q8 >> 8u);
            best->pio_divider_frac = (uint8_t)(divider_q8 & 0xffu);
            best->pattern_bits = DDS_PATTERN_BITS;
            best->pattern_ones = (uint16_t)ones;
            best->base_half_cycles = 2u;
            best->achieved_hz = (double)achieved;
            best->error_hz = (double)error;
            best->error_ppm = (double)(error * 1000000.0L /
                                       (long double)target_hz);
            *best_abs_error = abs_error;
            *found = true;
        }
    }
}

bool dds_init(dds_t *dds, const dds_config_t *config) {
    if (dds == NULL || config == NULL || config->pio == NULL ||
        config->output_gpio >= NUM_BANK0_GPIOS ||
        config->sys_clk_min_hz == 0u ||
        config->sys_clk_min_hz > config->sys_clk_max_hz ||
        !is_power_of_two_u32(DDS_PATTERN_BITS) ||
        !is_power_of_two_u32(sizeof(dds->dma_stream))) {
        return false;
    }

    memset(dds, 0, sizeof(*dds));
    dds->config = *config;

    dds->state_machine = config->state_machine;
    if (dds->state_machine < 0) {
        dds->state_machine = (int)pio_claim_unused_sm(config->pio, true);
    } else {
        pio_sm_claim(config->pio, (uint)dds->state_machine);
    }

    dds->dma_channel = config->dma_channel;
    if (dds->dma_channel < 0) {
        dds->dma_channel = dma_claim_unused_channel(true);
    } else {
        dma_channel_claim((uint)dds->dma_channel);
    }

    if (!pio_can_add_program(config->pio, &dds_square_program)) {
        return false;
    }
    dds->program_offset = pio_add_program(config->pio, &dds_square_program);

    dds_irq_owner = dds;
    irq_set_exclusive_handler(DMA_IRQ_0, dds_dma_irq0);
    irq_set_priority(DMA_IRQ_0, PICO_HIGHEST_IRQ_PRIORITY);
    irq_set_enabled(DMA_IRQ_0, true);
    dma_channel_set_irq0_enabled((uint)dds->dma_channel, true);

    gpio_init(config->output_gpio);
    gpio_set_dir(config->output_gpio, GPIO_OUT);
    gpio_put(config->output_gpio, false);

    dds->initialized = true;
    return true;
}

bool dds_solve(const dds_t *dds, uint32_t target_hz,
               dds_solution_t *solution) {
    if (dds == NULL || !dds->initialized || solution == NULL ||
        target_hz == 0u) {
        return false;
    }

    bool found = false;
    long double best_abs_error = LDBL_MAX;
    dds_solution_t best;
    memset(&best, 0, sizeof(best));

    for (uint32_t fbdiv = 16u; fbdiv <= 320u; ++fbdiv) {
        const uint64_t vco64 = (uint64_t)DDS_XOSC_HZ * fbdiv;
        if (vco64 < DDS_PLL_VCO_MIN_HZ || vco64 > DDS_PLL_VCO_MAX_HZ) {
            continue;
        }
        const uint32_t vco_hz = (uint32_t)vco64;

        for (uint postdiv1 = 1u; postdiv1 <= 7u; ++postdiv1) {
            for (uint postdiv2 = 1u; postdiv2 <= postdiv1; ++postdiv2) {
                const uint32_t post_product = postdiv1 * postdiv2;
                if (vco_hz % post_product != 0u) {
                    continue;
                }
                const uint32_t sys_clk_hz = vco_hz / post_product;
                if (sys_clk_hz < dds->config.sys_clk_min_hz ||
                    sys_clk_hz > dds->config.sys_clk_max_hz) {
                    continue;
                }

                //uint32_t checked_vco = 0u;
                //uint checked_postdiv1 = 0u;
                //uint checked_postdiv2 = 0u;
                
                uint checked_vco = 0u;
                uint checked_postdiv1 = 0u;
                uint checked_postdiv2 = 0u;

                if (!check_sys_clock_hz(sys_clk_hz, &checked_vco,
                                        &checked_postdiv1,
                                        &checked_postdiv2)) {
                    continue;
                }
                /* Keep only the SDK's canonical solution. Besides validation,
                 * this removes duplicate PLL representations of one clock. */
                if (checked_vco != vco_hz || checked_postdiv1 != postdiv1 ||
                    checked_postdiv2 != postdiv2) {
                    continue;
                }

                evaluate_clock(target_hz, sys_clk_hz, vco_hz,
                               postdiv1, postdiv2, &best_abs_error, &best,
                               &found);
            }
        }
    }

    if (!found || !isfinite(best.achieved_hz) ||
        !isfinite(best.error_hz)) {
        return false;
    }
    *solution = best;
    return true;
}

static bool apply_system_clock(const dds_solution_t *solution) {
    if (clock_get_hz(clk_sys) == solution->sys_clk_hz) {
        return true;
    }

    /* The tested board was stable at 250 MHz. Raising VREG before clocks above
     * 200 MHz follows the conservative overclock sequence. */
    if (solution->sys_clk_hz > 200000000u) {
        vreg_set_voltage(VREG_VOLTAGE_1_20);
        sleep_ms(10);
    }

    set_sys_clock_pll(solution->pll_vco_hz,
                      solution->pll_postdiv1,
                      solution->pll_postdiv2);
    return clock_get_hz(clk_sys) == solution->sys_clk_hz;
}

bool dds_start(dds_t *dds, const dds_solution_t *solution) {
    if (dds == NULL || !dds->initialized || solution == NULL ||
        solution->pattern_bits != DDS_PATTERN_BITS ||
        solution->pio_divider_q8 < DDS_PIO_DIV_Q8_MIN) {
        return false;
    }

    dds_stop(dds);
    if (!apply_system_clock(solution)) {
        return false;
    }

    build_pattern(dds, solution);

    const PIO pio = dds->config.pio;
    const uint sm = (uint)dds->state_machine;
    pio_sm_config sm_config =
        dds_square_program_get_default_config(dds->program_offset);
    sm_config_set_sideset_pins(&sm_config, dds->config.output_gpio);
    sm_config_set_out_shift(&sm_config, true, true, 32u);
    sm_config_set_fifo_join(&sm_config, PIO_FIFO_JOIN_TX);
    sm_config_set_clkdiv_int_frac8(&sm_config,
                                   solution->pio_divider_int,
                                   solution->pio_divider_frac);

    pio_gpio_init(pio, dds->config.output_gpio);
    pio_sm_set_consecutive_pindirs(pio, sm, dds->config.output_gpio, 1u, true);
    pio_sm_init(pio, sm, dds->program_offset, &sm_config);
    pio_sm_clear_fifos(pio, sm);
    pio_sm_restart(pio, sm);

    dma_channel_config dma_config =
        dma_channel_get_default_config((uint)dds->dma_channel);
    channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_32);
    channel_config_set_read_increment(&dma_config, true);
    channel_config_set_write_increment(&dma_config, false);
    channel_config_set_dreq(&dma_config, pio_get_dreq(pio, sm, true));
    channel_config_set_ring(&dma_config, true, 13u); /* 8192-byte stream. */
    channel_config_set_high_priority(&dma_config, true);

    dma_channel_configure((uint)dds->dma_channel, &dma_config,
                          &pio->txf[sm], dds->dma_stream,
                          dma_encode_transfer_count(DDS_DMA_STREAM_WORDS),
                          false);

    dds->active_solution = *solution;
    dds->running = true;
    dma_start_channel_mask(1u << (uint)dds->dma_channel);
    pio_sm_set_enabled(pio, sm, true);
    return true;
}

void dds_stop(dds_t *dds) {
    if (dds == NULL || !dds->initialized) {
        return;
    }

    dds->running = false;
    dma_channel_abort((uint)dds->dma_channel);
    dma_hw->ints0 = 1u << (uint)dds->dma_channel;
    pio_sm_set_enabled(dds->config.pio, (uint)dds->state_machine, false);
    pio_sm_clear_fifos(dds->config.pio, (uint)dds->state_machine);
    gpio_set_function(dds->config.output_gpio, GPIO_FUNC_SIO);
    gpio_set_dir(dds->config.output_gpio, GPIO_OUT);
    gpio_put(dds->config.output_gpio, false);
}

dds_setfreq_result_t dds_setfreq(dds_t *dds, uint32_t target_hz,
                                 dds_solution_t *solution_out) {
    dds_solution_t solution;
    if (!dds_solve(dds, target_hz, &solution)) {
        return DDS_SETFREQ_FAILED;
    }
    if (solution_out != NULL) {
        *solution_out = solution;
    }

/*
 * dds_start() detiene PIO/DMA, cambia PLL_SYS si es necesario,
 * carga el nuevo patrón y vuelve a iniciar la generación.
 *
 * PLL_USB permanece funcionando a 48 MHz, por lo que no se
 * reinicia TinyUSB ni el RP2040.
 */
    return dds_start(dds, &solution)
             ? DDS_SETFREQ_APPLIED
             : DDS_SETFREQ_FAILED;


}

bool dds_take_pending_frequency(uint32_t *target_hz) {
    if (target_hz == NULL || watchdog_hw->scratch[6] != DDS_PENDING_MAGIC) {
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

const char *dds_setfreq_result_name(dds_setfreq_result_t result) {
    switch (result) {
        case DDS_SETFREQ_APPLIED:
            return "aplicada";
        case DDS_SETFREQ_REBOOT_REQUIRED:
            return "requiere reinicio";
        case DDS_SETFREQ_REBOOTING:
            return "reiniciando";
        case DDS_SETFREQ_FAILED:
        default:
            return "error";
    }
}
