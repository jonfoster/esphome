#pragma once

#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/milight_rx/milight_rx_radio_event.h"

namespace esphome {
namespace milight_rx {

class MiLightRxRadioDebugSensor : public text_sensor::TextSensor, public Component, public MiLightRxRadioEventHandler {
 public:
  void dump_config() override;

  void milight_radio_event_handler(const uint8_t *packet, unsigned packet_length) override;
};

}  // namespace milight_rx
}  // namespace esphome