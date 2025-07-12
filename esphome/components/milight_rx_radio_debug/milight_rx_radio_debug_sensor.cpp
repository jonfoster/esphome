#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "milight_rx_radio_debug_sensor.h"

namespace esphome {
namespace milight_rx {

static const char *TAG = "milight_rx_radio_debug";

void MiLightRxRadioDebugSensor::milight_radio_event_handler(const uint8_t *packet, unsigned packet_length) {
  std::string new_state = format_hex_pretty(packet, packet_length);
  ESP_LOGI(TAG, "Received packet: %s", new_state.c_str());

  publish_state(new_state);
}

void MiLightRxRadioDebugSensor::dump_config() { ESP_LOGCONFIG(TAG, "Milight RX Debug sensor"); }

}  // namespace milight_rx
}  // namespace esphome