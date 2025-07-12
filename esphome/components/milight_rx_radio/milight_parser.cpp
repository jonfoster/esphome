#include "esphome/components/milight_rx/milight_rx_remote_event.h"
#include "milight_parser_internal.h"

namespace esphome {
namespace milight_rx {

bool parse_packet(const uint8_t *packet, unsigned packet_length, MiLightRemoteEvent &result) {
  bool ok = FUT089_parsePacket(packet, packet_length, result) || FUT091_parsePacket(packet, packet_length, result) ||
            FUT092_parsePacket(packet, packet_length, result);

  return ok;
}

}  // namespace milight_rx
}  // namespace esphome
