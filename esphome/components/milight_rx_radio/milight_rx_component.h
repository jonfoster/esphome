#pragma once

#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/spi/spi.h"
#include "milight_radio_driver.h"
#include "NRF24L01_comms.h"
#include "esphome/components/milight_rx/milight_rx_radio_event.h"
#include "esphome/components/milight_rx/milight_rx_remote_event.h"
#include <vector>

namespace esphome {
namespace milight_rx {
namespace radio {

/**
 * The ESP Home Component that uses an NRF24L01+ radio to receive
 * MiLight remote commands.
 *
 * This doesn't directly do anything with the received commands, but you can
 * register listeners to receive the parsed Milight commands and do useful
 * things with them.  For example, the milight_rx_filter component can be used
 * to filter only commands from remotes that are paired with a particular light,
 * and then the milight_rx_target component can be used to actually control the
 * light based on the received commands.  To aid debugging, there is also a
 * milight_rx_remote_debug component that can be used to log the decoded
 * packets to Home Assistant.
 *
 * Also allows registering listeners to receive the raw radio packets, which
 * can be useful for debugging or when adding support for new remotes.
 * To aid debugging, there is a  milight_rx_radio_debug component that can be
 * used to log the raw radio packets to Home Assistant.
 *
 * The NRF24L01+ is connected over SPI, including a mandatory CS (Chip Select)
 * pin.  There is also a mandatory GPIO output pin used for the CE (Chip Enable)
 * pin.  There is also an optional, but strongly recommended, GPIO input pin
 * used for the IRQ (Interrupt Request) pin.  Not having an IRQ pin will make
 * the code fall back to polling the radio chip over SPI, which is wasteful
 * compared to just reading the IRQ pin.
 *
 * This component is normally used to receive with a single radio configuration.
 * The radio configurations are numbered from 0 to 15 inclusive.
 * Pick the one that works best for your remotes, but the default configuration
 * of 0 works for the more modern remotes.  If you want to receive on multiple
 * radio configurations, then use multiple physical NRF24L01+ radio modules,
 * and multiple instances of this component.
 *
 * This component can also be used to scan through all available radio channels.
 * This can be useful when setting up a system, to find out which radio channel
 * your remotes are using.  To do this, set the config_index to -1, and set the
 * scan_time_millis to the time you want to spend scanning each radio channel
 * in milliseconds.  This mode is not inteded for normal use, it is only to help
 * you set up your system.
 */
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
  void handle_raw_packet(const uint8_t *packet_buf, unsigned packet_length);
  void advance_scan_if_needed();

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
