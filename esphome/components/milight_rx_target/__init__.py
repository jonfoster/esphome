import esphome.codegen as cg
from esphome.components import light
from esphome.components.milight_rx import (
    MiLightRxRemoteEventHandler,
    MiLightRxRemoteEventSource,
    milight_rx_ns,
)
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_LIGHT, CONF_SOURCE

AUTO_LOAD = ["milight_rx"]
MULTI_CONF = True

milight_rx_target_ns = milight_rx_ns.namespace("target")
MiLightRxTargetComponent = milight_rx_target_ns.class_(
    "MiLightRxTargetComponent", cg.Component, MiLightRxRemoteEventHandler
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(MiLightRxTargetComponent),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(
        {
            cv.Required(CONF_LIGHT): cv.use_id(light.LightState),
            cv.Required(CONF_SOURCE): cv.use_id(MiLightRxRemoteEventSource),
        }
    )
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    target_light = await cg.get_variable(config[CONF_LIGHT])
    cg.add(var.set_light(target_light))

    radio = await cg.get_variable(config[CONF_SOURCE])
    cg.add(radio.add_remote_event_listener(var))
