#include "esphome/components/milight_rx/milight_rx_remote_event.h"
#include "milight_parser_internal.h"
#include <cmath>
#include <algorithm>

namespace esphome {
namespace milight_rx {

enum MiLightRgbCctCommand {
  RGB_CCT_ON = 0x01,
  RGB_CCT_OFF = 0x01,
  RGB_CCT_COLOR = 0x02,
  RGB_CCT_KELVIN = 0x03,
  RGB_CCT_BRIGHTNESS = 0x04,
  RGB_CCT_SATURATION = 0x04,
  RGB_CCT_MODE = 0x05
};

enum MiLightRgbCctArguments { RGB_CCT_MODE_SPEED_UP = 0x0A, RGB_CCT_MODE_SPEED_DOWN = 0x0B };

#define RGB_CCT_NUM_MODES 9

#define RGB_CCT_COLOR_OFFSET 0x5F
#define RGB_CCT_BRIGHTNESS_OFFSET 0x8F
#define RGB_CCT_SATURATION_OFFSET 0xD
#define RGB_CCT_KELVIN_OFFSET 0x94

// Remotes have a larger range
#define RGB_CCT_KELVIN_REMOTE_START 0x94
#define RGB_CCT_KELVIN_REMOTE_END 0xCC

static float fromv2scale(uint8_t value, uint8_t offset) {
  static constexpr uint8_t guard = 20u;

  value = std::clamp<int>((int) ((value - (offset - guard)) & 0xFFu) - guard, 0, 200);

  return value / 200.0;
}

bool FUT092_parsePacket(const uint8_t *packet, unsigned packet_length, MiLightRemoteEvent &result) {
  if (packet_length != 8) {
    return false;
  }
  if (packet[0] > 3) {
    // Not a channel that has FUT092 packets.
    return false;
  }
  if (packet[1] != 0x20) {
    return false;
  }

  result.remote_type = MiLightRemoteType::FUT092;
  result.remote_id = (packet[2] << 8) | packet[3];
  result.group_id = packet[7];

  uint8_t full_command = packet[4];
  uint8_t command = (full_command & 0x7F);
  uint8_t arg = packet[5];

  if (command == RGB_CCT_ON) {
    if ((full_command & 0x80) == 0x80) {
      result.command = MiLightRemoteCommand::NIGHT_MODE;
    } else if (arg == RGB_CCT_MODE_SPEED_DOWN) {
      result.command = MiLightRemoteCommand::MODE_SPEED_DOWN;
    } else if (arg == RGB_CCT_MODE_SPEED_UP) {
      result.command = MiLightRemoteCommand::MODE_SPEED_UP;
    } else if (arg < 5) {  // Group is not reliably encoded in group byte. Extract from arg byte
      result.command = MiLightRemoteCommand::ON;
      result.group_id = arg;
    } else {
      result.command = MiLightRemoteCommand::OFF;
      result.group_id = arg - 5;
    }
  } else if (command == RGB_CCT_COLOR) {
    result.command = MiLightRemoteCommand::SET_HUE;
    result.arg1 = ((arg - RGB_CCT_COLOR_OFFSET) & 0xFFu) / 255.0;
  } else if (command == RGB_CCT_KELVIN) {
    result.command = MiLightRemoteCommand::SET_COLOR_TEMP;
    result.arg1 = 1.0 - fromv2scale(arg, RGB_CCT_KELVIN_REMOTE_END);
    // brightness == saturation
  } else if (command == RGB_CCT_BRIGHTNESS && arg >= (RGB_CCT_BRIGHTNESS_OFFSET - 15)) {
    result.command = MiLightRemoteCommand::SET_BRIGHTNESS;
    result.arg1 = std::clamp<int>(arg - RGB_CCT_BRIGHTNESS_OFFSET, 0, 100) / 100.0;
  } else if (command == RGB_CCT_SATURATION) {
    result.command = MiLightRemoteCommand::SET_SATURATION;
    result.arg1 = std::clamp<int>(arg - RGB_CCT_SATURATION_OFFSET, 0, 100) / 100.0;
  } else if (command == RGB_CCT_MODE) {
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