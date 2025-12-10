#pragma once

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"

#ifdef USE_ESP32
#include <esp_sleep.h>
#endif

#ifdef USE_ESP8266
#include <Esp.h>
#endif

#ifdef USE_LIBRETINY
#include <lt_sleep.h>
#endif

namespace esphome {
namespace deep_sleep {

class DeepSleepComponent : public Component {
 public:
  void set_sleep_duration(uint32_t time_ms);
  void set_wakeup_pin(InternalGPIOPin *pin) { this->wakeup_pin_ = pin; }

  void setup() override;
  void dump_config() override;
  void loop() override;
  float get_loop_priority() const override;
  float get_setup_priority() const override;

  void begin_sleep(bool manual = false);
  void prevent_deep_sleep();
  void allow_deep_sleep();

 protected:
  optional<uint64_t> sleep_duration_;
  InternalGPIOPin *wakeup_pin_{nullptr};
  optional<uint32_t> run_duration_;
  bool next_enter_deep_sleep_{false};
  bool prevent_{false};
};

extern bool global_has_deep_sleep;

}  // namespace deep_sleep
}  // namespace esphome
