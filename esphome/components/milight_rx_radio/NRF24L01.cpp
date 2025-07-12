#include <string.h>
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "NRF24L01.h"

namespace esphome {
namespace milight_rx {

static const char *TAG = "milight_rx_radio.NRF24L01";

// Register addresses per datasheet.
static constexpr uint8_t NRF_REG_CONFIG = 0x00;
static constexpr uint8_t NRF_REG_EN_AA = 0x01;
static constexpr uint8_t NRF_REG_EN_RXADDR = 0x02;
static constexpr uint8_t NRF_REG_SETUP_AW = 0x03;
static constexpr uint8_t NRF_REG_SETUP_RETR = 0x04;
static constexpr uint8_t NRF_REG_RF_CH = 0x05;
static constexpr uint8_t NRF_REG_RF_SETUP = 0x06;
static constexpr uint8_t NRF_REG_STATUS = 0x07;
static constexpr uint8_t NRF_REG_OBSERVE_TX = 0x08;
static constexpr uint8_t NRF_REG_RPD = 0x09;
static constexpr uint8_t NRF_REG_RX_ADDR_P0 = 0x0A;
static constexpr uint8_t NRF_REG_RX_ADDR_P1 = 0x0B;
static constexpr uint8_t NRF_REG_RX_ADDR_P2 = 0x0C;
static constexpr uint8_t NRF_REG_RX_ADDR_P3 = 0x0D;
static constexpr uint8_t NRF_REG_RX_ADDR_P4 = 0x0E;
static constexpr uint8_t NRF_REG_RX_ADDR_P5 = 0x0F;
static constexpr uint8_t NRF_REG_TX_ADDR = 0x10;
static constexpr uint8_t NRF_REG_RX_PW_P0 = 0x11;
static constexpr uint8_t NRF_REG_RX_PW_P1 = 0x12;
static constexpr uint8_t NRF_REG_RX_PW_P2 = 0x13;
static constexpr uint8_t NRF_REG_RX_PW_P3 = 0x14;
static constexpr uint8_t NRF_REG_RX_PW_P4 = 0x15;
static constexpr uint8_t NRF_REG_RX_PW_P5 = 0x16;
static constexpr uint8_t NRF_REG_FIFO_STATUS = 0x17;
static constexpr uint8_t NRF_REG_DYNPD = 0x1C;
static constexpr uint8_t NRF_REG_FEATURE = 0x1D;

bool NRF24L01::setup(esphome::spi::SPIDelegate *spi, esphome::GPIOPin *ce_pin, esphome::GPIOPin *irq_pin,
                     NRF24L01RadioConfig const &config) {
  comms_.setup(spi, ce_pin, irq_pin);

  if (!config.is_valid()) {
    ESP_LOGE(TAG, "Invalid config");
    return false;
  }

  // The datasheet says 100ms delay after power is applied to the chip.
  delay(100);

  // Ensure power is turned off by writing config register with
  // PWR_UP bit clear.
  comms_.write_register(NRF_REG_CONFIG, 0);

  // Set data rate 1Mbs (bits set to 00)
  //
  // Power amplifier is only used when transmitting, and this driver
  // is recieve-only.  So the power amplifier setting doesn't matter,
  // we just set it to the lowest power (bits set to 00).
  comms_.write_register(NRF_REG_RF_SETUP, 0);

  // disable enhanded features
  comms_.write_register(NRF_REG_FEATURE, 0);
  comms_.write_register(NRF_REG_DYNPD, 0);
  comms_.write_register(NRF_REG_EN_AA, 0);

  // We only use one RX pipe, which is pipe 1.
  comms_.write_register(NRF_REG_EN_RXADDR, 2);

  // Set config
  apply_configuration(config);

  // Check chip is connected
  if (comms_.read_register(NRF_REG_SETUP_AW) != config.address_width - 2) {
    ESP_LOGE(TAG, "Cannot read from chip");
    return false;
  }

  power_up();

  return true;
}

bool NRF24L01::set_configuration(NRF24L01RadioConfig const &config) {
  if (!config.is_valid()) {
    ESP_LOGE(TAG, "Invalid config");
    return false;
  }

  // Power down
  comms_.set_ce_pin(false);
  delay(1);
  comms_.write_register(NRF_REG_CONFIG, 0);

  // Set config
  apply_configuration(config);

  // Power up
  power_up();

  return true;
}

bool NRF24L01::read(uint8_t *packet_buf, uint8_t length) {
  if (length < payload_size_) {
    ESP_LOGE(TAG, "Read buf too small");
    return false;
  }

  if (force_fifo_check_) {
    // We just handled an interrupt in a previous call.
    // Need to check FIFO_STATUS to ensure we read ALL
    // available packets, can't assume there was only one.
    uint8_t fifo_status = comms_.read_register(NRF_REG_FIFO_STATUS);
    bool rx_data = (fifo_status & 1u) == 0u;
    if (!rx_data) {
      // Done handling that interrupt.
      ESP_LOGD(TAG, "IRQ done");
      force_fifo_check_ = false;
      return false;
    }
    ESP_LOGD(TAG, "IRQ more data");
  } else {
    // IRQ pin is optional, but recommended.
    // Without it we have to poll every time.
    if (comms_.is_irq_pin_connected()) {
      if (!comms_.is_irq_pin_active()) {
        // No data, because if there was data we would have got an IRQ.
        return false;
      }

      ESP_LOGD(TAG, "IRQ received");
    }

    uint8_t status = comms_.get_status();
    bool irq_rx_dr = ((status & 0b01000000u) != 0u);
    if (!irq_rx_dr) {
      // To avoid log spam, don't log if we're configured to always poll
      if (comms_.is_irq_pin_connected()) {
        ESP_LOGD(TAG, "Status: no DR");
      }
      return false;
    }
  }

  // Fetch the payload
  comms_.read_rx_packet(packet_buf, payload_size_);

  // Clear the only applicable interrupt flag
  comms_.write_register(NRF_REG_STATUS, 0b01000000);

  // Interrupt doesn't mean "one packet available", it means "AT LEAST one
  // packet available".  So on next read() call, have to check to see if
  // there's another packet available, regardless of state of interrupt pin.
  force_fifo_check_ = true;

  ESP_LOGD(TAG, "Got packet: %d bytes", payload_size_);

  return true;
}

void NRF24L01::apply_configuration(NRF24L01RadioConfig const &config) {
  payload_size_ = config.payload_size;

  comms_.write_register(NRF_REG_RX_PW_P1, config.payload_size);
  comms_.write_register(NRF_REG_SETUP_AW, config.address_width - 2);
  comms_.write_register(NRF_REG_RX_ADDR_P1, config.address, config.address_width);
  comms_.write_register(NRF_REG_RF_CH, config.channel);
}

void NRF24L01::power_up() {
  // Flush buffers
  comms_.flush_rx();
  comms_.flush_tx();

  // Acknowledge any interrupts.
  comms_.write_register(NRF_REG_STATUS, 0b01110000);

  // Clear interrupt-handling state in this driver.
  force_fifo_check_ = false;

  // Power up.
  // Sets the PWR_UP and PRIM_RX bits.
  comms_.write_register(NRF_REG_CONFIG, 3);

  // Delay of Tpd2stby to allow clock to start.
  // Actual delay needed depends on the board design, mainly the choice of
  // crystal.  It may be 1.5ms to 4.5ms.  Since we don't know what the
  // hardware is, we just use 5ms.
  delay(5);

  comms_.set_ce_pin(true);
}

}  // namespace milight_rx
}  // namespace esphome