#pragma once

#include <stdint.h>
#include "esphome/components/spi/spi.h"
#include "esphome/core/gpio.h"
#include "NRF24L01.h"
#include "MiLightRadioConfig.h"

namespace esphome {
namespace milight_rx {

/**
 * Driver to receive MiLight packets using the NRF24L01+ radio chip.
 *
 * This recieves the radio packets and decrypts them if necessary.
 * It does NOT try to parse the packets, that is left to the caller.
 * It just returns the rew bytes.  Depending on configuration, each packet
 * contains 6 or 7 bytes of payload data which is returned by this class.
 *
 * This class will check packet CRCs and checksums, and will also discard
 * repeated packets.  So it doesn't return all packets that it receives.
 *
 *
 */
class MiLightRadioRxDriver {
 public:
  MiLightRadioRxDriver() = default;

  /**
   * Set up this object.
   *
   * Must be called once.
   *
   * Caller should call set_config_index() before calling this method
   * (although if that isn't called, the default config index 0 will be used).
   *
   * This sets up the hardware, and starts receiving packets.
   *
   * This does check that it can communicate with the radio.  Note that this
   * does not check the CE or IRQ pins are correctly connected, just that the
   * radio chip has power and the SPI pins (MISO, MOSI, SCK, and CE) are
   * correctly connected.
   *
   * @param spi The SPI delegate to use.  Non-NULL.
   * @param ce_pin The CE (Chip Enable) pin to use.  This is a digital output.
   *     This function will do all the necessary setup.  Non-NULL.
   * @param irq_pin Optionally, the IRQ (Interrupt) pin to use.  This is a
   *     digital input.  This function will do all the necessary setup.
   *     May be NULL if the IRQ pin is not connected.
   *
   * @return True on success, false on error.  Errors are likely to be unable
   *     to communicate with the radio hardware.
   */
  bool setup(esphome::spi::SPIDelegate *spi, esphome::GPIOPin *ce_pin, esphome::GPIOPin *irq_pin);

  /**
   * Set the index of the radio configuration to use.
   *
   * Valid range is 0 to NUM_CONFIGS-1 inclusive.
   *
   * If called before setup(), then this will just store the index so that
   * setup() can use it.
   *
   * If called after a successful call to setup(), with a different config
   * index from the current configuration, then this will reconfigure the
   * radio to use the new configuration.  In that case, it will stop the
   * radio, discards any received packets that have not been returned yet,
   * set the new configuration, and start receiving packets.
   *
   * If called after a failed call to setup(), then this will do nothing and
   * return false.
   *
   * @param cfg The index of the configuration to use.
   * @return True on success, false on error.
   */
  bool set_config_index(int cfg);

  /**
   * Get the index of the currently set radio configuration.
   *
   * @return The index of the current configuration.
   */
  int get_config_index() { return config_index_; }

  /**
   * Try to read a packet from the radio.
   *
   * This is intended to be called frequently, polling for new packets.
   *
   * If no packet is available, will return false.
   * If a packet is available, then the payload of that packet will be stored
   * into the provided buffer and this function will return true.
   *
   * The radio has an on-chip buffer for 3 packets.  If you don't call read()
   * often enough, that can cause that buffer to be full when a new packet
   * arrives, which will cause that packet to be lost.  Everything else will
   * continue to work.
   *
   * For encrypted packets, this decrypts the packet and returns the decrypted
   * payload.
   *
   * setup() should be called successfully before this function is called.
   * If not, this function will return false.
   *
   * @param[out] data The buffer to read the packet payload into.
   *     If this function returns true, then the first `data_length` bytes
   *     will be set to the packet payload.
   * @param[inout] data_length On call, the allocated length of `packet_buf`.
   *     On return, the actual length of the packet payload that was written
   *     into `data`, or 0 if this function returns false.  If the initial
   *     value is too small, this function will always return false
   *     (without even checking if a packet is available).
   * @return true if a packet was read.  False if no packet was available or
   *     if the passed `data_length` was too small.
   */
  bool read(uint8_t data[], unsigned &data_length);

  /**
   * The number of radio configurations available.
   */
  static constexpr unsigned NUM_CONFIGS = MiLightRadioConfig::NUM_CONFIGS * MiLightRadioConfig::NUM_CHANNELS;

 private:
  /**
   * Check the packet CRC.
   *
   * @param data The packet data to check.
   * @return True if the CRC matches, false if it does not.
   */
  bool check_packet_crc(const uint8_t data[], unsigned data_length);

  /**
   * Result of a read_once() call.
   */
  enum class ReadResult { NO_DATA, TRY_AGAIN, SUCCESS };

  /**
   * Like read(), but only reads at most a single packet from the hardware.
   *
   * If a packet is read but shouldn't be returned to the caller (e.g
   * CRC fail or repeated packet), then this function will return TRY_AGAIN.
   * When TRY_AGAIN is returned, `data_length` is unmodified.
   *
   * NO_DATA means read() should return false.
   * SUCCESS means read() should return true.
   */
  ReadResult read_once(uint8_t data[], unsigned &data_length);

  enum class StartupState { STARTED, NOT_STARTED, BROKEN };

  StartupState startup_state_ = StartupState::NOT_STARTED;

  /**
   * The maximum number of packets that read() will read in a single call.
   *
   * If read() reads and discards this many packets, it will return false
   * without trying to read any more.
   *
   * This is to prevent read() from running for too long, which can
   * happen if the radio is receiving packets faster than we can process them.
   */
  static constexpr unsigned max_read_attempts = 3;

  NRF24L01 radio_;

  /**
   * The current radio configuration index.
   *
   * See set_config_index() and get_config_index().
   */
  int config_index_ = 0;

  /**
   * The current radio configuration.
   *
   * This is set by set_config_index().
   *
   * This config object specifies 3 different radio channels,
   * so rf_channel_index_ is used to identify which of those
   * channels is currently configured.
   */
  const MiLightRadioConfig *config_ = &MiLightRadioConfig::ALL_CONFIGS[0];

  /**
   * The RF channel index withing the current radio configuration.
   *
   * This is set by set_config_index().
   */
  uint8_t rf_channel_index_ = 0;

  bool have_last_crcerr_packet_ = false;
  uint8_t last_crcerr_packet_buf_[MiLightRadioConfig::MAX_PACKET_LENGTH + 3u] = {0};

  bool have_last_packet_ = false;
  uint8_t last_packet_buf_[MiLightRadioConfig::MAX_PACKET_LENGTH + 3u] = {0};
  unsigned last_packet_repeats_ = 0;
};

}  // namespace milight_rx
}  // namespace esphome