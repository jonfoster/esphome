#pragma once

#include "esphome/components/milight_rx/milight_rx_remote_event.h"

namespace esphome {
namespace milight_rx {

bool parse_packet(const uint8_t *packet, unsigned packet_length, MiLightRemoteEvent &result);

bool FUT089_parsePacket(const uint8_t *packet, unsigned packet_length, MiLightRemoteEvent &result);
bool FUT091_parsePacket(const uint8_t *packet, unsigned packet_length, MiLightRemoteEvent &result);
bool FUT092_parsePacket(const uint8_t *packet, unsigned packet_length, MiLightRemoteEvent &result);

}  // namespace milight_rx
}  // namespace esphome