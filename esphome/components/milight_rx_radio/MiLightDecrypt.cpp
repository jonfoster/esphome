#include <stdint.h>
#include <string.h>
#include "MilightDecrypt.h"
#include "esphome/core/log.h"

namespace esphome {
namespace milight_rx {

static const char *TAG = "milight_rx_radio.decrypt";

// V2 packet format:
//  0. Key
//  1. request type
//  2. id 1
//  3. id 2
//  4. command
//  5. argument
//  6. sequence
//  7. group
//  8. checksum

static uint8_t const V2_OFFSETS[4][8] = {{0x45, 0x2B, 0x6D, 0xAF, 0x1A, 0x04, 0xAF, 0x61},
                                         {0x1F, 0xC9, 0x5F, 0x03, 0xE2, 0xD8, 0x04, 0x13},
                                         {0x14, 0xE3, 0x8A, 0x1D, 0xF0, 0x71, 0xDD, 0x38},
                                         {0x5C, 0x11, 0x2B, 0xF3, 0xD1, 0x42, 0x07, 0x64}};

static uint8_t milight_convert_key(uint8_t key) {
  uint8_t msn = (((key + 0x6Cu) & 0x70u) + 0x40u);
  uint8_t lsn = ((key + 4u) & 0x0Fu);
  return (msn | lsn) ^ 0x12u;
}

bool milight_decrypt(uint8_t *packet) {
  uint8_t key_part1 = milight_convert_key(packet[0]);
  uint8_t key_part2 = packet[0] & 3u;
  uint8_t const *offsets = V2_OFFSETS[key_part2];
  uint8_t key_part3 = (packet[0] + 0x2Cu) & 0x80u;
  uint8_t sum = key_part1;

  for (unsigned i = 1; i <= 7; i++) {
    uint8_t s2 = offsets[i - 1] ^ key_part3;
    uint8_t value = (packet[i] - s2) ^ key_part1;
    packet[i] = value;
    sum += value;
  }

  // 8th byte is encoded differently.
  packet[8] = ((packet[8] - offsets[7]) ^ key_part1) - 2u;

  if (packet[8] != sum) {
    ESP_LOGW(TAG, "checksum fail. Got=%02x Calc=%02x", packet[8], sum);
    return false;
  }
  return true;
}

}  // namespace milight_rx
}  // namespace esphome
