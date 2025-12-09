#pragma once

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"

#ifdef USE_ESP32
#include <esp_sleep.h>
#endif

#ifdef USE_TIME
#include "esphome/components/time/real_time_clock.h"
#include "esphome/core/time.h"
#endif

#include <cinttypes>

namespace esphome {
namespace deep_sleep {

#ifdef USE_ESP32
enum WakeupPinMode {
  WAKEUP_PIN_MODE_IGNORE = 0,
  WAKEUP_PIN_MODE_KEEP_AWAKE,
  WAKEUP_PIN_MODE_INVERT_WAKEUP,
};

#if !defined(USE_ESP32_VARIANT_ESP32C2) && !defined(USE_ESP32_VARIANT_ESP32C3)
struct Ext1Wakeup {
  uint64_t mask;
  esp_sleep_ext1_wakeup_mode_t wakeup_mode;
};
#endif

struct WakeupCauseToRunDuration {
  uint32_t default_cause;
  uint32_t touch_cause;
  uint32_t gpio_cause;
};
#endif  // USE_ESP32

#ifdef USE_LIBRETINY
enum LibretinyWakeLevel {
  LIBRETINY_WAKE_ON_LOW = 0,
  LIBRETINY_WAKE_ON_HIGH = 1,
};
#endif

class DeepSleepComponent : public Component {
 public:
  void set_sleep_duration(uint32_t time_ms);

#ifdef USE_ESP32
  void set_wakeup_pin(InternalGPIOPin *pin) { this->wakeup_pin_ = pin; }
  void set_wakeup_pin_mode(WakeupPinMode wakeup_pin_mode);
#if !defined(USE_ESP32_VARIANT_ESP32C2) && !defined(USE_ESP32_VARIANT_ESP32C3)
  void set_ext1_wakeup(Ext1Wakeup ext1_wakeup);
#endif
#if !defined(USE_ESP32_VARIANT_ESP32C2) && !defined(USE_ESP32_VARIANT_ESP32C3) && \
    !defined(USE_ESP32_VARIANT_ESP32C6) && !defined(USE_ESP32_VARIANT_ESP32C61) && !defined(USE_ESP32_VARIANT_ESP32H2)
  void set_touch_wakeup(bool touch_wakeup);
#endif
  void set_run_duration(WakeupCauseToRunDuration wakeup_cause_to_run_duration);
#endif  // USE_ESP32

#ifdef USE_LIBRETINY
  void set_wakeup_pin(InternalGPIOPin *pin) { this->libretiny_wakeup_pin_ = pin; }
  void set_libretiny_wake_level(LibretinyWakeLevel level) { this->libretiny_wake_level_ = level; }
#endif

  void set_run_duration(uint32_t time_ms);

  void setup() override;
  void dump_config() override;
  void loop() override;
  float get_loop_priority() const override;
  float get_setup_priority() const override;

  void begin_sleep(bool manual = false);
  void prevent_deep_sleep();
  void allow_deep_sleep();

 protected:
  optional<uint32_t> get_run_duration_() const;
  void dump_config_platform_();
  bool prepare_to_sleep_();
  void deep_sleep_();

  optional<uint64_t> sleep_duration_;
#ifdef USE_ESP32
  InternalGPIOPin *wakeup_pin_{nullptr};
  WakeupPinMode wakeup_pin_mode_{WAKEUP_PIN_MODE_IGNORE};
  optional<Ext1Wakeup> ext1_wakeup_;
  optional<bool> touch_wakeup_;
  optional<WakeupCauseToRunDuration> wakeup_cause_to_run_duration_;
#endif

#ifdef USE_LIBRETINY
  InternalGPIOPin *libretiny_wakeup_pin_{nullptr};
  LibretinyWakeLevel libretiny_wake_level_{LIBRETINY_WAKE_ON_HIGH};
#endif

  optional<uint32_t> run_duration_;
  bool next_enter_deep_sleep_{false};
  bool prevent_{false};
};

extern bool global_has_deep_sleep;

}  // namespace deep_sleep
}  // namespace esphome
