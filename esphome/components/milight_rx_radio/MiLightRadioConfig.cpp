#include "MiLightRadioConfig.h"

namespace esphome {
namespace milight_rx {

const MiLightRadioConfig MiLightRadioConfig::ALL_CONFIGS[] = {
    // The V2 protocol is used by most/all(?) recent products.
    MiLightRadioConfig(0xAA, 0x18097236, 0x5, 9, 8, 39, 70, true),
    // Legacy RGBW
    MiLightRadioConfig(0xAA, 0x258B147A, 0x5, 7, 9, 40, 71, false),
    // Legacy CCT
    MiLightRadioConfig(0xAA, 0x55AA050A, 0x5, 7, 4, 39, 74, false),
    // Legacy RGB
    MiLightRadioConfig(0x55, 0xBCCD9AAB, 0xA, 6, 3, 38, 73, false),
    // Legacy FUT020
    MiLightRadioConfig(0xAA, 0xAA5550A0, 0xA, 6, 6, 41, 76, false)};

}
}  // namespace esphome
