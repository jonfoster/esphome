#ifdef USE_ESP32

#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "preferences.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_idf_version.h>
#include <esp_task_wdt.h>
#include <esp_timer.h>
#include <soc/rtc.h>

#include <hal/cpu_hal.h>

#ifdef USE_ARDUINO
#include <Esp.h>
#else
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 0)
#include <esp_clk_tree.h>
#endif
void setup();
void loop();
#endif

namespace esphome {

void IRAM_ATTR HOT yield() { vPortYield(); }
uint32_t IRAM_ATTR HOT millis() { return (uint32_t) (esp_timer_get_time() / 1000ULL); }

// Delay at least the specified number of milliseconds.
// (May delay a bit longer, but will never delay for a shorter time).
//
// Implementation Note:
//     vTaskDelay(N) waits for the clock to tick over N times.  This is
//     SUBTLY DIFFERENT from waiting for (N * portTICK_PERIOD_MS) milliseconds.
//     The delay may be 1 tick shorter than that, because the clock
//     may happen to tick over immediately after the wait starts.
//
//     That is, vTaskDelay(N) actually waits for between
//     ((N-1) * portTICK_PERIOD_MS) milliseconds
//     and (N * portTICK_PERIOD_MS) milliseconds.
//     (Plus however many microseconds are needed for the function call and
//     return, plus possible further delays due to other tasks running on
//     the processor).
//
//     For example, if the clock ticks every 1ms, then vTaskDelay(1) may
//     return immediately, without waiting the 1ms as the caller wanted.
//     The delay will be 0ms to 1ms.
//
//     Another example, if the clock ticks every 1ms, then vTaskDelay(10) may
//     return after just 9ms.  The delay will be 9ms to 10ms.
//
//     For delay(), that's not what the caller wants, the caller wants to wait
//     *at least* the specified time.  (For example, waiting for some hardware
//     to initialise when we know how long that takes).
//
//     So we add 1 tick to the wait time before we pass it to vTaskDelay().
//
//     Also, if the clock does not tick every millisecond, then we ensure we
//     round UP to the next number of whole clock ticks.
//
//     For example, if the clock ticks every 10ms, then delay(1) will block
//     for 10ms to 20ms.
//
void IRAM_ATTR HOT delay(uint32_t ms) {
  vTaskDelay((ms + ((uint32_t) portTICK_PERIOD_MS - 1u)) / (uint32_t) portTICK_PERIOD_MS + 1u);
}

uint32_t IRAM_ATTR HOT micros() { return (uint32_t) esp_timer_get_time(); }
void IRAM_ATTR HOT delayMicroseconds(uint32_t us) { delay_microseconds_safe(us); }
void arch_restart() {
  esp_restart();
  // restart() doesn't always end execution
  while (true) {  // NOLINT(clang-diagnostic-unreachable-code)
    yield();
  }
}

void arch_init() {
  // Enable the task watchdog only on the loop task (from which we're currently running)
#if defined(USE_ESP_IDF)
  esp_task_wdt_add(nullptr);
  // Idle task watchdog is disabled on ESP-IDF
#elif defined(USE_ARDUINO)
  enableLoopWDT();
  // Disable idle task watchdog on the core we're using (Arduino pins the task to a core)
#if defined(CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0) && CONFIG_ARDUINO_RUNNING_CORE == 0
  disableCore0WDT();
#endif
#if defined(CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU1) && CONFIG_ARDUINO_RUNNING_CORE == 1
  disableCore1WDT();
#endif
#endif
}
void IRAM_ATTR HOT arch_feed_wdt() { esp_task_wdt_reset(); }

uint8_t progmem_read_byte(const uint8_t *addr) { return *addr; }
uint32_t arch_get_cpu_cycle_count() { return esp_cpu_get_cycle_count(); }
uint32_t arch_get_cpu_freq_hz() {
  uint32_t freq = 0;
#ifdef USE_ESP_IDF
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 0)
  esp_clk_tree_src_get_freq_hz(SOC_MOD_CLK_CPU, ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, &freq);
#else
  rtc_cpu_freq_config_t config;
  rtc_clk_cpu_freq_get_config(&config);
  freq = config.freq_mhz * 1000000U;
#endif
#elif defined(USE_ARDUINO)
  freq = ESP.getCpuFreqMHz() * 1000000;
#endif
  return freq;
}

#ifdef USE_ESP_IDF
TaskHandle_t loop_task_handle = nullptr;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

void loop_task(void *pv_params) {
  setup();
  while (true) {
    loop();
  }
}

extern "C" void app_main() {
  esp32::setup_preferences();
  xTaskCreate(loop_task, "loopTask", 8192, nullptr, 1, &loop_task_handle);
}
#endif  // USE_ESP_IDF

#ifdef USE_ARDUINO
extern "C" void init() { esp32::setup_preferences(); }
#endif  // USE_ARDUINO

}  // namespace esphome

#endif  // USE_ESP32
