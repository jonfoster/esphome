#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/milight_rx/milight_rx_remote_event.h"
#include <vector>
#include <set>

namespace esphome {
namespace milight_rx {
namespace filter {

struct MiLightRemoteIdentifier {
 private:
  MiLightRemoteType remote_type = MiLightRemoteType::UNKNOWN;
  uint16_t remote_id = 0;  // Remote ID, used to identify the remote.
  uint8_t group_id = 0;    // Group ID, used to identify the group of lights.

 public:
  bool operator==(const MiLightRemoteIdentifier &other) const = default;
  bool operator<(const MiLightRemoteIdentifier &other) const {
    if (this->remote_type != other.remote_type) {
      return this->remote_type < other.remote_type;
    }
    if (this->remote_id != other.remote_id) {
      return this->remote_id < other.remote_id;
    }
    return this->group_id < other.group_id;
  }

  MiLightRemoteIdentifier(const MiLightRemoteEvent &event)
      : remote_type(event.remote_type), remote_id(event.remote_id), group_id(event.group_id) {}
  MiLightRemoteIdentifier(MiLightRemoteType remote_type, uint16_t remote_id, uint8_t group_id)
      : remote_type(remote_type), remote_id(remote_id), group_id(group_id) {}

  MiLightRemoteIdentifier get_all_groups_address() const {
    // Return a generic identifier with group_id set to 0.
    return MiLightRemoteIdentifier(this->remote_type, this->remote_id, 0);
  }

  MiLightRemoteType get_remote_type() const { return this->remote_type; }
  uint16_t get_remote_id() const { return this->remote_id; }
  uint8_t get_group_id() const { return this->group_id; }

  bool is_all_groups() const { return this->group_id == 0; }
};

class MiLightRxFilterComponent : public Component,
                                 public MiLightRxRemoteEventHandler,
                                 public MiLightRxRemoteEventSource {
 public:
  void dump_config() override;

  void add_remote_event_listener(MiLightRxRemoteEventHandler *listener) override {
    this->listeners_.push_back(listener);
  }

  void milight_remote_event_handler(MiLightRemoteEvent const &event) override;

  void statically_pair_remote(MiLightRemoteIdentifier const &remote) {
    this->statically_paired_remotes_.push_back(remote);
  }

  void statically_pair_remote(MiLightRemoteType remote_type, uint16_t remote_id, uint8_t group_id) {
    this->statically_paired_remotes_.emplace_back(remote_type, remote_id, group_id);
  }
  void statically_pair_remote(uint8_t remote_type, uint16_t remote_id, uint8_t group_id) {
    statically_pair_remote(static_cast<MiLightRemoteType>(remote_type), remote_id, group_id);
  }

  void set_pairing_timeout(uint32_t timeout) { this->pairing_timeout_ = timeout; }

  Trigger<> *get_pairing_start_trigger() { return this->pairing_start_trigger_; }

  Trigger<> *get_pairing_success_trigger() { return this->pairing_success_trigger_; }

  Trigger<> *get_pairing_cancelled_trigger() { return this->pairing_cancelled_trigger_; }

  Trigger<> *get_pairing_timeout_trigger() { return this->pairing_timeout_trigger_; }

  void setup() override;

  void loop() override;

  void start_pairing();

  void cancel_pairing();

  void unpair_all();

  bool is_pairing_active() const { return this->pairing_active_; }

 protected:
  bool is_remote_paired(const MiLightRemoteIdentifier &source) const;
  void got_pairing_signal(const MiLightRemoteIdentifier &candidate);

  std::vector<MiLightRxRemoteEventHandler *> listeners_;
  std::vector<MiLightRemoteIdentifier> statically_paired_remotes_;
  std::vector<MiLightRemoteIdentifier> dynamically_paired_remotes_;

  uint32_t pairing_timeout_{0};  // Timeout for dynamic pairing in milliseconds, 0 means no timeout.
  bool pairing_active_{false};
  uint32_t pairing_start_time_{0};
  std::multiset<MiLightRemoteIdentifier> pairing_candidate_remotes_;

  Trigger<> *pairing_start_trigger_{new Trigger<>()};
  Trigger<> *pairing_success_trigger_{new Trigger<>()};
  Trigger<> *pairing_cancelled_trigger_{new Trigger<>()};
  Trigger<> *pairing_timeout_trigger_{new Trigger<>()};
};

}  // namespace filter
}  // namespace milight_rx
}  // namespace esphome