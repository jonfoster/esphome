#pragma once
#include <stdint.h>

namespace esphome {
namespace milight_rx {

/**
 * Decrypt a MiLight V2 packet.
 *
 * The encrypted packet is 9 bytes long.  The first byte identifies the key
 * used, the last byte is the encrypted checksum, and the bytes in between are
 * encrypted data.
 *
 * The decrypted packet has 7 payload bytes.
 *
 * This function both decrypts and checks the checksum byte, so the caller can
 * ignore the checksum byte and just use the decrypted data.
 *
 * @param[inout] packet On input, the RF packet payload.  9 bytes long.
 *     On return, the first byte will be unchanged and the following 8 bytes
 *     will have been decrypted in-place.
 * @return True if the checksum byte was correct, false if it was not.
 *     Note that the data was decrypted either way, it's just that if the
 *     checksum is wrong, you might not want to rely on that data.
 */
bool milight_decrypt(uint8_t *packet);

}  // namespace milight_rx
}  // namespace esphome
