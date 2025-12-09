#include "deep_sleep_component.h"
#include "esphome/core/application.h"
#include "esphome/core/log.h"

namespace esphome {
namespace deep_sleep {

static const char *const TAG = "deep_sleep";
static const uint32_t TEARDOWN_TIMEOUT_DEEP_SLEEP_MS = 5000;

bool global_has_deep_sleep = false;

void DeepSleepComponent::setup() {
  global_has_deep_sleep = true;
  const optional<uint32_t> run_duration = get_run_duration_();
  if (run_duration.has_value()) {
    ESP_LOGI(TAG, "Scheduling in %" PRIu32 " ms", *run_duration);
    this->set_timeout(*run_duration, [this]() { this->begin_sleep(); });
  }
}

void DeepSleepComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Deep sleep:");
  if (this->sleep_duration_.has_value()) {
    ESP_LOGCONFIG(TAG, "  Sleep Duration: %" PRIu64 " us", *this->sleep_duration_);
  }
  if (this->run_duration_.has_value()) {
    ESP_LOGCONFIG(TAG, "  Run Duration: %" PRIu32 " ms", *this->run_duration_);
  }

#ifdef USE_LIBRETINY
  if (this->libretiny_wakeup_pin_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Wakeup Pin (Libretiny): %u", this->libretiny_wakeup_pin_->get_pin());
    ESP_LOGCONFIG(TAG, "  Wake Level: %s",
                  this->libretiny_wake_level_ == LIBRETINY_WAKE_ON_HIGH ? "HIGH" : "LOW");
  }
#endif

  this->dump_config_platform_();
}

void DeepSleepComponent::loop() {
  if (this->next_enter_deep_sleep_) {
    this->begin_sleep();
  }
}

float DeepSleepComponent::get_loop_priority() const { return -100.0f; }
float DeepSleepComponent::get_setup_priority() const { return setup_priority::LATE; }

void DeepSleepComponent::set_sleep_duration(uint32_t time_ms) { this->sleep_duration_ = uint64_t(time_ms) * 1000ULL; }
void DeepSleepComponent::set_run_duration(uint32_t time_ms) { this->run_duration_ = time_ms; }
optional<uint32_t> DeepSleepComponent::get_run_duration_() const { return this->run_duration_; }

void DeepSleepComponent::begin_sleep(bool manual) {
  if (this->prevent_ && !manual) {
    this->next_enter_deep_sleep_ = true;
    return;
  }
  if (!this->prepare_to_sleep_()) {
    ESP_LOGW(TAG, "prepare_to_sleep_() failed; aborting deep sleep");
    return;
  }

  ESP_LOGI(TAG, "Beginning sleep");
  App.run_safe_shutdown_hooks();
  App.teardown_components(TEARDOWN_TIMEOUT_DEEP_SLEEP_MS);
  App.run_powerdown_hooks();
  this->deep_sleep_();
}

void DeepSleepComponent::prevent_deep_sleep() { this->prevent_ = true; }
void DeepSleepComponent::allow_deep_sleep() { this->prevent_ = false; }

bool DeepSleepComponent::prepare_to_sleep_() {
#ifdef USE_LIBRETINY
  if (this->libretiny_wakeup_pin_ != nullptr) {
    uint8_t pin = this->libretiny_wakeup_pin_->get_pin();
    bool wake_high = (this->libretiny_wake_level_ == LIBRETINY_WAKE_ON_HIGH);
    ESP_LOGD(TAG, "Configuring Libretiny wakeup pin: %u, level: %s", pin, wake_high ? "HIGH" : "LOW");

    // TODO: Libretiny SDK 함수로 교체 필요
    // lt_gpio_setup_input(pin, LT_GPIO_PULL_UP);
    // lt_sleep_enable_gpio_wakeup(pin, wake_high ? LT_WAKE_HIGH : LT_WAKE_LOW);
  }
  return true;
#else
  return true;
#endif
}

void DeepSleepComponent::deep_sleep_() {
#ifdef USE_LIBRETINY
  const uint64_t sleep_us = this->sleep_duration_.value_or(0ULL);
  if (sleep_us > 0ULL) {
    ESP_LOGI(TAG, "Libretiny deep sleep for %" PRIu64 " us", sleep_us);
    // TODO: Libretiny SDK timed sleep 호출
    // lt_sleep_for_us(sleep_us);
  } else {
    ESP_LOGI(TAG, "Libretiny deep sleep indefinite");
    // TODO: Libretiny SDK indefinite sleep 호출
    // lt_sleep_start();
  }
  while (true) { delay(1000); }
#else
  ESP_LOGE(TAG, "Deep sleep not implemented for this platform");
#endif
}

void DeepSleepComponent::dump_config_platform_() {
#ifdef USE_LIBRETINY
  ESP_LOGCONFIG(TAG, "  Platform: Libretiny (Beken)");
#else
  ESP_LOGCONFIG(TAG, "  Platform: Unknown/Generic");
#endif
}

}  // namespace deep_sleep
}  // namespace esphome
