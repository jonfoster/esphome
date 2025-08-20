#pragma once

#include <stdint.h>

namespace esphome {
namespace milight_rx {

/**
 * Radio Configuation for a NRF24L01+ chip.
 *
 * By "Radio" configuration, we mean how the chip communicates over the radio
 * channel.  This class has nothing to do with how the chip connects to the
 * main processor, that's a differet bit of configuration.
 *
 * This class only supports a subset of possible configurations, because this
 * driver only supports a subset of possible configurations.
 *
 * In particular, the limitations are:
 *
 *  * Recieve only.  No transmitting
 *  * Shockburst protocol only.  No Enhanced Shockburst protocol.  This means:
 *    * On-air protocol is the Shockburst format
 *    * Automatic acknowledgements are not supported
 *    * Payload length is fixed, it cannot be dynamic.
 *  * Data rate is fixed at 1Mbps.  No support for 2Mbps or 500kHz.
 *  * No CRC.  No support for 1-byte CRC or 2-byte CRC.
 *  * Only one RX pipe is supported.  This means only one address can be
 *    configured to be recieved.
 *
 * The ShockBurst packet format is:
 *
 *  * Preamble.  Length is 1 byte.  If the first bit in the address is 1, then
 *    the preamble is 10101010.  If the first bit in the address is 0, then
 *    the preamble is 01010101.
 *  * Address.  Length is 3-5 bytes, configurable with the `address_width` field.
 *    Only packets that match the address configured in the `address` field
 *    will be read, other packets will be ignored by the chip.
 *  * Payload.  Length is 1-32 bytes, configurable with the `payload_size` field.
 *  * Optional CRC.  Length is 0 to 2 bytes.  Not supported by this driver.
 *    This driver always configures the chip to have no CRC.
 *
 *  All fields are transmitted and received MSB first.
 */
struct NRF24L01RadioConfig {
  static constexpr uint8_t min_address_width = 3;
  static constexpr uint8_t max_address_width = 5;
  static constexpr uint8_t max_channel = 125;
  static constexpr uint8_t min_payload_size = 1;
  static constexpr uint8_t max_payload_size = 32;

  /**
   * The RF address.
   *
   * The `address_width` field gives the length in bytes.
   *
   * The chip datasheet has this Note:
   *
   *     Addresses where the level shifts only one time (that is, 000FFFFFFF)
   *     can often be detected in noise and can give a false detection, which
   *     may give a raised Packet Error Rate. Addresses as a continuation of
   *     the preamble (hi-low toggling) also raises the Packet Error Rate.
   */
  uint8_t address[max_address_width] = {};

  /**
   * The address width to use.
   *
   * Range `min_address_width` to `max_address_width` inclusive.
   */
  uint8_t address_width = 0;

  /**
   * The RF channel to use.
   *
   * The RF frequency used is `(2400 + channel)` MHz.
   *
   * Range 0 to `max_channel` inclusive.
   */
  uint8_t channel = 0;

  /**
   * The size of the RF payload.  In bytes.
   */
  uint8_t payload_size = 0;

  // Usual default constructors, assignment operators
  NRF24L01RadioConfig() = default;
  NRF24L01RadioConfig(const NRF24L01RadioConfig &) = default;
  NRF24L01RadioConfig(NRF24L01RadioConfig &&) = default;
  NRF24L01RadioConfig &operator=(const NRF24L01RadioConfig &) = default;
  NRF24L01RadioConfig &operator=(NRF24L01RadioConfig &&) = default;

  // Default equality operators
  bool operator==(const NRF24L01RadioConfig &) const = default;
  bool operator!=(const NRF24L01RadioConfig &) const = default;

  /**
   * Constructor that sets all parameters.
   */
  NRF24L01RadioConfig(uint8_t *address, uint8_t address_width, uint8_t channel, uint8_t payload_size)
      : channel(channel), payload_size(payload_size) {
    set_address(address, address_width);
  }

  /**
   * Set the address and address width.
   */
  void set_address(const uint8_t *new_address, uint8_t new_address_width);

  /**
   * Check if all parameters are within the valid ranges.
   *
   * This does not check the `address` field because no validation is
   * needed for that field.
   *
   * @return True if all parameters are within the valid ranges,
   *     false otherwise.
   */
  bool is_valid() const;
};

}  // namespace milight_rx
}  // namespace esphome
