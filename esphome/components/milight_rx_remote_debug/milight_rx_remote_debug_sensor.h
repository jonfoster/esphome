#pragma once

#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/milight_rx/milight_rx_remote_event.h"

namespace esphome {
namespace milight_rx {

class MiLightRxRemoteDebugSensor : public text_sensor::TextSensor,
                                   public Component,
                                   public MiLightRxRemoteEventHandler {
 public:
  void dump_config() override;

  void milight_remote_event_handler(MiLightRemoteEvent const &event) override;
};

}  // namespace milight_rx
}  // namespace esphome