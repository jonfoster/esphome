#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "NRF24L01Comms.h"

namespace esphome {
namespace milight_rx {

static const char *TAG = "milight_rx_radio.NRF24L01Comms";

// SPI Commands
static constexpr uint8_t R_REGISTER = 0x00;
static constexpr uint8_t W_REGISTER = 0x20;
static constexpr uint8_t R_RX_PAYLOAD = 0x61;
static constexpr uint8_t FLUSH_TX = 0xE1;
static constexpr uint8_t FLUSH_RX = 0xE2;
static constexpr uint8_t RF24_NOP = 0xFF;

void NRF24L01Comms::setup(esphome::spi::SPIDelegate *spi, esphome::GPIOPin *ce_pin, esphome::GPIOPin *irq_pin) {
  m_spi = spi;
  m_ce_pin = ce_pin;
  m_irq_pin = irq_pin;

  m_ce_pin->setup();
  m_ce_pin->pin_mode(gpio::FLAG_OUTPUT);
  m_irq_pin->setup();
  m_irq_pin->pin_mode(gpio::FLAG_INPUT);

  m_ce_pin->digital_write(false);
}

uint8_t NRF24L01Comms::read_register(uint8_t address) {
  uint8_t result;

  m_spi->begin_transaction();
  m_spi->transfer(R_REGISTER | address);
  result = m_spi->transfer(0xff);
  m_spi->end_transaction();

  return result;
}

void NRF24L01Comms::write_register(uint8_t address, uint8_t value) {
  ESP_LOGD(TAG, "write_register(%02x,%02x)", address, value);

  m_spi->begin_transaction();
  m_spi->transfer(W_REGISTER | address);
  m_spi->transfer(value);
  m_spi->end_transaction();
}

void NRF24L01Comms::write_register(uint8_t address, const uint8_t *value, uint8_t length) {
  ESP_LOGD(TAG, "write_register(%02x, len=%d)", address, length);
  m_spi->begin_transaction();
  m_spi->transfer(W_REGISTER | address);
  m_spi->write_array(value, length);
  m_spi->end_transaction();
}

uint8_t NRF24L01Comms::write_command_only(uint8_t cmd) {
  m_spi->begin_transaction();
  uint8_t status = m_spi->transfer(cmd);
  m_spi->end_transaction();

  return status;
}

uint8_t NRF24L01Comms::get_status() { return write_command_only(RF24_NOP); }

void NRF24L01Comms::flush_rx() {
  ESP_LOGD(TAG, "flush_rx()");
  write_command_only(FLUSH_RX);
}

void NRF24L01Comms::flush_tx() {
  ESP_LOGD(TAG, "flush_tx()");
  write_command_only(FLUSH_TX);
}

void NRF24L01Comms::read_rx_packet(uint8_t *packet, uint8_t length) {
  ESP_LOGD(TAG, "read_rx_packet(len=%d)", length);
  m_spi->begin_transaction();
  m_spi->transfer(R_RX_PAYLOAD);
  for (uint8_t i = 0; i < length; ++i) {
    packet[i] = m_spi->transfer(0xFF);
  }
  m_spi->end_transaction();
}

}  // namespace milight_rx
}  // namespace esphome