#pragma once

#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/spi/spi.h"
#include "MiLightRadioRxDriver.h"
#include "NRF24L01Comms.h"
#include "esphome/components/milight_rx/milight_rx_radio_event.h"
#include "esphome/components/milight_rx/milight_rx_remote_event.h"
#include <vector>

namespace esphome {
namespace milight_rx {
namespace radio {

class MiLightRxComponent : public Component,
                           public MiLightRxRadioEventSource,
                           public MiLightRxRemoteEventSource,
                           public NRF24L01SPIDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  void set_ce_pin(GPIOPin *pin) { ce_pin_ = pin; }
  void set_irq_pin(GPIOPin *pin) { irq_pin_ = pin; }

  void set_config_index(int index) { config_index_ = index; }
  void set_scan_time_millis(int time) { scan_time_millis_ = time; }

  void add_radio_event_listener(MiLightRxRadioEventHandler *listener) override {
    radio_packet_listeners_.push_back(listener);
  }

  void add_remote_event_listener(MiLightRxRemoteEventHandler *listener) override {
    remote_packet_listeners_.push_back(listener);
  }

 private:
  void try_to_read();
  void advance_scan_if_needed();
  void handle_raw_packet(const uint8_t *packet_buf, unsigned packet_length);

 protected:
  /** Chip enable pin, as configured in device configuration.
   * Must be non-NULL before setup() is called.
   */
  GPIOPin *ce_pin_ = NULL;
  /** IRQ pin, as configured in device configuration.
   * Optional, may be NULL if not connected.
   */
  GPIOPin *irq_pin_ = NULL;

  /** Radio channel selection, as configured in device configuration.
   *
   * Special value -1 means scan mode, where the component will scan through all
   * available radio channels.
   *
   * Otherwise, valid range is 0 to MiLightRadioRxDriver::NUM_CONFIGS-1
   * inclusive.
   */
  int config_index_ = -1;
  /** Duration to scan each radio channel, in milliseconds.
   * As configured in device configuration.
   * Ignored if config_index_ is not -1.
   */
  int scan_time_millis_ = 1000;

  /** Current radio channel that is being scanned.
   * Only used if config_index_ is -1.
   */
  int scan_current_config_index_ = 0;

  /** The timestamp for the scan to move on to the next radio channel.
   * Only used if config_index_ is -1.
   */
  uint32_t scan_next_step_time_ = 0;

  /** The actual MiLight radio receiver implementation.
   *
   * This code is deliberately split so MiLightRxComponent provides the ESPHome
   * Component interface & integration, and the actual radio handling and
   * transport protocol decoding is done by MiLightRadioRxDriver
   */
  MiLightRadioRxDriver impl_;

  /** List of registered radio packet listeners. */
  std::vector<MiLightRxRadioEventHandler *> radio_packet_listeners_;

  /** List of registered remote event listeners. */
  std::vector<MiLightRxRemoteEventHandler *> remote_packet_listeners_;
};

}  // namespace radio
}  // namespace milight_rx
}  // namespace esphome