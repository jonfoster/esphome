#pragma once

#include <stdint.h>

namespace esphome {
namespace milight_rx {

class MiLightRxRadioEventHandler {
 public:
  virtual void milight_radio_event_handler(const uint8_t *packet, unsigned packet_length) = 0;
};

class MiLightRxRadioEventSource {
 public:
  virtual void add_radio_event_listener(MiLightRxRadioEventHandler *listener) = 0;
};

}  // namespace milight_rx
}  // namespace esphome