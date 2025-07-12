#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "MiLightRadioRxDriver.h"
#include "MiLightRadioConfig.h"
#include "MiLightDecrypt.h"

namespace esphome {
namespace milight_rx {

static uint16_t calc_crc(const uint8_t *data, unsigned data_length);
static bool try_to_fix_crc(uint8_t *data, unsigned data_length);

static const char *TAG = "milight_rx_radio.driver";

bool MiLightRadioRxDriver::setup(esphome::spi::SPIDelegate *spi, esphome::GPIOPin *ce_pin, esphome::GPIOPin *irq_pin) {
  if (startup_state_ != StartupState::NOT_STARTED) {
    ESP_LOGE(TAG, "setup() called multiple times");
    return false;
  }

  NRF24L01RadioConfig radio_config = config_->make_radio_config(rf_channel_index_);
  if (!radio_.setup(spi, ce_pin, irq_pin, radio_config)) {
    startup_state_ = StartupState::BROKEN;
    return false;
  }

  startup_state_ = StartupState::STARTED;
  return true;
}

bool MiLightRadioRxDriver::set_config_index(int cfg) {
  if (cfg < 0 || cfg >= NUM_CONFIGS) {
    ESP_LOGE(TAG, "Bad config index");
    return false;
  }
  if (startup_state_ == StartupState::BROKEN) {
    return false;
  }
  if (cfg == config_index_) {
    // Already set to this config.
    return true;
  }

  config_index_ = cfg;
  config_ = &MiLightRadioConfig::ALL_CONFIGS[cfg / 3];
  rf_channel_index_ = cfg % 3;

  if (startup_state_ != StartupState::STARTED) {
    // startup_state_ == NOT_STARTED
    return true;
  }

  // Running, so need to change the radio configuration.
  NRF24L01RadioConfig radio_config = config_->make_radio_config(rf_channel_index_);
  if (!radio_.set_configuration(radio_config)) {
    // Now have a mismatch between the radio configuration and config_,
    // so read() will not work.
    startup_state_ = StartupState::BROKEN;
    return false;
  }

  have_last_packet_ = false;

  return true;
}

bool MiLightRadioRxDriver::check_packet_crc(const uint8_t data[], unsigned data_length) {
  uint16_t calculated_crc = calc_crc(data, data_length - 2u);
  uint16_t crc_field = (data[data_length - 1u] << 8) | data[data_length - 2u];

  if (crc_field != calculated_crc) {
    ESP_LOGD(TAG, "CRC field %04x not calc'd %04x", crc_field, calculated_crc);
    return false;
  }

  return true;
}

MiLightRadioRxDriver::ReadResult MiLightRadioRxDriver::read_once(uint8_t data[], unsigned &data_length) {
  uint8_t packet_buffer[MiLightRadioConfig::MAX_PACKET_LENGTH + 3u];
  uint8_t packet_len = config_->packet_length + 3u;
  uint8_t *packet_start = packet_buffer;

  if (!radio_.read(packet_start, packet_len)) {
    data_length = 0;
    return ReadResult::NO_DATA;
  }

  for (int inp = 0; inp < packet_len; inp++) {
    packet_start[inp] = reverse_bits(packet_start[inp]);
  }

  // Check CRC matches.  If not, then check if there's a 1-bit error which
  // we can fix.  (This seems to happen often in my testing).
  bool fixed_packet_error = false;
  if (!check_packet_crc(packet_start, packet_len)) {
    if (have_last_crcerr_packet_ && memcmp(last_crcerr_packet_buf_, packet_start, packet_len) == 0) {
      // This is a repeated CRC error packet.
      // Don't waste time trying to fix the same CRC error, when we'd just
      // discard this packet as a duplicate anyway.
      ESP_LOGD(TAG, "Repeated CRC error packet");
      return ReadResult::TRY_AGAIN;
    }
    memcpy(last_crcerr_packet_buf_, packet_start, packet_len);
    have_last_crcerr_packet_ = true;

    if (!try_to_fix_crc(packet_start, packet_len)) {
      return ReadResult::TRY_AGAIN;
    }
    fixed_packet_error = true;
  }

  if (have_last_packet_ && memcmp(last_packet_buf_, packet_start, packet_len) == 0) {
    // This is a repeated packet.
    ESP_LOGD(TAG, "Repeated packet");
    return ReadResult::TRY_AGAIN;
  }
  have_last_packet_ = true;
  memcpy(last_packet_buf_, packet_start, packet_len);
  if (!fixed_packet_error) {
    // Last CRC error packet may have been fixed to a DIFFERENT value, so would
    // no longer be a repeat.
    have_last_crcerr_packet_ = false;
  }

  // ESP_LOGD(TAG, "RX len=%u:", packet_len);
  // ESP_LOG_BUFFER_HEX_LEVEL(TAG, packet_start, packet_len, ESP_LOG_DEBUG);

  // Remove CRC from the end of the packet.
  packet_len -= 2;

  // Check length field in packet (first byte of packet)
  // matches expected length.
  if (packet_start[0] != config_->packet_length) {
    ESP_LOGI(TAG, "Length field %u not expected %u", packet_start[0], config_->packet_length);
    return ReadResult::TRY_AGAIN;
  }
  // Length field is no longer interesting.
  packet_start++;
  packet_len--;

  if (config_->encrypted) {
    if (!milight_decrypt(packet_start)) {
      return ReadResult::TRY_AGAIN;
    }
    packet_len -= 2;
    packet_start++;
  }

  ESP_LOGD(TAG, "Good packet len=%u", packet_len);

  memcpy(data, packet_start, packet_len);
  data_length = packet_len;

  return ReadResult::SUCCESS;
}

bool MiLightRadioRxDriver::read(uint8_t data[], unsigned &data_length) {
  if (startup_state_ != StartupState::STARTED) {
    return false;
  }

  if (data_length < config_->packet_length) {
    ESP_LOGE(TAG, "Buf too small");
    return false;
  }

  // There is a limit to how long a loop() funtion should/can run for.
  // So we limit how long this read() function runs for, by limiting the
  // number of read attempts.  This is particularly important if lots of
  // logging is enabled, because RF packets may come in faster than we can
  // process them and this function may loop for a very long time (375ms seen).
  for (unsigned i = 0; i < max_read_attempts; ++i) {
    ReadResult state = read_once(data, data_length);
    if (state == ReadResult::NO_DATA) {
      // Chip does not have any data for us.
      return false;
    } else if (state == ReadResult::SUCCESS) {
      // We read a packet that we want to return to caller.
      return true;
    }
    // Else we read a packet but don't want to return it to the caller for
    // some reason.  (E.g. CRC fail or repeated packet).  So we read the next
    // packet.
  }

  // We read max_read_attempts packets and decided not to return any of them
  // to the caller.
  //
  // Note that read_once() doesn't set data_length in this case, so we do that
  // here.
  data_length = 0;
  return false;
}

#define CRC_POLY 0x8408

/* // Code to generate the CRC table below.
 *
 * #define CRC_POLY 0x8408
 * #include <stdio.h>
 * #include <stdint.h>
 *
 * int main()
 * {
 *   for (uint32_t initial_state = 0; initial_state <= UINT8_MAX; initial_state++) {
 *     uint16_t state = initial_state;
 *     for (int j = 0; j < 8; j++) {
 *       if (state & 0x01) {
 *         state = (state >> 1) ^ CRC_POLY;
 *       } else {
 *         state = state >> 1;
 *       }
 *     }
 *     printf("0x%04x, ", state);
 *     if ((initial_state & 7) == 7) printf("\n");
 *   }
 *   return 0;
 * }
 */

static const uint16_t crc_table[256] = {
    0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf, 0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5,
    0xe97e, 0xf8f7, 0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e, 0x9cc9, 0x8d40, 0xbfdb, 0xae52,
    0xdaed, 0xcb64, 0xf9ff, 0xe876, 0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd, 0xad4a, 0xbcc3,
    0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5, 0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
    0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974, 0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9,
    0x2732, 0x36bb, 0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3, 0x5285, 0x430c, 0x7197, 0x601e,
    0x14a1, 0x0528, 0x37b3, 0x263a, 0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72, 0x6306, 0x728f,
    0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9, 0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
    0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738, 0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862,
    0x9af9, 0x8b70, 0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7, 0x0840, 0x19c9, 0x2b52, 0x3adb,
    0x4e64, 0x5fed, 0x6d76, 0x7cff, 0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036, 0x18c1, 0x0948,
    0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e, 0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
    0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd, 0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226,
    0xd0bd, 0xc134, 0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c, 0xc60c, 0xd785, 0xe51e, 0xf497,
    0x8028, 0x91a1, 0xa33a, 0xb2b3, 0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb, 0xd68d, 0xc704,
    0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232, 0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
    0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1, 0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb,
    0x0e70, 0x1ff9, 0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330, 0x7bc7, 0x6a4e, 0x58d5, 0x495c,
    0x3de3, 0x2c6a, 0x1ef1, 0x0f78,
};

static uint16_t calc_crc(const uint8_t *data, unsigned data_length) {
  uint16_t state = 0;
  for (unsigned i = 0; i < data_length; i++) {
    state ^= data[i];
#if 1
    // Faster code that needs a 512-byte lookup table.
    state = (state >> 8) ^ crc_table[state & 0xFF];
#else
    // Slower, smaller code.
    for (int j = 0; j < 8; j++) {
      if (state & 0x01) {
        state = (state >> 1) ^ CRC_POLY;
      } else {
        state = state >> 1;
      }
    }
#endif
  }
  return state;
}

static bool try_to_fix_crc(uint8_t *data, unsigned data_length) {
  int first_fix = -1;
  unsigned len_bits = data_length * 8;
  for (unsigned i = 0; i < len_bits; i++) {
    uint8_t byte = i / 8u;
    uint8_t bit = (1u << (i % 8u));

    // Try flipping the bit.
    data[byte] ^= bit;

    uint16_t crc_field = (data[data_length - 1u] << 8) | data[data_length - 2u];
    uint16_t calc_crc_value = calc_crc(data, data_length - 2u);

    // Restore the bit.
    data[byte] ^= bit;

    if (crc_field == calc_crc_value) {
      if (first_fix >= 0) {
        uint8_t other_byte = (unsigned) first_fix / 8u;
        uint8_t other_bit = (1u << ((unsigned) first_fix % 8u));
        ESP_LOGW(
            TAG,
            "Not fixing CRC error: Multiple possible fixes found, including byte %u bit 0x%02x and byte %u bit 0x%02x",
            byte, bit, other_byte, other_bit);

        return false;  // Multiple fixes found, so don't fix.
      }

      first_fix = i;
    }
  }

  if (first_fix < 0) {
    ESP_LOGW(TAG, "Could not fix CRC error");
    return false;  // No fix found.
  }

  // Fix found, so apply it.
  unsigned byte = (unsigned) first_fix / 8u;
  unsigned bit = (1u << ((unsigned) first_fix % 8u));
  data[byte] ^= bit;
  ESP_LOGW(TAG, "Fixed 1-bit packet corruption by flipping byte %u bit 0x%02x", byte, bit);

  return true;
}

}  // namespace milight_rx
}  // namespace esphome