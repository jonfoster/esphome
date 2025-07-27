#include "esp_hsv_float_color.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace light {

ESPHSVFloatColor::ESPHSVFloatColor(Color const &color) {
  // Convert red, green and blue (all 0-1) values to hue (0-1), saturation (0-1) and value (0-1).
  rgb_to_hsvf(color.red / 255.0, color.green / 255.0, color.blue / 255.0, this->hue, this->saturation, this->value);
}

ESPHSVFloatColor::ESPHSVFloatColor(ESPRGBFloatColor const &color) {
  rgb_to_hsvf(color.red, color.green, color.blue, this->hue, this->saturation, this->value);
}

ESPRGBFloatColor ESPHSVFloatColor::to_rgb() const {
  ESPRGBFloatColor rgb;
  hsvf_to_rgb(this->hue, this->saturation, this->value, rgb.red, rgb.green, rgb.blue);
  return rgb;
}

void ESPHSVFloatColor::normalize() {
  this->hue = std::fmod(this->hue, 1.0f);
  if (this->hue < 0.0f)
    this->hue += 1.0f;

  this->saturation = std::clamp(this->saturation, 0.0f, 1.0f);
  this->value = std::clamp(this->value, 0.0f, 1.0f);
}

}  // namespace light
}  // namespace esphome
