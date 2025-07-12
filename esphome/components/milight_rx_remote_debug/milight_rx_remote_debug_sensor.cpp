#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "milight_rx_remote_debug_sensor.h"

namespace esphome {
namespace milight_rx {

static const char *TAG = "milight_rx_remote_debug";

void MiLightRxRemoteDebugSensor::milight_remote_event_handler(MiLightRemoteEvent const &event) {
  std::string new_state =
      str_sprintf("Type: %u, ID: 0x%04x, Group: %u, Command: %u, Arg1: %f", static_cast<unsigned>(event.remote_type),
                  event.remote_id, event.group_id, static_cast<unsigned>(event.command), event.arg1);

  ESP_LOGI(TAG, "Received packet: %s", new_state.c_str());

  publish_state(new_state);
}

void MiLightRxRemoteDebugSensor::dump_config() { ESP_LOGCONFIG(TAG, "Milight RX Remote Debug sensor"); }

}  // namespace milight_rx
}  // namespace esphome