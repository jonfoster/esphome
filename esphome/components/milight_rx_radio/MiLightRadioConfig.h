#pragma once

#include <stdint.h>
#include "NRF24L01RadioConfig.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace milight_rx {

class MiLightRadioConfig {
 private:
  constexpr MiLightRadioConfig(uint8_t preamble, uint32_t syncword, uint8_t trailer, uint8_t packet_length,
                               uint8_t channel0, uint8_t channel1, uint8_t channel2, bool encrypted)
      : packet_length(packet_length),
        channels{channel0, channel1, channel2},
        syncword_bytes{
            // precompute the syncword for the nRF24.  we include the fixed preamble and trailer in the
            // syncword to avoid needing to bitshift packets.  trailer is 4 bits, so the actual syncword
            // is no longer byte-aligned.
            reverse_bits((uint8_t) (((syncword >> 28) & 0x0F) | ((trailer << 4) & 0xF0))),
            reverse_bits((uint8_t) ((syncword >> 20) & 0xFF)),
            reverse_bits((uint8_t) ((syncword >> 12) & 0xFF)),
            reverse_bits((uint8_t) ((syncword >> 4) & 0xFF)),
            reverse_bits((uint8_t) (((syncword << 4) & 0xF0) | (preamble & 0x0F))),
        },
        encrypted(encrypted) {}

 public:
  static constexpr uint8_t MAX_PACKET_LENGTH = 9;

  static constexpr unsigned NUM_CHANNELS = 3;

  // We can set this to only one possible value.  This controls what
  // we set the "address" to, which roughly corresponds to the LT8900 syncword.
  //
  // The PL1167 packet is structured as follows (lengths in bits):
  //  Preamble ( 8) | Syncword (32) | Trailer ( 4) | Packet Len ( 8) | Packet (...)
  //
  // 5 -- Include the Trailer in the syncword.  Avoids us needing to bitshift packet data. The
  //      downside is that the Trailer is hardcoded and assumed based on received packets.
  //
  // In general, this should be set to 5 unless packets that should be showing up are
  // mysteriously not present.
  //
  // Support for other values has been removed.
  static constexpr uint8_t SYNCWORD_LENGTH = 5;

  NRF24L01RadioConfig make_radio_config(uint8_t rf_channel_index) const {
    NRF24L01RadioConfig radio_config;
    radio_config.set_address(syncword_bytes, MiLightRadioConfig::SYNCWORD_LENGTH);
    radio_config.channel = 2 + channels[rf_channel_index];
    // +1 to be able to buffer the length
    // +2 for CRC
    radio_config.payload_size = packet_length + 1u + 2u;

    return radio_config;
  }

  static constexpr uint8_t NUM_CONFIGS = 5;
  static const MiLightRadioConfig ALL_CONFIGS[NUM_CONFIGS];

  const uint8_t packet_length;
  const uint8_t channels[NUM_CHANNELS];
  const uint8_t syncword_bytes[SYNCWORD_LENGTH];
  const bool encrypted;
};

}  // namespace milight_rx
}  // namespace esphome