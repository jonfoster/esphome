#pragma once

#include <stdint.h>

namespace esphome {
namespace milight_rx {

/**
 * Identifies the remote control sub-protocols that are supported.
 *
 * @see MiLightRemoteEvent
 */
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

/**
 * Identifies the commands that can be sent by MiLight remotes.
 *
 * @see MiLightRemoteEvent
 */
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

/**
 * A command received from a MiLight remote.
 *
 * This is a fully parsed and interpreted command.
 */
struct MiLightRemoteEvent {
  /** The remote control sub-protocol that was used. */
  MiLightRemoteType remote_type = MiLightRemoteType::UNKNOWN;

  /** The ID of the remote control.
   *
   * This is (mostly) unique for each particular remote control manufactured.
   * (I mean, they have probably manufactured more than 65,536 remote controls,
   * so there are probably duplicates out there, but it is unlikely any
   * household will actually get two remotes with the same remote ID).
   *
   * For pairing remote controls with lights, the light should have a list of
   * pairing records, where each pairing record contains a remote ID and a
   * group ID.  Commands should be acted on if either the remote ID and group
   * ID match a pairing record, or if the remote ID matches and the group ID
   * in the event is 0.  Group ID 0 means "all groups of lights controlled by
   * this remote".
   */
  uint16_t remote_id = 0;

  /** The ID of the group of lights that the command is sent to.
   *
   * Or 0 for "all groups of lights controlled by this remote".
   */
  uint8_t group_id = 0;

  /** The command. */
  MiLightRemoteCommand command = MiLightRemoteCommand::UNKNOWN;

  /** The argument for the command.
   *
   * Meaning of this argument depends on the command.
   * For some commands it is not used.
   *
   * See the documentation for each command in MiLightRemoteCommand for details.
   */
  float arg1 = 0;

  MiLightRemoteEvent() = default;
  MiLightRemoteEvent(const MiLightRemoteEvent &other) = default;
  bool operator==(const MiLightRemoteEvent &other) const = default;
  MiLightRemoteEvent &operator=(const MiLightRemoteEvent &other) = default;
};

/**
 * Interface for listeners that receives decoded MiLight remote commands.
 *
 * This receives fully parsed and interpreted commands.
 *
 * (If you want to receive raw command data, consider using
 * MiLightRxRadioEventHandler instead).
 */
class MiLightRxRemoteEventHandler {
 public:
  /**
   * Called when a MiLight remote event happens.
   *
   * This is only called from the loop() method of a component, so it has the
   * same restrictions as that method.  It is guaranteed to be only called
   * from a single thread - the thread that does loop() calls.
   */
  virtual void milight_remote_event_handler(MiLightRemoteEvent const &event) = 0;
};

/**
 * Interface for sources that sends MiLight decoded remote commands to listeners.
 *
 * This somehow generates fully parsed and interpreted commands, and passes
 * them to the registered listeners.
 *
 * This interface allows for event listeners to be registered.
 */
class MiLightRxRemoteEventSource {
 public:
  /**
   * Add a listener to receive MiLight remote events.
   *
   * The listener will receive all future events.
   * The listener must not be NULL.
   *
   * It is guaranteed to be only called from a single thread.  (The single
   * thread that does setup, and then the single thread that does loop()
   * calls).
   */
  virtual void add_remote_event_listener(MiLightRxRemoteEventHandler *listener) = 0;
};

}  // namespace milight_rx
}  // namespace esphome
