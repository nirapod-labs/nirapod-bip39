/**
 * @file bench_port_nrf52.c
 * @brief Zephyr (nRF52) implementation of the portable benchmark HAL.
 *
 * @details
 * Provides the concrete platform bindings for nRF52/Zephyr. Implements
 * timing, stack watermark, metadata, and isolated thread execution.
 *
 * @author Nirapod Team
 * @date 2026
 * @version 0.1.0
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 *
 * @ingroup bench_port
 */

#include "../include/bench_port.h"

#include <kernel.h>
#include <sys/clock.h>
#include <string.h>

/* Static stack for isolated task to avoid dynamic allocation */
static K_THREAD_STACK_DEFINE(s_thread_stack, BIP39_BENCH_TASK_STACK_BYTES);
static struct k_thread s_thread;

uint64_t bench_port_now_us(void)
{
    /* k_uptime_get() returns nanoseconds */
    uint64_t us = k_uptime_get() / 1000ULL;
    NIRAPOD_ASSERT(us >= 0);  /* Unsigned always non-negative */
    NIRAPOD_ASSERT(us <= UINT64_MAX / 2); /* Sanity: not overflowed */
    return us;
}

uint32_t bench_port_cycle_count(void)
{
    uint32_t cycles = k_cycle_get_32();
    NIRAPOD_ASSERT(cycles <= UINT32_MAX);
    NIRAPOD_ASSERT(cycles >= 0);  /* Wrap is okay, just not negative */
    return cycles;
}

uint32_t bench_port_get_stack_watermark(void)
{
    uint32_t free_bytes = k_thread_stack_space_get(NULL);
    /* Sanity: free_bytes cannot exceed typical stack size and must be non-zero if stack exists */
    NIRAPOD_ASSERT(free_bytes <= BIP39_BENCH_TASK_STACK_BYTES);
    NIRAPOD_ASSERT(free_bytes <= 16384U); /* Upper bound based on config */
    return free_bytes;
}

void bench_port_fill_metadata(bip39_bench_metadata *metadata,
                              const char *backend,
                              const char *git_sha,
                              const char *build_profile,
                              uint8_t task_core,
                              uint32_t task_stack_bytes,
                              uint32_t timer_overhead_cycles,
                              uint32_t timer_overhead_wall_us)
{
    NIRAPOD_ASSERT(metadata != NULL);
    NIRAPOD_ASSERT(backend != NULL);
    NIRAPOD_ASSERT(git_sha != NULL);
    NIRAPOD_ASSERT(build_profile != NULL);

    metadata->schema_version = "1";
    metadata->platform = "nrf52";
    metadata->backend = backend;
    metadata->git_sha = git_sha;
    metadata->sdk_version = KERNEL_VERSION_STRING;
    NIRAPOD_ASSERT(metadata->sdk_version != NULL);
    metadata->chip_model = "nrf52840"; /* TODO: detect from DT if possible */
    metadata->chip_revision = 0U;      /* Not readily available on Zephyr */
    metadata->chip_cores = 1U;
    uint32_t hw_cycles_per_sec = sys_clock_hw_cycles_per_sec();
    NIRAPOD_ASSERT(hw_cycles_per_sec > 0U);
    metadata->cpu_freq_mhz = hw_cycles_per_sec / 1000000U;
    metadata->flash_size_bytes = 512U * 1024U; /* TODO: derive from device description */
    metadata->build_profile = build_profile;
    metadata->task_core = task_core;
    metadata->task_stack_bytes = task_stack_bytes;
    metadata->timer_overhead_cycles = timer_overhead_cycles;
    metadata->timer_overhead_wall_us = timer_overhead_wall_us;
}

void bench_port_run_isolated_task(void (*fn)(void *arg), void *arg,
                                  uint32_t stack_bytes, int task_core)
{
    NIRAPOD_ASSERT(fn != NULL);
    NIRAPOD_ASSERT(stack_bytes >= 1024U);  /* Min stack sanity */
    NIRAPOD_ASSERT(stack_bytes <= 32U * 1024U); /* Upper bound */

    unsigned int options = 0;
#if defined(CONFIG_CPU_AFFINITY) && (CONFIG_CPU_AFFINITY != 0)
    if (task_core >= 0) {
        options = K_AFFINITY_CPU((k_cpu_id_t)task_core);
    }
#endif

    /* Use static stack buffer to avoid heap allocation */
    struct k_thread thread;
    k_thread_stack_t *stack = s_thread_stack;

    struct k_thread *created = k_thread_create(&thread, stack, BIP39_BENCH_TASK_STACK_BYTES,
                                               (k_thread_entry_t)fn, arg,
                                               NULL, NULL, NULL,
                                               options, 0, 0);
    NIRAPOD_ASSERT(created != NULL);
    if (!created) return;

    /* Priority: 0 = highest for application threads */
    k_thread_priority_set(&thread, 0);
    k_thread_start(&thread);
    k_thread_join(&thread, K_FOREVER);
    /* Thread detached after join; static stack remains */
}
