#pragma once

#include <stdint.h>

namespace esphome {
namespace milight_rx {

enum class MiLightRemoteType {
  UNKNOWN = 255,
  // RGBW    = 0,  // Currently not supported, but could be added later.
  // CCT     = 1,  // Currently not supported, but could be added later.
  FUT092 = 2,  // Also called RGB_CCT
  // RGB     = 3,  // Currently not supported, but could be added later.
  FUT089 = 4,
  FUT091 = 5,
  // FUT020  = 6  // Currently not supported, but could be added later.
};

enum class MiLightRemoteCommand {
  UNKNOWN = 255,

  // No arguments
  OFF = 0,

  // No arguments
  ON,

  // No arguments
  NIGHT_MODE,

  // No arguments
  MODE_SPEED_DOWN,

  // No arguments
  MODE_SPEED_UP,

  // No arguments
  SET_WHITE,

  // Arg1 = hue (float 0-1)
  SET_HUE,

  // Arg1 = brightness (float 0-1)
  SET_BRIGHTNESS,

  // Arg1 = color temperature (float 0-1)
  SET_COLOR_TEMP,

  // Arg1 = saturation (float 0-1)
  SET_SATURATION,

  // Arg1 = mode (integer 0-255)
  SET_MODE,

  // Set saturation if bulb is in color mode.
  // Set color temperature if bulb is in white mode.
  // Arg1 = new value (float 0-1)
  SET_SATURATION_OR_COLOR_TEMP,
};

struct MiLightRemoteEvent {
  MiLightRemoteType remote_type = MiLightRemoteType::UNKNOWN;
  uint16_t remote_id = 0;
  uint8_t group_id = 0;
  MiLightRemoteCommand command = MiLightRemoteCommand::UNKNOWN;
  float arg1 = 0;

  MiLightRemoteEvent() = default;
  MiLightRemoteEvent(const MiLightRemoteEvent &other) = default;
  bool operator==(const MiLightRemoteEvent &other) const = default;
  MiLightRemoteEvent &operator=(const MiLightRemoteEvent &other) = default;
};

class MiLightRxRemoteEventHandler {
 public:
  virtual void milight_remote_event_handler(MiLightRemoteEvent const &event) = 0;
};

class MiLightRxRemoteEventSource {
 public:
  virtual void add_remote_event_listener(MiLightRxRemoteEventHandler *listener) = 0;
};

}  // namespace milight_rx
}  // namespace esphome