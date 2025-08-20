#pragma once

#include <stdint.h>

namespace esphome {
namespace milight_rx {

/**
 * Interface for listeners that receives raw MiLight remote commands.
 *
 * This receives raw commands from the radio.  However, V2 commands will have
 * been decrypted.
 *
 * This is useful for debugging, or to understand the raw commands that are
 * being sent by a new remote control so you can implement a decoder for them.
 *
 * Consider using MiLightRxRemoteEventHandler, which receives fully parsed
 * and interpreted commands, instead of this interface.
 */
class MiLightRxRadioEventHandler {
 public:
  /**
   * Called when a raw Milight command is received over the radio.
   *
   * The packet data is in two parts.
   *
   * The first byte holds the index of the radio configuration that was used
   * to receive the packet.
   *
   * The rest of the buffer is the raw packet data from the radio.  Except that
   * V2 commands will have been decrypted.  This will be 6 or 7 bytes long.
   *
   * So the total packet length, as reported by packet_length, will be 7 or 8
   * bytes.
   *
   * This is only called from the loop() method of a component, so it has the
   * same restrictions as that method.  It is guaranteed to be only called
   * from a single thread.
   *
   * @param packet The packet data as described above.  Will be non-NULL.
   *               Length in bytes is given by packet_length.
   *               This pointer is only valid for the duration of this call.
   * @param packet_length The length of the packet data, in bytes.
   */
  virtual void milight_radio_event_handler(const uint8_t *packet, unsigned packet_length) = 0;
};

/**
 * Interface for sources that sends raw MiLight remote commands to listeners.
 *
 * This somehow generates raw commands, and passes them to the registered
 * listeners.
 *
 * This interface allows for event listeners to be registered.
 */
class MiLightRxRadioEventSource {
 public:
  /**
   * Add a listener to receive raw MiLight remote commands.
   *
   * The listener will receive all future events.
   * The listener must not be NULL.
   *
   * It is guaranteed to be only called from a single thread.  (The single
   * thread that does setup, and then the single thread that does loop()
   * calls).
   */
  virtual void add_radio_event_listener(MiLightRxRadioEventHandler *listener) = 0;
};

}  // namespace milight_rx
}  // namespace esphome
