#include "esphome/components/milight_rx/milight_rx_remote_event.h"
#include "milight_parser_internal.h"
#include <cmath>
#include <algorithm>

namespace esphome {
namespace milight_rx {

enum class FUT091Command { ON_OFF = 0x01, BRIGHTNESS = 0x2, KELVIN = 0x03 };

static const uint8_t BRIGHTNESS_SCALE_OFFSET = 0x97;
static const uint8_t KELVIN_SCALE_OFFSET = 0xC5;

static float fromv2scale(uint8_t value, uint8_t offset) {
  static constexpr uint8_t guard = 20u;

  value = std::clamp<int>((int) ((value - (offset - guard)) & 0xFFu) - guard, 0, 200);

  return value / 200.0;
}

bool FUT091_parsePacket(const uint8_t *packet, unsigned packet_length, MiLightRemoteEvent &result) {
  if (packet_length != 8) {
    return false;
  }
  if (packet[0] > 3) {
    // Not a channel that has FUT091 packets.
    return false;
  }
  if (packet[1] != 0x21) {
    return false;
  }

  result.remote_type = MiLightRemoteType::FUT091;
  result.remote_id = (packet[2] << 8) | packet[3];
  result.group_id = packet[7];

  uint8_t full_command = packet[4];
  uint8_t command = (full_command & 0x7F);
  uint8_t arg = packet[5];

  if (command == (uint8_t) FUT091Command::ON_OFF) {
    if ((full_command & 0x80) == 0x80) {
      result.command = MiLightRemoteCommand::NIGHT_MODE;
    } else if (arg < 5) {  // Group is not reliably encoded in group byte. Extract from arg byte
      result.command = MiLightRemoteCommand::ON;
      result.group_id = arg;
    } else {
      result.command = MiLightRemoteCommand::OFF;
      result.group_id = arg - 5;
    }
  } else if (command == (uint8_t) FUT091Command::BRIGHTNESS) {
    result.command = MiLightRemoteCommand::SET_BRIGHTNESS;
    result.arg1 = 1.0 - fromv2scale(arg, BRIGHTNESS_SCALE_OFFSET);
  } else if (command == (uint8_t) FUT091Command::KELVIN) {
    result.command = MiLightRemoteCommand::SET_COLOR_TEMP;
    result.arg1 = fromv2scale(arg, KELVIN_SCALE_OFFSET);
  } else {
    // result["button_id"] = command;
    // result["argument"] = arg;
    return false;
  }

  return true;
}

}  // namespace milight_rx
}  // namespace esphome