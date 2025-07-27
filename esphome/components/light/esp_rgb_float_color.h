#pragma once

#include "esphome/core/helpers.h"

namespace esphome {
namespace light {

struct ESPRGBFloatColor {
  float red;
  float green;
  float blue;

 private:
  // Ensure not less than 0
  static inline float clamp_0(float value) ESPHOME_ALWAYS_INLINE { return std::max<float>(0.0f, value); }
  // Ensure not greater than 1
  static inline float clamp_1(float value) ESPHOME_ALWAYS_INLINE { return std::min<float>(1.0f, value); }
  // Ensure not less than 0 and not greater than 1
  static inline float clamp(float value) ESPHOME_ALWAYS_INLINE { return std::clamp<float>(value, 0.0f, 1.0f); }

 public:
  constexpr inline ESPRGBFloatColor() ESPHOME_ALWAYS_INLINE : red(0.0), green(0.0), blue(0.0) {}
  constexpr inline ESPRGBFloatColor(float red, float green, float blue) ESPHOME_ALWAYS_INLINE : red(red),
                                                                                                green(green),
                                                                                                blue(blue) {}

  inline bool operator==(const ESPRGBFloatColor &rhs) const = default;
  inline bool operator!=(const ESPRGBFloatColor &rhs) const = default;

  inline ESPRGBFloatColor operator~() const ESPHOME_ALWAYS_INLINE {
    return ESPRGBFloatColor(1.0 - this->red, 1.0 - this->green, 1.0 - this->blue);
  }

  inline ESPRGBFloatColor operator+(const ESPRGBFloatColor &add) const ESPHOME_ALWAYS_INLINE {
    return ESPRGBFloatColor(clamp_1(this->red + add.red), clamp_1(this->green + add.green),
                            clamp_1(this->blue + add.blue));
  }
  inline ESPRGBFloatColor &operator+=(const ESPRGBFloatColor &add) ESPHOME_ALWAYS_INLINE {
    this->red = clamp_1(this->red + add.red);
    this->green = clamp_1(this->green + add.green);
    this->blue = clamp_1(this->blue + add.blue);
    return *this;
  }
  inline ESPRGBFloatColor operator-(const ESPRGBFloatColor &subtract) const ESPHOME_ALWAYS_INLINE {
    return ESPRGBFloatColor(clamp_0(this->red - subtract.red), clamp_0(this->green - subtract.green),
                            clamp_0(this->blue - subtract.blue));
  }
  inline ESPRGBFloatColor &operator-=(const ESPRGBFloatColor &subtract) ESPHOME_ALWAYS_INLINE {
    this->red = clamp_0(this->red - subtract.red);
    this->green = clamp_0(this->green - subtract.green);
    this->blue = clamp_0(this->blue - subtract.blue);
    return *this;
  }

  inline ESPRGBFloatColor operator*(float scale) const ESPHOME_ALWAYS_INLINE {
    return ESPRGBFloatColor(clamp(this->red * scale), clamp(this->green * scale), clamp(this->blue * scale));
  }
  inline ESPRGBFloatColor &operator*=(float scale) ESPHOME_ALWAYS_INLINE {
    this->red = clamp(this->red * scale);
    this->green = clamp(this->green * scale);
    this->blue = clamp(this->blue * scale);
    return *this;
  }

  inline ESPRGBFloatColor operator+(float add) const ESPHOME_ALWAYS_INLINE {
    return ESPRGBFloatColor(clamp(this->red + add), clamp(this->green + add), clamp(this->blue + add));
  }
  inline ESPRGBFloatColor &operator+=(float add) ESPHOME_ALWAYS_INLINE {
    this->red = clamp(this->red + add);
    this->green = clamp(this->green + add);
    this->blue = clamp(this->blue + add);
    return *this;
  }
  inline ESPRGBFloatColor operator-(float subtract) const ESPHOME_ALWAYS_INLINE { return (*this) + -subtract; }
  inline ESPRGBFloatColor &operator-=(float subtract) ESPHOME_ALWAYS_INLINE { return *this += -subtract; }

  ESPRGBFloatColor gradient(const ESPRGBFloatColor &to_color, float amnt) {
    ESPRGBFloatColor new_color;
    new_color.red = amnt * (to_color.red - this->red) + this->red;
    new_color.green = amnt * (to_color.green - this->green) + this->green;
    new_color.blue = amnt * (to_color.blue - this->blue) + this->blue;
    return new_color;
  }
  ESPRGBFloatColor fade_to_white(float amnt) { return this->gradient(ESPRGBFloatColor::WHITE, amnt); }
  ESPRGBFloatColor fade_to_black(float amnt) { return this->gradient(ESPRGBFloatColor::BLACK, amnt); }

  ESPRGBFloatColor lighten(float delta) { return *this + delta; }
  ESPRGBFloatColor darken(float delta) { return *this - delta; }

  static const ESPRGBFloatColor BLACK;
  static const ESPRGBFloatColor WHITE;
};

}  // namespace light
}  // namespace esphome
