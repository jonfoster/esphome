#include <string.h>
#include "NRF24L01_radio_config.h"

namespace esphome {
namespace milight_rx {

void NRF24L01RadioConfig::set_address(const uint8_t *new_address, uint8_t new_address_width) {
  address_width = new_address_width;
  if (new_address_width <= max_address_width) {
    memcpy(address, new_address, new_address_width);
  }
}

bool NRF24L01RadioConfig::is_valid() const {
  if (address_width < min_address_width || address_width > max_address_width || channel > max_channel ||
      payload_size < min_payload_size || payload_size > max_payload_size) {
    return false;
  }
  return true;
}

}  // namespace milight_rx
}  // namespace esphome
