/**
 * @file bench_port_esp32.c
 * @brief ESP32 implementation of the portable benchmark HAL.
 *
 * @details
 * Provides the concrete platform bindings for ESP32/ESP-IDF. This file
 * implements all functions declared in bench_port.h: high-resolution
 * timing, CPU cycle counting, stack watermarking, isolated task creation,
 * and session metadata population.
 *
 * Timing sources:
 *   - Wall-clock: esp_timer_get_time() (microseconds, monotonic)
 *   - Cycles:     esp_cpu_get_cycle_count() (CPU cycles)
 *
 * Thread management uses FreeRTOS with xTaskCreatePinnedToCore() and
 * semaphore-based completion signaling.
 *
 * Metadata extraction uses ESP-IDF's esp_chip_info(), esp_get_idf_version(),
 * and esp_flash_get_chip_size().
 *
 * @author Nirapod Team
 * @date 2026
 * @version 0.1.0
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 *
 * @ingroup bench_port
 * @{
 */

#include "../include/bench_port.h"

#include "esp_chip_info.h"
#include "esp_cpu.h"
#include "esp_flash.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_timer.h"
#if CONFIG_PM_ENABLE
#include "esp_pm.h"
#endif
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include <string.h>

/* Static objects for heap-free isolated task execution */
static SemaphoreHandle_t s_done = NULL;
static StaticSemaphore_t s_sem_buf;
static StaticTask_t s_task_buf;
static StackType_t s_stack_buf[(BIP39_BENCH_TASK_STACK_BYTES + sizeof(StackType_t) - 1) / sizeof(StackType_t)];

/* ── Local helpers ── */

static const char *bip39_bench_chip_model_name(esp_chip_model_t model)
{
    NIRAPOD_ASSERT((int)model >= 0);
    NIRAPOD_ASSERT(sizeof(model) <= 4U);

    switch (model) {
    case CHIP_ESP32:    return "ESP32";
    case CHIP_ESP32S2:  return "ESP32-S2";
    case CHIP_ESP32S3:  return "ESP32-S3";
    case CHIP_ESP32C3:  return "ESP32-C3";
    case CHIP_ESP32C2:  return "ESP32-C2";
    case CHIP_ESP32C6:  return "ESP32-C6";
    case CHIP_ESP32H2:  return "ESP32-H2";
    default:            return "unknown";
    }
}

static uint32_t bip39_bench_flash_size_bytes(void)
{
    uint32_t flash_size = 0U;
    if (esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        return 0U;
    }
    return flash_size;
}

/* ── Timer operations ── */

/**
 * @brief Return current wall-clock time in microseconds since boot.
 * Implements bench_port_now_us().
 *
 * @return Microseconds since boot.
 *
 * @pre ESP-IDF timer system must be initialized (happens before app_main).
 *
 * @post Returned value is monotonically increasing.
 */
uint64_t bench_port_now_us(void)
{
    uint64_t t = (uint64_t)esp_timer_get_time();
    NIRAPOD_ASSERT(sizeof(t) == sizeof(uint64_t));
    NIRAPOD_ASSERT(t <= UINT64_MAX / 2); /* Sanity: not overflowed */
    return t;
}

uint32_t bench_port_cycle_count(void)
{
    uint32_t cycles = esp_cpu_get_cycle_count();
    NIRAPOD_ASSERT(sizeof(cycles) <= UINT32_MAX);  /* size_t range check before sizeof comparison */
    NIRAPOD_ASSERT(sizeof(cycles) == sizeof(uint32_t));
    NIRAPOD_ASSERT(cycles <= UINT32_MAX); /* Always true, but keep assertion */
    return cycles;
}

/* ── Stack watermark ── */

/**
 * @brief Return the minimum free stack bytes of the current FreeRTOS task.
 * Implements bench_port_get_stack_watermark().
 *
 * @return Number of free bytes remaining in the task's stack at its peak usage.
 * @retval 0 if the task's stack has overflowed or the call fails.
 *
 * @pre Must be called from within a FreeRTOS task context.
 */
uint32_t bench_port_get_stack_watermark(void)
{
    NIRAPOD_ASSERT(xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED);

    UBaseType_t free_words = uxTaskGetStackHighWaterMark(NULL);
    size_t free_bytes = (size_t)free_words * sizeof(StackType_t);

    NIRAPOD_ASSERT(free_words <= UINT32_MAX / sizeof(StackType_t));  /* No overflow */
    NIRAPOD_ASSERT(free_bytes <= BIP39_BENCH_TASK_STACK_BYTES);
    NIRAPOD_ASSERT(free_bytes <= UINT32_MAX);

    return (uint32_t)free_bytes;
}

/* ── Metadata population ── */

/**
 * @brief Fill benchmark metadata with ESP32-specific platform information.
 * Implements bench_port_fill_metadata().
 *
 * Populates all fields of the metadata structure including:
 *  - Chip model and revision from esp_chip_info()
 *  - CPU core count
 *  - Flash size via esp_flash_get_chip_size()
 *  - SDK version via esp_get_idf_version()
 *  - Timings and stack configuration passed through verbatim
 *
 * @param[out] metadata               Destination structure
 * @param[in]  backend                Backend name string (compiler-injected)
 * @param[in]  git_sha                12-char Git SHA (compiler-injected)
 * @param[in]  build_profile          Build profile label
 * @param[in]  task_core              Core ID used for measurement tasks
 * @param[in]  task_stack_bytes       Stack bytes allocated per task
 * @param[in]  timer_overhead_cycles  Calibrated cycle overhead
 * @param[in]  timer_overhead_wall_us Calibrated wall-clock overhead
 */
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

    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    metadata->schema_version = "1";
    metadata->platform = "esp32";
    metadata->backend = backend;
    metadata->git_sha = git_sha;
    metadata->sdk_version = esp_get_idf_version();
    NIRAPOD_ASSERT(metadata->sdk_version != NULL);
    metadata->chip_model = bip39_bench_chip_model_name(chip_info.model);
    metadata->chip_revision = chip_info.revision;
    NIRAPOD_ASSERT(chip_info.cores <= UINT8_MAX);
    metadata->chip_cores = (uint8_t)chip_info.cores;
    NIRAPOD_ASSERT(chip_info.cores >= 1U);
    metadata->cpu_freq_mhz = 240U;
    metadata->flash_size_bytes = bip39_bench_flash_size_bytes();
    NIRAPOD_ASSERT(metadata->flash_size_bytes > 0U);
    metadata->build_profile = build_profile;
    metadata->task_core = task_core;
    metadata->task_stack_bytes = task_stack_bytes;
    metadata->timer_overhead_cycles = timer_overhead_cycles;
    metadata->timer_overhead_wall_us = timer_overhead_wall_us;
}

/* ── Isolated task runner ── */

/**
 * @brief Context block passed to the FreeRTOS wrapper task.
 */
typedef struct {
    void (*fn)(void *);       ///< User function to execute inside isolated task
    void *arg;                ///< Argument passed to @p fn
    SemaphoreHandle_t done;   ///< Binary semaphore used for completion signaling
} task_ctx_t;

/**
 * @brief FreeRTOS task entrypoint that runs the user function and signals.
 *
 * @param arg  Pointer to task_ctx_t (fn, arg, done_sem).
 *
 * @post The semaphore is given, unblocking the creator.
 * @post The task deletes itself before return.
 */
static void bench_port_task_wrapper(void *arg)
{
    NIRAPOD_ASSERT(arg != NULL);
    task_ctx_t *ctx = (task_ctx_t *)arg;
    NIRAPOD_ASSERT(ctx->fn != NULL);
    NIRAPOD_ASSERT(ctx->done != NULL);

    ctx->fn(ctx->arg);
    xSemaphoreGive(ctx->done);
    vTaskDelete(NULL);
}

/**
 * @brief Create and execute a function in an isolated FreeRTOS task.
 * Implements bench_port_run_isolated_task().
 *
 * Uses static allocation for task control block, stack, and semaphore
 * to satisfy NASA Rule 3 (no dynamic allocation after init).
 *
 * @param fn          Function to execute in the isolated context.
 * @param arg         Argument passed to @p fn.
 * @param stack_bytes Stack allocation in bytes (rounded up to StackType_t words).
 * @param task_core   Core pinning (0 or 1); ignored if core not available.
 *
 * @post If creation succeeds, the task runs to completion before return.
 * @post All allocated resources (task control block, semaphore, memory) are freed.
 *
 * @pre @p fn is a valid function pointer.
 * @pre @p stack_bytes is within the FreeRTOS configurable limit (typically ≤ 32 KiB).
 *
 * @note On allocation failure or task creation error, the function returns
 *       silently: the benchmark suite will be skipped without crashing.
 */
void bench_port_run_isolated_task(void (*fn)(void *arg), void *arg,
                                  uint32_t stack_bytes, int task_core)
{
    NIRAPOD_ASSERT(fn != NULL);
    NIRAPOD_ASSERT(stack_bytes >= 1024U);  /* Sanity: must be at least 1 KiB */
    NIRAPOD_ASSERT(stack_bytes <= 32U * 1024U); /* FreeRTOS typical limit */

    /* Initialize static semaphore once */
    if (s_done == NULL) {
        s_done = xSemaphoreCreateBinaryStatic(&s_sem_buf);
        NIRAPOD_ASSERT(s_done != NULL);
        if (s_done == NULL) return;
    }

    /* Prepare context on stack (no heap) */
    task_ctx_t ctx;
    ctx.fn = fn;
    ctx.arg = arg;
    ctx.done = s_done;

    /* Compute stack depth in words (rounded up) */
    uint32_t stack_words = (stack_bytes + sizeof(StackType_t) - 1) / sizeof(StackType_t);
    size_t max_stack_words = sizeof(s_stack_buf) / sizeof(StackType_t);
    BaseType_t core_id = (task_core < 0) ? tskNO_AFFINITY : (BaseType_t)task_core;
    NIRAPOD_ASSERT(stack_words > 0);
    NIRAPOD_ASSERT(max_stack_words <= UINT32_MAX);
    NIRAPOD_ASSERT(stack_words <= (uint32_t)max_stack_words);
    NIRAPOD_ASSERT((task_core < 0) || (taskVALID_CORE_ID(core_id) == pdTRUE));

    TaskHandle_t worker = xTaskCreateStaticPinnedToCore(
        bench_port_task_wrapper,
        "bench_worker",
        stack_words,
        &ctx,
        tskIDLE_PRIORITY + 2,
        s_stack_buf,
        &s_task_buf,
        core_id
    );
    NIRAPOD_ASSERT(worker != NULL);
    if (worker == NULL) {
        return;
    }

    xSemaphoreTake(s_done, portMAX_DELAY);
    /* Task has deleted itself; static resources remain for next call */
}

/* ── Optional platform setup (called from app_main) ── */

/**
 * @brief Enter benchmark mode: lock CPU frequency and disable light sleep.
 *
 * Acquires ESP-IDF power-management locks to keep the CPU at maximum
 * frequency and prevent light sleep entry for the duration of the session.
 * Log level is reduced to errors only to reduce UART interference.
 *
 * Safe to call multiple times; locks are created on first use.
 *
 * @post CPU frequency is locked at maximum; light sleep is disabled.
 * @post Log level for all tags is set to ESP_LOG_ERROR.
 */
void bip39_bench_platform_enter(void)
{
    NIRAPOD_ASSERT(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING);
    esp_log_level_set("*", ESP_LOG_ERROR);
    NIRAPOD_ASSERT(esp_log_level_get("*") == ESP_LOG_ERROR);

#if CONFIG_PM_ENABLE
    static esp_pm_lock_handle_t s_cpu_lock = NULL;
    static esp_pm_lock_handle_t s_sleep_lock = NULL;

    if (s_cpu_lock == NULL) {
        ESP_ERROR_CHECK(esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX, 0, "bip39_bench_cpu", &s_cpu_lock));
        NIRAPOD_ASSERT(s_cpu_lock != NULL);
    }
    if (s_sleep_lock == NULL) {
        ESP_ERROR_CHECK(esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, "bip39_bench_sleep", &s_sleep_lock));
        NIRAPOD_ASSERT(s_sleep_lock != NULL);
    }
    ESP_ERROR_CHECK(esp_pm_lock_acquire(s_cpu_lock));
    ESP_ERROR_CHECK(esp_pm_lock_acquire(s_sleep_lock));
    NIRAPOD_ASSERT(esp_pm_lock_acquired(s_cpu_lock));
    NIRAPOD_ASSERT(esp_pm_lock_acquired(s_sleep_lock));
#endif
}

/**
 * @brief Leave benchmark mode: release power-management locks.
 *
 * Releases the frequency and light-sleep locks acquired by
 * bip39_bench_platform_enter(). Does nothing if PM is disabled or
 * locks were not acquired.
 *
 * @post If PM is enabled, all locks are released.
 */
void bip39_bench_platform_leave(void)
{
#if CONFIG_PM_ENABLE
    extern esp_pm_lock_handle_t s_sleep_lock;
    extern esp_pm_lock_handle_t s_cpu_lock;
    if (s_sleep_lock) {
        NIRAPOD_ASSERT(esp_pm_lock_acquired(s_sleep_lock));
        esp_pm_lock_release(s_sleep_lock);
    }
    if (s_cpu_lock) {
        NIRAPOD_ASSERT(esp_pm_lock_acquired(s_cpu_lock));
        esp_pm_lock_release(s_cpu_lock);
    }
#endif
}

/** @} */  // end group bench_port
