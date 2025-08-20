#pragma once

#include <stdint.h>
#include "esphome/components/spi/spi.h"
#include "esphome/core/gpio.h"

namespace esphome {
namespace milight_rx {

/**
 * Alias for spi::SPIDevice with the right template parameters
 * to communicate with a NRF24L01+ chip.
 *
 * You should derive your main ESPHome component/sensor from this,
 * and then in `setup()` you call spi_setup() to fill in the
 * `delegate_` parameter with the SPIDelegate object that should be
 * passed to the NRF24L01Comms::setup() function.
 *
 * You also need to add appropriate Python to configure your SPI connection.
 * Note that for this driver, the CS pin is required (this is the default in
 * the ESPHome SPI configuration).
 */
typedef spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW, spi::CLOCK_PHASE_LEADING,
                       spi::DATA_RATE_10MHZ>
    NRF24L01SPIDevice;

/**
 * Low level communications with an NRF24L01+ chip.
 *
 * This implements the low-level SPI protocol, such as reading and writing
 * registers, reading packets, etc.
 *
 * It also allows the IRQ pin (if connected) to be read, and the CE (Chip
 * Enable) pin to be controlled.
 *
 * Note that the IRQ pin is optional, but recommended.   Without an IRQ pin,
 * higher level code will have to poll for status over SPI, which is slower
 * than just checking the IRQ pin.
 */
class NRF24L01Comms {
 private:
  esphome::spi::SPIDelegate *m_spi = NULL;
  esphome::GPIOPin *m_ce_pin = NULL;
  esphome::GPIOPin *m_irq_pin = NULL;

  uint8_t write_command_only(uint8_t cmd);

 public:
  /**
   * Set up this object.
   *
   * Must be called once, before any other method is called.
   *
   * This initialises the CE pin to low.
   *
   * @param spi The SPI delegate to use.  Non-NULL.
   * @param ce_pin The CE (Chip Enable) pin to use.  This is a digital output.
   *     This function will do all the necessary setup.  Non-NULL.
   * @param irq_pin Optionally, the IRQ (Interrupt) pin to use.  This is a
   *     digital input.  This function will do all the necessary setup.
   *     May be NULL if the IRQ pin is not connected.
   */
  void setup(esphome::spi::SPIDelegate *spi, esphome::GPIOPin *ce_pin, esphome::GPIOPin *irq_pin);

  /**
   * Read a register from the chip.
   *
   * @param address The address of the register to read.
   * @return The value of the register
   */
  uint8_t read_register(uint8_t address);

  /**
   * Write a register on the chip.
   *
   * @param address The address of the register to read.
   * @param value The new value of the register
   */
  void write_register(uint8_t address, uint8_t value);

  /**
   * Write a multi-byte register on the chip.
   *
   * This is used for configuring the RF address, since those registers are
   * multiple bytes long.
   *
   * @param address The address of the register to read.
   * @param value The new value of the register
   * @param length The length of the value, in bytes.
   */
  void write_register(uint8_t address, const uint8_t *value, uint8_t length);

  /**
   * Read a recieved radio packet from the chip.
   *
   * @param[out] packet Buffer to hold the recieved radio packet.
   * @param length The length of the radio packet, in bytes.
   */
  void read_rx_packet(uint8_t *packet, uint8_t length);

  /**
   * Get the status byte from the chip.
   *
   * This sends a NOP commands to the chip, and reads the status byte that is
   * returned in parallel to the command byte being sent.
   *
   * @return The value of the chip's status register.
   */
  uint8_t get_status();

  /**
   * Flush any queued recieved packets that are in the chip's RX FIFO.
   */
  void flush_rx();

  /**
   * Flush any queued transmit packets that are in the chip's TX FIFO.
   */
  void flush_tx();

  /**
   * Check if an IRQ pin is configured.
   *
   * @return true if an IRQ pin was passed to setup(), false otherwise.
   */
  bool is_irq_pin_connected() { return m_irq_pin != NULL; }

  /**
   * Check if the IRQ pin is active.
   *
   * That is, this returns true if there's an interrupt pending that needs
   * to be handled.
   *
   * Caller must check is_irq_pin_connected() first.  Calling this when
   * is_irq_pin_connected() is false, is Undefined Behaviour.
   *
   * @return true if the IRQ pin is active (logic level low).
   *         false if the IRQ pin is not active (logic level high).
   */
  bool is_irq_pin_active() { return !m_irq_pin->digital_read(); }

  /**
   * Set the Chip Enabled pin.
   *
   * Despite the name, this doesn't enable/disable the entire NRF24L01+ chip,
   * just parts of the RF transmit/recieve circuits.  See the datasheet.
   *
   * @param new_level True for logic level high (enabled), or
   *                  False for logic level low (disabled).
   */
  void set_ce_pin(bool new_level) { m_ce_pin->digital_write(new_level); }
};

}  // namespace milight_rx
}  // namespace esphome
