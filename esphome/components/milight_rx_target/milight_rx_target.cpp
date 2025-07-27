#include "esphome/core/log.h"
#include "milight_rx_target.h"
#include "esphome/components/light/esp_hsv_float_color.h"
#include "esphome/components/light/esp_rgb_float_color.h"

namespace esphome {
namespace milight_rx {
namespace target {

static const char *TAG = "milight_rx_target";

class CallFactory {
 public:
  light::LightState const *const light;
  light::LightColorValues const &current_values;
  light::LightTraits const traits;
  light::ColorMode color_mode;
  esphome::light::LightCall call;

  explicit CallFactory(light::LightState *light)
      : light(light),
        current_values(light->remote_values),
        traits(light->get_traits()),
        color_mode(light->remote_values.get_color_mode()),
        call(light->make_call()) {}

  bool ensure_color_mode(light::ColorMode const desired_modes[], unsigned desired_modes_size);

 private:
  CallFactory(CallFactory const &) = delete;
  CallFactory &operator=(CallFactory const &) = delete;
};

bool CallFactory::ensure_color_mode(light::ColorMode const desired_modes[], unsigned desired_modes_size) {
  for (unsigned i = 0; i < desired_modes_size; ++i) {
    if (color_mode == desired_modes[i]) {
      return true;
    }
  }

  for (unsigned i = 0; i < desired_modes_size; ++i) {
    if (traits.supports_color_mode(desired_modes[i])) {
      color_mode = desired_modes[i];
      call.set_color_mode(color_mode);
      return true;
    }
  }

  return false;
}

void MiLightRxTargetComponent::dump_config() { ESP_LOGCONFIG(TAG, "milight_rx_target Component"); }

void MiLightRxTargetComponent::set_white() {
  ESP_LOGI(TAG, "Setting white");

  CallFactory factory{this->light_};

  static const light::ColorMode preferred_modes[] = {
      light::ColorMode::RGB_COLOR_TEMPERATURE,
      light::ColorMode::RGB_COLD_WARM_WHITE,
      light::ColorMode::COLOR_TEMPERATURE,
      light::ColorMode::COLD_WARM_WHITE,
      light::ColorMode::RGB_WHITE,
      light::ColorMode::WHITE,
  };

  static const light::ColorMode barely_acceptable_modes[] = {
      light::ColorMode::BRIGHTNESS,
      light::ColorMode::ON_OFF,
      light::ColorMode::RGB,
  };

  if (!factory.ensure_color_mode(preferred_modes, sizeof(preferred_modes) / sizeof(preferred_modes[0])) &&
      !factory.ensure_color_mode(barely_acceptable_modes,
                                 sizeof(barely_acceptable_modes) / sizeof(barely_acceptable_modes[0]))) {
    // Should not get here.
    ESP_LOGE(TAG, "White not supported by light");
    return;
  }

  if (factory.current_values.get_state() != 1.0) {
    factory.call.set_state(true);
  }
  if ((factory.color_mode & light::ColorCapability::BRIGHTNESS) &&
      factory.current_values.get_brightness() < 1.0 / 255.0) {
    factory.call.set_brightness(1.0);
  }
  if ((factory.color_mode & light::ColorCapability::WHITE) && factory.current_values.get_white() < 1.0 / 255.0) {
    factory.call.set_white(1.0);
  }

  if (factory.color_mode & light::ColorCapability::COLD_WARM_WHITE) {
    float total_brightness = factory.current_values.get_warm_white() + factory.current_values.get_cold_white();
    if (total_brightness < 1.0 / 255.0) {
      factory.call.set_cold_white(1.0);
      factory.call.set_warm_white(0.0);
    }
  }

  if (factory.color_mode == light::ColorMode::RGB) {
    // RGB only.  Fake white.
    float max_brightness =
        std::max<float>(std::max<float>(factory.current_values.get_red(), factory.current_values.get_green()),
                        factory.current_values.get_blue());
    if (max_brightness < 1.0 / 255.0) {
      max_brightness = 1.0;
    }
    factory.call.set_red(max_brightness);
    factory.call.set_green(max_brightness);
    factory.call.set_blue(max_brightness);

    if (factory.current_values.get_color_brightness() < 1.0 / 255.0) {
      factory.call.set_color_brightness(1.0);
    }
  } else if (factory.color_mode & light::ColorCapability::RGB) {
    // Turn off RGB channels, we want white only.
    factory.call.set_color_brightness(0.0);
  }

  factory.call.perform();
}

void MiLightRxTargetComponent::set_white_warmness(float warmness) {
  ESP_LOGI(TAG, "Setting white warmness to %f", warmness);

  CallFactory factory{this->light_};

  static const light::ColorMode desired_modes[] = {
      light::ColorMode::RGB_COLOR_TEMPERATURE,
      light::ColorMode::RGB_COLD_WARM_WHITE,
      light::ColorMode::COLOR_TEMPERATURE,
      light::ColorMode::COLD_WARM_WHITE,
  };

  if (!factory.ensure_color_mode(desired_modes, sizeof(desired_modes) / sizeof(desired_modes[0]))) {
    ESP_LOGE(TAG, "Color temperature not supported by light");
    return;
  }

  if (factory.current_values.get_state() != 1.0) {
    factory.call.set_state(true);
  }
  if (factory.current_values.get_brightness() < 1.0 / 255.0) {
    factory.call.set_brightness(1.0);
  }

  if (factory.color_mode & light::ColorCapability::COLOR_TEMPERATURE) {
    if (factory.current_values.get_white() < 1.0 / 255.0) {
      factory.call.set_white(1.0);
    }

    float min_mireds = factory.traits.get_min_mireds();
    float max_mireds = factory.traits.get_max_mireds();
    float want_mireds = (max_mireds - min_mireds) * warmness + min_mireds;
    factory.call.set_color_temperature(want_mireds);
  } else if (factory.color_mode & light::ColorCapability::COLD_WARM_WHITE) {
    float total_brightness =
        std::min<float>(1.0, factory.current_values.get_warm_white() + factory.current_values.get_cold_white());
    if (total_brightness < 1.0 / 255.0) {
      total_brightness = 1.0;
    }

    float cold_white = total_brightness * (1.0 - warmness);
    float warm_white = total_brightness * warmness;
    factory.call.set_cold_white(cold_white);
    factory.call.set_warm_white(warm_white);
  } else {
    // Should not happen.
    ESP_LOGE(TAG, "Color temperature not supported by light");
    return;
  }

  factory.call.perform();
}

void MiLightRxTargetComponent::set_color_hsv(optional<float> hue, optional<float> saturation, optional<float> value) {
  ESP_LOGI(TAG, "Setting color HSV to (%f, %f, %f)", hue.value_or(-1.0), saturation.value_or(-1.0),
           value.value_or(-1.0));

  CallFactory factory{this->light_};

  static const light::ColorMode desired_modes[] = {
      light::ColorMode::RGB_COLOR_TEMPERATURE,
      light::ColorMode::RGB_COLD_WARM_WHITE,
      light::ColorMode::RGB_WHITE,
      light::ColorMode::RGB,
  };

  if (!factory.ensure_color_mode(desired_modes, sizeof(desired_modes) / sizeof(desired_modes[0]))) {
    ESP_LOGE(TAG, "RGB not supported by light");
    return;
  }

  if (factory.current_values.get_state() != 1.0) {
    factory.call.set_state(true);
  }
  if (factory.current_values.get_brightness() < 1.0 / 255.0) {
    factory.call.set_brightness(1.0);
  }
  if (factory.current_values.get_color_brightness() < 1.0 / 255.0) {
    factory.call.set_color_brightness(1.0);
  }

  light::ESPRGBFloatColor rgb_color{factory.current_values.get_red(), factory.current_values.get_green(),
                                    factory.current_values.get_blue()};

  light::ESPHSVFloatColor hsv_color{rgb_color};

  if (hue.has_value()) {
    hsv_color.hue = hue.value();
  }
  if (saturation.has_value()) {
    hsv_color.saturation = saturation.value();
  }
  if (value.has_value()) {
    hsv_color.value = value.value();
  }
  hsv_color.normalize();

  rgb_color = hsv_color.to_rgb();

  factory.call.set_red(rgb_color.red);
  factory.call.set_green(rgb_color.green);
  factory.call.set_blue(rgb_color.blue);

  factory.call.perform();
}

void MiLightRxTargetComponent::milight_remote_event_handler(MiLightRemoteEvent const &event) {
  // TODO filter?

  switch (event.command) {
    case MiLightRemoteCommand::ON:
      ESP_LOGI(TAG, "Turning on");
      this->light_->turn_on().perform();
      // TODO set brightness and color if necessary
      break;
    case MiLightRemoteCommand::OFF:
      ESP_LOGI(TAG, "Turning off");
      this->light_->turn_off().perform();
      break;
    case MiLightRemoteCommand::SET_BRIGHTNESS:
      ESP_LOGI(TAG, "Setting brightness to %f", event.arg1);
      this->light_->make_call().set_brightness_if_supported(event.arg1).perform();
      // TODO turn on/off if necessary
      break;
    case MiLightRemoteCommand::SET_COLOR_TEMP:
      ESP_LOGI(TAG, "Setting white warmness to %f", event.arg1);
      set_white_warmness(event.arg1);
      break;
    case MiLightRemoteCommand::SET_SATURATION:
      ESP_LOGI(TAG, "Setting saturation to %f", event.arg1);
      set_color_hsv(optional<float>(), event.arg1, optional<float>());
      break;
    case MiLightRemoteCommand::SET_HUE:
      ESP_LOGI(TAG, "Setting hue to %u", event.arg1);
      set_color_hsv(event.arg1, optional<float>(), optional<float>());
      break;
    case MiLightRemoteCommand::SET_MODE:
      // ESP_LOGI(TAG, "Setting mode to %f", event.arg1);
      // TODO
      break;
    case MiLightRemoteCommand::NIGHT_MODE:
      // ESP_LOGI(TAG, "Setting night mode");
      // TODO
      break;
    case MiLightRemoteCommand::MODE_SPEED_DOWN:
      // ESP_LOGI(TAG, "Decreasing mode speed");
      // TODO This isn't supported by the esphome light / LightEffect API.
      break;
    case MiLightRemoteCommand::MODE_SPEED_UP:
      // ESP_LOGI(TAG, "Increasing mode speed");
      // TODO This isn't supported by the esphome light / LightEffect API.
      break;
    case MiLightRemoteCommand::SET_WHITE:
      ESP_LOGI(TAG, "Setting white");
      set_white();
      break;
    case MiLightRemoteCommand::SET_SATURATION_OR_COLOR_TEMP:
      // TODO Decide whether to set saturation or color temperature.
      // For now, always set color temperature.
      //
      ESP_LOGI(TAG, "Setting white warmness to %f", event.arg1);
      set_white_warmness(event.arg1);
      break;
  }
}

}  // namespace target
}  // namespace milight_rx
}  // namespace esphome