#pragma once

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"

#ifdef USE_ESP32
#include <esp_sleep.h>
#endif

#include <cinttypes>
#include <vector>

namespace esphome {
namespace deep_sleep {

#ifdef USE_ESP32
enum WakeupPinMode {
  WAKEUP_PIN_MODE_IGNORE = 0,
  WAKEUP_PIN_MODE_KEEP_AWAKE,
  WAKEUP_PIN_MODE_INVERT_WAKEUP,
};
#endif

#ifdef USE_LIBRETINY
struct LibretinyWakePin {
  InternalGPIOPin *pin;
  WakeupPinMode mode;
};
#endif

class DeepSleepComponent : public Component {
 public:
  void set_sleep_duration(uint32_t time_ms);
  void set_run_duration(uint32_t time_ms);

#ifdef USE_ESP32
  void set_wakeup_pin(InternalGPIOPin *pin) { this->wakeup_pin_ = pin; }
  void set_wakeup_pin_mode(WakeupPinMode mode) { this->wakeup_pin_mode_ = mode; }
#endif

#ifdef USE_LIBRETINY
  void add_wakeup_pin(InternalGPIOPin *pin, WakeupPinMode mode);
#endif

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
  optional<uint32_t> run_duration_;
  bool next_enter_deep_sleep_{false};
  bool prevent_{false};

#ifdef USE_ESP32
  InternalGPIOPin *wakeup_pin_{nullptr};
  WakeupPinMode wakeup_pin_mode_{WAKEUP_PIN_MODE_IGNORE};
#endif

#ifdef USE_LIBRETINY
  std::vector<LibretinyWakePin> libretiny_wakeup_pins_;
#endif
};

extern bool global_has_deep_sleep;

}  // namespace deep_sleep
}  // namespace esphome
