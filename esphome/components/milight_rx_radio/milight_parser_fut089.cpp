#include "esphome/components/milight_rx/milight_rx_remote_event.h"
#include "milight_parser_internal.h"
#include <cmath>
#include <algorithm>

namespace esphome {
namespace milight_rx {

enum MiLightFUT089Command {
  FUT089_ON = 0x01,
  FUT089_OFF = 0x01,
  FUT089_COLOR = 0x02,

  // Usually seen on a "K1 Rotating Switch Panel Remote", when button rotated while pushed in.
  FUT089_COLOR_TEMP = 0x03,

  FUT089_BRIGHTNESS = 0x05,
  FUT089_MODE = 0x06,
  // Controls Kelvin when in White mode.  Controls Saturation when in Color mode
  FUT089_KELVIN_OR_SATURATION = 0x07,

  // Sometimes seen on a "K1 Rotating Switch Panel Remote", when button rotated while pushed in.
  // TODO figure out what this is and give it a better name.
  FUT089_ROTATE_UNKNOWN = 0x09,
};

enum MiLightFUT089Arguments { FUT089_MODE_SPEED_UP = 0x12, FUT089_MODE_SPEED_DOWN = 0x13, FUT089_WHITE_MODE = 0x14 };

bool FUT089_parsePacket(const uint8_t *packet, unsigned packet_length, MiLightRemoteEvent &result) {
  if (packet_length != 8) {
    return false;
  }
  if (packet[0] > 3) {
    // Not a channel that has FUT089 packets.
    return false;
  }
  if (packet[1] != 0x25) {
    return false;
  }

  result.remote_type = MiLightRemoteType::FUT089;
  result.remote_id = (packet[2] << 8) | packet[3];
  result.group_id = packet[7];

  uint8_t full_command = packet[4];
  uint8_t command = (full_command & 0x7F);
  uint8_t arg = packet[5];

  if (command == FUT089_ON) {
    if ((full_command & 0x80) == 0x80) {
      result.command = MiLightRemoteCommand::NIGHT_MODE;
    } else if (arg == FUT089_MODE_SPEED_DOWN) {
      result.command = MiLightRemoteCommand::MODE_SPEED_DOWN;
    } else if (arg == FUT089_MODE_SPEED_UP) {
      result.command = MiLightRemoteCommand::MODE_SPEED_UP;
    } else if (arg == FUT089_WHITE_MODE) {
      result.command = MiLightRemoteCommand::SET_WHITE;
    } else if (arg <= 8) {  // Group is not reliably encoded in group byte. Extract from arg byte
      result.command = MiLightRemoteCommand::ON;
      result.group_id = arg;
    } else if (arg >= 9 && arg <= 17) {
      result.command = MiLightRemoteCommand::OFF;
      result.group_id = arg - 9;
    }
  } else if (command == FUT089_COLOR) {
    result.command = MiLightRemoteCommand::SET_HUE;
    result.arg1 = arg / 255.0;
  } else if (command == FUT089_BRIGHTNESS) {
    result.command = MiLightRemoteCommand::SET_BRIGHTNESS;
    result.arg1 = std::clamp<uint8_t>(arg, 0, 100) / 100.0;
  } else if (command == FUT089_KELVIN_OR_SATURATION) {
    result.command = MiLightRemoteCommand::SET_SATURATION_OR_COLOR_TEMP;
    // saturation or color temperature
    result.arg1 = (100 - std::clamp<uint8_t>(arg, 0, 100)) / 100.0;
  } else if (command == FUT089_COLOR_TEMP) {
    result.command = MiLightRemoteCommand::SET_COLOR_TEMP;
    result.arg1 = std::clamp<uint8_t>(arg, 0, 100) / 100.0;
  } else if (command == FUT089_ROTATE_UNKNOWN) {
    result.command = MiLightRemoteCommand::SET_HUE;  // TODO figure out what this should be
    result.arg1 = arg / 255.0;
  } else if (command == FUT089_MODE) {
    result.command = MiLightRemoteCommand::SET_MODE;
    result.arg1 = arg;
  } else {
    // result["button_id"] = command;
    // result["argument"] = arg;
    return false;
  }

  return true;
}

}  // namespace milight_rx
}  // namespace esphome