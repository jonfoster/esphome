from esphome import pins
import esphome.codegen as cg
from esphome.components import spi
from esphome.components.milight_rx import (
    MiLightRxRadioEventSource,
    MiLightRxRemoteEventSource,
    milight_rx_ns,
)
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_IRQ_PIN
from esphome.cpp_helpers import gpio_pin_expression

CONF_CE_PIN = "ce_pin"
CONF_CONFIG_INDEX = "config_index"
CONF_SCAN_TIME_MILLIS = "scan_time_millis"

AUTO_LOAD = ["milight_rx"]
DEPENDENCIES = ["spi"]
MULTI_CONF = True

milight_rx_radio_ns = milight_rx_ns.namespace("radio")
MiLightRxComponent = milight_rx_radio_ns.class_(
    "MiLightRxComponent",
    cg.Component,
    spi.SPIDevice,
    MiLightRxRadioEventSource,
    MiLightRxRemoteEventSource,
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(MiLightRxComponent),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(spi.spi_device_schema())
    .extend(
        {
            cv.Required(CONF_CE_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_IRQ_PIN): pins.gpio_input_pin_schema,
            cv.Optional(CONF_CONFIG_INDEX): cv.int_range(-1, 15 - 1),
            cv.Optional(CONF_SCAN_TIME_MILLIS): cv.int_range(100, None),
        }
    )
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await spi.register_spi_device(var, config)

    ce_pin = await gpio_pin_expression(config[CONF_CE_PIN])
    cg.add(var.set_ce_pin(ce_pin))

    if CONF_IRQ_PIN in config:
        irq_pin = await gpio_pin_expression(config[CONF_IRQ_PIN])
        cg.add(var.set_irq_pin(irq_pin))

    if CONF_CONFIG_INDEX in config:
        cg.add(var.set_config_index(config[CONF_CONFIG_INDEX]))

    if CONF_SCAN_TIME_MILLIS in config:
        cg.add(var.set_scan_time_millis(config[CONF_SCAN_TIME_MILLIS]))
