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

namespace esphome {
namespace deep_sleep {

#ifdef USE_ESP32

// Behavior when a wakeup pin is already in the wakeup state at the moment we try to sleep.
enum WakeupPinMode {
  WAKEUP_PIN_MODE_IGNORE = 0,    // Ignore and go to deep sleep anyway.
  WAKEUP_PIN_MODE_KEEP_AWAKE,    // Stay awake while the wakeup pin remains in the wakeup state.
  WAKEUP_PIN_MODE_INVERT_WAKEUP, // Flip the wakeup level to allow entering deep sleep immediately.
};

struct Ext1Wakeup {
  uint64_t mask;
  esp_sleep_ext1_wakeup_mode_t wakeup_mode;
};

struct WakeupCauseToRunDuration {
  // Run duration if woken by timer or any other cause not explicitly listed below.
  uint32_t default_cause;
  // Run duration if woken by touch pads.
  uint32_t touch_cause;
  // Run duration if woken by GPIO pins.
  uint32_t gpio_cause;
};

#endif  // USE_ESP32

template<typename... Ts> class EnterDeepSleepAction;
template<typename... Ts> class PreventDeepSleepAction;

/**
 * DeepSleepComponent lets the node enter deep sleep to conserve power.
 *
 * Configure:
 * - set_run_duration / set_run_duration(WakeupCauseToRunDuration) for how long to run before sleeping
 * - set_sleep_duration for how long to sleep
 * - on ESP32, optionally set_wakeup_pin and set_wakeup_pin_mode
 */
class DeepSleepComponent : public Component {
 public:
  // Set the duration in ms to sleep once entering deep sleep.
  void set_sleep_duration(uint32_t time_ms);

#if defined(USE_ESP32)
  // Set the pin used for GPIO wakeup (level determined by pin invert property).
  void set_wakeup_pin(InternalGPIOPin *pin) { this->wakeup_pin_ = pin; }

  void set_wakeup_pin_mode(WakeupPinMode wakeup_pin_mode);
#endif

#if defined(USE_ESP32)
#if !defined(USE_ESP32_VARIANT_ESP32C3)
  // Configure EXT1 wakeup (bitmap mask of RTC-capable GPIO and mode).
  void set_ext1_wakeup(Ext1Wakeup ext1_wakeup);

  // Enable/disable touch wakeup support.
  void set_touch_wakeup(bool touch_wakeup);
#endif  // !USE_ESP32_VARIANT_ESP32C3

  // Cause-aware run duration before entering deep sleep (ESP32 only).
  void set_run_duration(WakeupCauseToRunDuration wakeup_cause_to_run_duration);
#endif  // USE_ESP32

  // Simple run duration (all platforms).
  void set_run_duration(uint32_t time_ms);

  void setup() override;
  void dump_config() override;
  void loop() override;
  float get_loop_priority() const override;
  float get_setup_priority() const override;

  // Enter deep sleep manually (immediately or at next loop boundary depending on implementation).
  void begin_sleep(bool manual = false);

  // Temporarily prevent deep sleep; call allow_deep_sleep() to resume.
  void prevent_deep_sleep();
  void allow_deep_sleep();

 protected:
  // Returns nullopt if no run duration is set; otherwise, the run duration in ms.
  optional<uint32_t> get_run_duration_() const;

  optional<uint64_t> sleep_duration_;
#ifdef USE_ESP32
  InternalGPIOPin *wakeup_pin_{nullptr};
  WakeupPinMode wakeup_pin_mode_{WAKEUP_PIN_MODE_IGNORE};
  optional<Ext1Wakeup> ext1_wakeup_;
  optional<bool> touch_wakeup_;
  optional<WakeupCauseToRunDuration> wakeup_cause_to_run_duration_;
#endif
  optional<uint32_t> run_duration_;
  bool next_enter_deep_sleep_{false};
  bool prevent_{false};
};

extern bool global_has_deep_sleep;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

template<typename... Ts> class EnterDeepSleepAction : public Action<Ts...> {
 public:
  explicit EnterDeepSleepAction(DeepSleepComponent *deep_sleep) : deep_sleep_(deep_sleep) {}
  TEMPLATABLE_VALUE(uint32_t, sleep_duration);

#ifdef USE_TIME
  void set_until(uint8_t hour, uint8_t minute, uint8_t second) {
    this->hour_ = hour;
    this->minute_ = minute;
    this->second_ = second;
  }

  void set_time(time::RealTimeClock *time) { this->time_ = time; }
#endif

  void play(Ts... x) override {
    if (this->sleep_duration_.has_value()) {
      this->deep_sleep_->set_sleep_duration(this->sleep_duration_.value(x...));
    }

#ifdef USE_TIME
    // If an absolute time-of-day is set, compute ms until that time today/tomorrow and sleep for that duration.
    if (this->hour_.has_value()) {
      auto t = this->time_->now();
      const uint32_t timestamp_now = t.timestamp;

      bool after_time = false;
      if (t.hour > *this->hour_) {
        after_time = true;
      } else if (t.hour == *this->hour_) {
        if (t.minute > *this->minute_) {
          after_time = true;
        } else if (t.minute == *this->minute_) {
          if (t.second > *this->second_) {
            after_time = true;
          }
        }
      }

      t.hour = *this->hour_;
      t.minute = *this->minute_;
      t.second = *this->second_;
      t.recalc_timestamp_utc();

      time_t target_local = t.timestamp;  // local time zone
      if (after_time) {
        target_local += 60 * 60 * 24;     // move to next day
      }

      const int32_t offset = ESPTime::timezone_offset();
      const uint32_t ms_left = static_cast<uint32_t>((target_local - timestamp_now - offset) * 1000);
      this->deep_sleep_->set_sleep_duration(ms_left);
    }
#endif

    this->deep_sleep_->begin_sleep(true);
  }

 protected:
  DeepSleepComponent *deep_sleep_;
#ifdef USE_TIME
  optional<uint8_t> hour_;
  optional<uint8_t> minute_;
  optional<uint8_t> second_;
  time::RealTimeClock *time_{nullptr};
#endif
};

template<typename... Ts>
class PreventDeepSleepAction : public Action<Ts...>, public Parented<DeepSleepComponent> {
 public:
  void play(Ts... x) override { this->parent_->prevent_deep_sleep(); }
};

template<typename... Ts>
class AllowDeepSleepAction : public Action<Ts...>, public Parented<DeepSleepComponent> {
 public:
  void play(Ts... x) override { this->parent_->allow_deep_sleep(); }
};

}  // namespace deep_sleep
}  // namespace esphome
