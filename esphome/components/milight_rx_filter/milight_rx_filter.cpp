#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "milight_rx_filter.h"

namespace esphome {
namespace milight_rx {
namespace filter {

static const char *TAG = "milight_rx_filter";

void MiLightRxFilterComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Milight RX Filter Component");
  // TODO
}

bool MiLightRxFilterComponent::is_remote_paired(const MiLightRemoteIdentifier &source) const {
  if (!source.is_all_groups()) {
    for (const auto &remote : this->statically_paired_remotes_) {
      if (remote == source) {
        return true;
      }
    }
    for (const auto &remote : this->dynamically_paired_remotes_) {
      if (remote == source) {
        return true;
      }
    }
  } else {
    for (const auto &remote : this->statically_paired_remotes_) {
      if (remote.get_all_groups_address() == source) {
        return true;
      }
    }
    for (const auto &remote : this->dynamically_paired_remotes_) {
      if (remote.get_all_groups_address() == source) {
        return true;
      }
    }
  }
  return false;
}

void MiLightRxFilterComponent::got_pairing_signal(const MiLightRemoteIdentifier &candidate) {
  // In pairing mode and recieved the "on" command from a remote that is not currently paired.
  // If we recieve this 3 times, we pair the remote.

  // If we already saw this remote twice, then this is the third time,
  // so we pair it and leave pairing mode.
  if (this->pairing_candidate_remotes_.count(candidate) >= 2) {
    ESP_LOGI(TAG, "Paired remote %u 0x%04x %u", candidate.get_remote_type(), candidate.get_remote_id(),
             candidate.get_group_id());
    this->dynamically_paired_remotes_.push_back(candidate);
    this->pairing_candidate_remotes_.clear();
    this->pairing_active_ = false;
    return;
  }

  // Avoid unbounded memory consumption.  In practise, this limit will never
  // be reached.
  if (this->pairing_candidate_remotes_.size() >= 20) {
    ESP_LOGW(TAG, "Too many pairing candidates, restarting pairing");
    this->pairing_candidate_remotes_.clear();
  }

  this->pairing_candidate_remotes_.insert(candidate);
}

void MiLightRxFilterComponent::milight_remote_event_handler(MiLightRemoteEvent const &event) {
  MiLightRemoteIdentifier remote_id{event};
  if (!is_remote_paired(remote_id)) {
    // The pairing happens in response to the "on" command.
    // We don't pair if the remote is using the wildcard group ID 0,
    // because we wouldn't know which group to pair to.
    // (Single-group remotes use group ID 1, not 0.  Group 0 is only used by
    // multi-group remotes when you press an "all groups" button).
    if (this->pairing_active_ && event.command == MiLightRemoteCommand::ON && !remote_id.is_all_groups()) {
      // Maybe pair it!
      got_pairing_signal(remote_id);
    }
    return;
  }

  for (auto *listener : this->listeners_) {
    listener->milight_remote_event_handler(event);
  }
}

void MiLightRxFilterComponent::setup() {
  // TODO read dynamic pairing data from preferences into dynamically_paired_remotes_
}

void MiLightRxFilterComponent::loop() {
  if (this->pairing_active_ && this->pairing_start_time_ != 0 &&
      (millis() - this->pairing_start_time_) > this->pairing_timeout_) {
    this->pairing_candidate_remotes_.clear();
    this->pairing_active_ = false;
    this->pairing_timeout_trigger_->trigger();
  }
}

void MiLightRxFilterComponent::start_pairing() {
  if (this->pairing_active_) {
    // Restart pairing timeout
    this->pairing_start_time_ = millis();
    return;
  }

  this->pairing_candidate_remotes_.clear();
  this->pairing_active_ = true;
  this->pairing_start_time_ = millis();

  this->pairing_start_trigger_->trigger();
}

void MiLightRxFilterComponent::cancel_pairing() {
  if (this->pairing_active_) {
    this->pairing_active_ = false;
    this->pairing_candidate_remotes_.clear();
    this->pairing_cancelled_trigger_->trigger();
  }
}

void MiLightRxFilterComponent::unpair_all() { dynamically_paired_remotes_.clear(); }

}  // namespace filter
}  // namespace milight_rx
}  // namespace esphome