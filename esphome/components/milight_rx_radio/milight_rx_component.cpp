#include "esphome/core/log.h"
#include "milight_rx_component.h"
#include "milight_radio_config.h"
#include "milight_parser_internal.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace milight_rx {
namespace radio {

static const char *TAG = "milight_rx_radio";

void MiLightRxComponent::setup() {
  spi_setup();

  int config_index = config_index_;
  if (config_index < 0) {
    // Scan mode
    scan_current_config_index_ = 0;
    config_index = scan_current_config_index_;
  }
  bool ok = impl_.set_config_index(config_index);
  if (ok) {
    ok = impl_.setup(delegate_, ce_pin_, irq_pin_);
  }
  if (!ok) {
    ESP_LOGE(TAG, "Setup failed");
    mark_failed("Setup failed");
  } else if (config_index_ < 0) {
    scan_next_step_time_ = millis() + scan_time_millis_ * 1000u;
  }
}

void MiLightRxComponent::try_to_read() {
  uint8_t packet_buf[MiLightRadioConfig::MAX_PACKET_LENGTH + 1];
  unsigned packet_length = sizeof(packet_buf) - 1;
  if (!impl_.read(packet_buf + 1, packet_length)) {
    return;
  }

  packet_buf[0] = (uint8_t) impl_.get_config_index();
  packet_length++;

  handle_raw_packet(packet_buf, packet_length);
}

void MiLightRxComponent::handle_raw_packet(const uint8_t *packet_buf, unsigned packet_length) {
  std::string new_state = format_hex_pretty(packet_buf, packet_length);

  ESP_LOGI(TAG, "Recieved packet: %s", new_state.c_str());

  for (auto *listener : radio_packet_listeners_) {
    listener->milight_radio_event_handler(packet_buf, packet_length);
  }

  MiLightRemoteEvent event;
  if (!parse_packet(packet_buf, packet_length, event)) {
    ESP_LOGW(TAG, "Failed to decode packet: %s", new_state.c_str());
    return;
  }

  for (auto *listener : remote_packet_listeners_) {
    listener->milight_remote_event_handler(event);
  }
}

void MiLightRxComponent::advance_scan_if_needed() {
  // If scanning, and time to move to next channel, then do so.
  if (config_index_ < 0 && millis() - scan_next_step_time_ > 0) {
    scan_current_config_index_++;
    if (scan_current_config_index_ >= impl_.NUM_CONFIGS) {
      scan_current_config_index_ = 0;
    }

    ESP_LOGI(TAG, "Reconfiguring to config %u", scan_current_config_index_);

    bool ok = impl_.set_config_index(scan_current_config_index_);
    if (!ok) {
      ESP_LOGE(TAG, "Failed to reconfigure to config %u", scan_current_config_index_);
      mark_failed("Reconfigure failed");
    } else {
      scan_next_step_time_ = millis() + scan_time_millis_ * 1000u;
    }
  }
}

void MiLightRxComponent::loop() {
  try_to_read();

  advance_scan_if_needed();
}

void MiLightRxComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "milight_rx Component");
  LOG_PIN("  ce_pin = %s", ce_pin_);
  LOG_PIN("  irq_pin = %s", irq_pin_);
  ESP_LOGCONFIG(TAG, "  config_index = %d", config_index_);
  ESP_LOGCONFIG(TAG, "  scan_time_millis = %d", scan_time_millis_);
}

}  // namespace radio
}  // namespace milight_rx
}  // namespace esphome
