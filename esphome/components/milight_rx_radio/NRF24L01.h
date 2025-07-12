#pragma once

#include <stdint.h>
#include "esphome/components/spi/spi.h"
#include "esphome/core/gpio.h"
#include "NRF24L01Comms.h"
#include "NRF24L01RadioConfig.h"

namespace esphome {
namespace milight_rx {

/**
 * High level communications with an NRF24L01+ chip.
 *
 * This driver only supports recieving packets, not transmitting.
 *
 * There are various other restrictions on the supported configurations.
 * See the NRF24L01RadioConfig documentation for details.
 *
 * Note that the IRQ pin is optional, but recommended.   Without an IRQ pin,
 * this code will have to poll for status over SPI on every call to read(),
 * which is slower than just checking a built-in ESP32 GPIO that is connected
 * to the IRQ pin.  (If you run out of pins and have to use a GPIO expander,
 * then it's likely that polling the NRF24L01+ chip is quicker than using the
 * GPIO expander).
 */
class NRF24L01 {
 private:
  NRF24L01Comms comms_;

  /** Fixed size of packet payloads. */
  uint8_t payload_size_ = 0;

  bool force_fifo_check_ = false;

 public:
  /**
   * Set up this object.
   *
   * Must be called once, before any other method is called.
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
   * @param config The radio configuration to use.
   *
   * @return True on success, false on error.  Errors can be an invalid
   *     `config` passed in, or unable to communicate with the radio hardware.
   */
  bool setup(esphome::spi::SPIDelegate *spi, esphome::GPIOPin *ce_pin, esphome::GPIOPin *irq_pin,
             NRF24L01RadioConfig const &config);

  /**
   * Change the configuration.
   *
   * Stops the radio, discards any received packets that have not been
   * returned yet, sets the new configuration, and starts receiving packets.
   *
   * If the configuration is invalid, just returns false without doing
   * anything else.
   *
   * @param config The radio configuration to use.
   * @return True on success, or false if the configuration is invalid.
   */
  bool set_configuration(NRF24L01RadioConfig const &config);

  /**
   * Try to read a packet from the radio.
   *
   * This is intended to be called frequently, polling for new packets.
   *
   * If no packet is available, will return false.
   * If a packet is available, then the payload of that packet will be stored
   * into the provided buffer and this function will return true.
   *
   * The length of the returned packet payload is given by the active
   * NRF24L01RadioConfig::payload_size, as passed to the last call to
   * begin() or set_configuration().
   *
   * The buffer length passed into this function is only intended to prevent
   * accidental buffer overflows.  The length passed in should be greater
   * than or equal to the configured payload size; if not then this function
   * will just return false without doing anything.
   *
   * The radio has an on-chip buffer for 3 packets.  If you don't call read()
   * often enough, that can cause that buffer to be full when a new packet
   * arrives, which will cause that packet to be lost.  Everything else will
   * continue to work.
   *
   * @param[out] packet_buf The buffer to read the packet payload into.
   *     If this function returns true, then the first `payload_size` bytes
   *     will be set to the packet payload.
   * @param length The allocated length of `packet_buf`.  If less than
   *     `payload_size`, this function will always return false
   *     (without even checking if a packet is available).
   * @return true if a packet was read.  False if no packet was available or
   *     if the passed `length` was smaller than `payload_size`.
   */
  bool read(uint8_t *packet_buf, uint8_t length);

 private:
  /**
   * Power up the chip.
   */
  void power_up();

  /**
   * Actually apply the configuration.
   *
   * Caller must check the config is valid before calling.
   *
   * @param config The configuration to apply.
   */
  void apply_configuration(NRF24L01RadioConfig const &config);
};

}  // namespace milight_rx
}  // namespace esphome