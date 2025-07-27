#pragma once

#include "esphome/core/component.h"
#include "esphome/components/milight_rx/milight_rx_remote_event.h"
#include "esphome/components/light/light_state.h"
#include <vector>

namespace esphome {
namespace milight_rx {
namespace target {

class MiLightRxTargetComponent : public Component, public MiLightRxRemoteEventHandler {
 public:
  void dump_config() override;

  void set_light(light::LightState *light) { light_ = light; }

  void milight_remote_event_handler(MiLightRemoteEvent const &event) override;

 protected:
  void set_white();
  void set_white_warmness(float warmness);
  void set_color_hsv(optional<float> hue, optional<float> saturation, optional<float> value);

  light::LightState *light_;
};

}  // namespace target
}  // namespace milight_rx
}  // namespace esphome