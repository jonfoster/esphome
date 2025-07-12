import esphome.codegen as cg
from esphome.components import text_sensor
from esphome.components.milight_rx import (
    MiLightRxRemoteEventHandler,
    MiLightRxRemoteEventSource,
    milight_rx_ns,
)
import esphome.config_validation as cv
from esphome.const import CONF_SOURCE, ENTITY_CATEGORY_DIAGNOSTIC, ICON_SIGNAL

MiLightRxRemoteDebugSensor = milight_rx_ns.class_(
    "MiLightRxRemoteDebugSensor", cg.Component, MiLightRxRemoteEventHandler
)

CONFIG_SCHEMA = text_sensor.text_sensor_schema(
    MiLightRxRemoteDebugSensor,
    icon=ICON_SIGNAL,
    entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
).extend(
    {
        cv.GenerateID(CONF_SOURCE): cv.use_id(MiLightRxRemoteEventSource),
    }
)


async def to_code(config):
    paren = await cg.get_variable(config[CONF_SOURCE])
    var = await text_sensor.new_text_sensor(config)
    await cg.register_component(var, config)

    cg.add(paren.add_remote_event_listener(var))
