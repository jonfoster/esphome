#include "esp_hsv_color.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace light {

ESPHSVColor::ESPHSVColor(Color const &color) {
  // Convert red, green and blue (all 0-1) values to hue (0-1), saturation (0-1) and value (0-1).
  float hue;
  float saturation;
  float value;
  rgb_to_hsvf(color.red / 255.0, color.green / 255.0, color.blue / 255.0, hue, saturation, value);

  // Convert to the 0-255 ranges used by this class.
  this->hue = static_cast<uint8_t>(lroundf(hue * 255.0));
  this->saturation = static_cast<uint8_t>(lroundf(saturation * 255.0));
  this->value = static_cast<uint8_t>(lroundf(value * 255.0));
}

Color ESPHSVColor::to_rgb() const {
  // based on FastLED's hsv rainbow to rgb
  const uint8_t hue = this->hue;
  const uint8_t sat = this->saturation;
  const uint8_t val = this->value;
  // upper 3 hue bits are for branch selection, lower 5 are for values
  const uint8_t offset8 = (hue & 0x1F) << 3;  // 0..248
  // third of the offset, 255/3 = 85 (actually only up to 82; 164)
  const uint8_t third = esp_scale8(offset8, 85);
  const uint8_t two_thirds = esp_scale8(offset8, 170);
  Color rgb(255, 255, 255, 0);
  switch (hue >> 5) {
    case 0b000:
      rgb.r = 255 - third;
      rgb.g = third;
      rgb.b = 0;
      break;
    case 0b001:
      rgb.r = 171;
      rgb.g = 85 + third;
      rgb.b = 0;
      break;
    case 0b010:
      rgb.r = 171 - two_thirds;
      rgb.g = 170 + third;
      rgb.b = 0;
      break;
    case 0b011:
      rgb.r = 0;
      rgb.g = 255 - third;
      rgb.b = third;
      break;
    case 0b100:
      rgb.r = 0;
      rgb.g = 171 - two_thirds;
      rgb.b = 85 + two_thirds;
      break;
    case 0b101:
      rgb.r = third;
      rgb.g = 0;
      rgb.b = 255 - third;
      break;
    case 0b110:
      rgb.r = 85 + third;
      rgb.g = 0;
      rgb.b = 171 - third;
      break;
    case 0b111:
      rgb.r = 170 + third;
      rgb.g = 0;
      rgb.b = 85 - third;
      break;
    default:
      break;
  }
  // low saturation -> add uniform color to orig. hue
  // high saturation -> use hue directly
  // scales with square of saturation
  // (r,g,b) = (r,g,b) * sat + (1 - sat)^2
  rgb *= sat;
  const uint8_t desat = 255 - sat;
  rgb += esp_scale8(desat, desat);
  // (r,g,b) = (r,g,b) * val
  rgb *= val;
  return rgb;
}

}  // namespace light
}  // namespace esphome
