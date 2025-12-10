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
#ifdef USE_LIBRETINY
#include <vector>
#endif

namespace esphome {
namespace deep_sleep {

#if defined(USE_ESP32) || defined(USE_LIBRETINY)

/** The values of this enum define what should be done if deep sleep is set up with a wakeup pin
 * and the scenario occurs that the wakeup pin is already in the wakeup state.
 */
enum WakeupPinMode {
  WAKEUP_PIN_MODE_IGNORE = 0,  ///< Ignore the fact that we will wake up when going into deep sleep.
  WAKEUP_PIN_MODE_KEEP_AWAKE,  ///< As long as the wakeup pin is still in the wakeup state, keep awake.

  /** Automatically invert the wakeup level. For example if we were set up to wake up on HIGH, but the pin
   * is already high when attempting to enter deep sleep, re-configure deep sleep to wake up on LOW level.
   */
  WAKEUP_PIN_MODE_INVERT_WAKEUP,
};

#if !defined(USE_LIBRETINY)
struct Ext1Wakeup {
  uint64_t mask;
  esp_sleep_ext1_wakeup_mode_t wakeup_mode;
};
#else
struct WakeUpPinItem {
  InternalGPIOPin *wakeup_pin;
  WakeupPinMode wakeup_pin_mode;
};
#endif

struct WakeupCauseToRunDuration {
  // Run duration if woken up by timer or any other reason besides those below.
  uint32_t default_cause;
  // Run duration if woken up by touch pads.
  uint32_t touch_cause;
  // Run duration if woken up by GPIO pins.
  uint32_t gpio_cause;
};

#endif  // defined(USE_ESP32) || defined(USE_LIBRETINY)

template<typename... Ts> class EnterDeepSleepAction;
template<typename... Ts> class PreventDeepSleepAction;

/** This component allows setting up the node to go into deep sleep mode to conserve battery.
 *
 * To set this component up, first set *when* the deep sleep should trigger using set_run_cycles
 * and set_run_duration, then set how long the deep sleep should last using set_sleep_duration and optionally
 * set_wakeup_pin.
 */
class DeepSleepComponent : public Component {
 public:
  /// Set the duration in ms the component should sleep once it's in deep sleep mode.
  void set_sleep_duration(uint32_t time_ms);

#if defined(USE_ESP32) || defined(USE_LIBRETINY)
  /** Set the pin to wake up once it's in deep sleep mode.
   * Use the inverted property to set the wakeup level.
   */
  void set_wakeup_pin(InternalGPIOPin *pin) { this->wakeup_pin_ = pin; }

  void set_wakeup_pin_mode(WakeupPinMode wakeup_pin_mode);
#endif

#ifdef USE_LIBRETINY
  void add_wakeup_pin(const WakeUpPinItem pin) { this->wakeup_pins_.push_back(pin); }
#endif

#if defined(USE_ESP32) || defined(USE_LIBRETINY)
#if !defined(USE_ESP32_VARIANT_ESP32C3) && !defined(USE_LIBRETINY)
  void set_ext1_wakeup(Ext1Wakeup ext1_wakeup);
  void set_touch_wakeup(bool touch_wakeup);
#endif

  // Set the duration in ms for how long the code should run before entering deep sleep mode,
  // according to the cause the device has woken.
  void set_run_duration(WakeupCauseToRunDuration wakeup_cause_to_run_duration);
#endif

  /// Set a duration in ms for how long the code should run before entering deep sleep mode.
  void set_run_duration(uint32_t time_ms);

  void setup() override;
  void dump_config() override;
  void loop() override;
  float get_loop_priority() const override;
  float get_setup_priority() const override;

  /// Helper to enter deep sleep mode
  void begin_sleep(bool manual = false);

  void prevent_deep_sleep();
  void allow_deep_sleep();

 protected:
  // Returns nullopt if no run duration is set. Otherwise, returns the run duration before entering deep sleep.
  optional<uint32_t> get_run_duration_() const;

  optional<uint64_t> sleep_duration_;
#if defined(USE_ESP32) || defined(USE_LIBRETINY)
  InternalGPIOPin *wakeup_pin_{nullptr
