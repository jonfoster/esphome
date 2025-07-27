#pragma once

#include "esphome/core/color.h"
#include "esphome/core/helpers.h"
#include "esp_rgb_float_color.h"

namespace esphome {
namespace light {

struct ESPHSVFloatColor {
  float hue;
  float saturation;
  float value;

  constexpr inline ESPHSVFloatColor() ESPHOME_ALWAYS_INLINE : hue(0), saturation(0), value(0) {}

  constexpr inline ESPHSVFloatColor(float hue, float saturation, float value) ESPHOME_ALWAYS_INLINE
      : hue(hue),
        saturation(saturation),
        value(value) {}
  explicit ESPHSVFloatColor(Color const &color);
  explicit ESPHSVFloatColor(ESPRGBFloatColor const &color);

  ESPRGBFloatColor to_rgb() const;

  void normalize();
};

}  // namespace light
}  // namespace esphome
