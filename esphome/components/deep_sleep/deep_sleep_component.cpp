#include "deep_sleep_component.h"
#include "esphome/core/application.h"
#include "esphome/core/log.h"

namespace esphome {
namespace deep_sleep {

static const char *const TAG = "deep_sleep";
bool global_has_deep_sleep = false;

void DeepSleepComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Deep Sleep...");
  global_has_deep_sleep = true;
}

void DeepSleepComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Deep Sleep:");
  if (this->sleep_duration_.has_value()) {
    ESP_LOGCONFIG(TAG, "  Sleep Duration: %u ms", (uint32_t)(*this->sleep_duration_ / 1000ULL));
  }
}

void DeepSleepComponent::loop() {
  if (this->next_enter_deep_sleep_)
    this->begin_sleep();
}

float DeepSleepComponent::get_loop_priority() const { return -100.0f; }
float DeepSleepComponent::get_setup_priority() const { return setup_priority::LATE; }

void DeepSleepComponent::set_sleep_duration(uint32_t time_ms) {
  this->sleep_duration_ = (uint64_t)time_ms * 1000ULL;
}

void DeepSleepComponent::begin_sleep(bool manual) {
  if (this->prevent_ && !manual) {
    this->next_enter_deep_sleep_ = true;
    return;
  }

  ESP_LOGI(TAG, "Beginning Deep Sleep");

#ifdef USE_ESP32
  if (this->sleep_duration_.has_value())
    esp_sleep_enable_timer_wakeup(*this->sleep_duration_);
  esp_deep_sleep_start();
#endif

#ifdef USE_ESP8266
  if (this->sleep_duration_.has_value())
    ESP.deepSleep(*this->sleep_duration_);
  else
    ESP.deepSleep(0);
#endif

#ifdef USE_LIBRETINY
  if (this->sleep_duration_.has_value()) {
    lt_deep_sleep_start(*this->sleep_duration_);
  } else {
    lt_deep_sleep_start(0);
  }
  if (this->wakeup_pin_ != nullptr) {
    bool level = !this->wakeup_pin_->is_inverted();
    lt_enable_gpio_wakeup(this->wakeup_pin_->get_pin(), level);
  }
#endif
}

void DeepSleepComponent::prevent_deep_sleep() { this->prevent_ = true; }
void DeepSleepComponent::allow_deep_sleep() { this->prevent_ = false; }

}  // namespace deep_sleep
}  // namespace esphome
