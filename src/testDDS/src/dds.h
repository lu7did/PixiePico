#ifndef PIXIEPICO_DDS_H
#define PIXIEPICO_DDS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "hardware/pio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Defaults are deliberately centralized so they can be changed later. */
#ifndef DDS_SYS_CLK_MIN_HZ
#define DDS_SYS_CLK_MIN_HZ 125000000u
#endif

#ifndef DDS_SYS_CLK_MAX_HZ
#define DDS_SYS_CLK_MAX_HZ 250000000u
#endif

#ifndef DDS_DEFAULT_GPIO
#define DDS_DEFAULT_GPIO 14u
#endif

/* Solver resolution: one 1024-bit Bresenham pattern, repeated in the DMA ring. */
#define DDS_PATTERN_BITS 1024u
#define DDS_PATTERN_WORDS (DDS_PATTERN_BITS / 32u)
#define DDS_DMA_STREAM_BITS 65536u
#define DDS_DMA_STREAM_WORDS (DDS_DMA_STREAM_BITS / 32u)

#define DDS_PENDING_MAGIC 0x44445331u

typedef enum {
    DDS_SETFREQ_FAILED = 0,
    DDS_SETFREQ_APPLIED,
    DDS_SETFREQ_REBOOT_REQUIRED,
    DDS_SETFREQ_REBOOTING
} dds_setfreq_result_t;

typedef struct {
    PIO pio;
    int state_machine;       /* -1: claim an unused SM. */
    int dma_channel;         /* -1: claim an unused DMA channel. */
    uint output_gpio;
    uint32_t sys_clk_min_hz;
    uint32_t sys_clk_max_hz;
    bool reboot_on_pll_change;
} dds_config_t;

typedef struct {
    uint32_t target_hz;

    uint32_t sys_clk_hz;
    uint32_t pll_vco_hz;
    uint16_t pll_fbdiv;
    uint8_t pll_postdiv1;
    uint8_t pll_postdiv2;

    /* PIO CLKDIV = pio_divider_q8 / 256 (hardware format 16.8). */
    uint32_t pio_divider_q8;
    uint16_t pio_divider_int;
    uint8_t pio_divider_frac;

    /* Each half-period is 2 cycles plus one optional Bresenham cycle. */
    uint16_t pattern_bits;
    uint16_t pattern_ones;
    uint8_t base_half_cycles;

    double achieved_hz;
    double error_hz;
    double error_ppm;
} dds_solution_t;

typedef struct {
    dds_config_t config;
    dds_solution_t active_solution;
    uint program_offset;
    int state_machine;
    int dma_channel;
    bool initialized;
    bool running;
    uint32_t pattern[DDS_PATTERN_WORDS];
    uint32_t dma_stream[DDS_DMA_STREAM_WORDS] __attribute__((aligned(8192)));
} dds_t;

/* Initialize resources, but do not change PLL_SYS or start the output. */
bool dds_init(dds_t *dds, const dds_config_t *config);

/* Exhaustive global solver over every PLL_SYS accepted in the configured range. */
bool dds_solve(const dds_t *dds, uint32_t target_hz,
               dds_solution_t *solution);

/* Apply the solution and start the output. Call before stdio_init_all/TinyUSB
 * when the solution requires a different PLL_SYS. */
bool dds_start(dds_t *dds, const dds_solution_t *solution);

/* Stop DMA/PIO and force the output low. */
void dds_stop(dds_t *dds);

/* Solve and apply a new frequency. If the globally best solution needs a new
 * PLL after USB is active, return REBOOT_REQUIRED or store the request and
 * perform a watchdog reboot, according to reboot_on_pll_change. */
dds_setfreq_result_t dds_setfreq(dds_t *dds, uint32_t target_hz,
                                 dds_solution_t *solution_out);

/* Recover a frequency stored by dds_setfreq() across a watchdog reboot. */
bool dds_take_pending_frequency(uint32_t *target_hz);

const char *dds_setfreq_result_name(dds_setfreq_result_t result);

#ifdef __cplusplus
}
#endif

#endif

