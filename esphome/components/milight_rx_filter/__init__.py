import esphome.codegen as cg
from esphome.components.milight_rx import (
    MiLightRxRemoteEventHandler,
    MiLightRxRemoteEventSource,
    milight_rx_ns,
)
import esphome.config_validation as cv
from esphome.const import CONF_GROUP, CONF_ID, CONF_PROTOCOL, CONF_SOURCE

CONF_REMOTES = "remotes"
CONF_PAIRING_TIMEOUT = "pairing_timeout"

AUTO_LOAD = ["milight_rx"]
MULTI_CONF = True

milight_rx_filter_ns = milight_rx_ns.namespace("filter")
MiLightRxFilterComponent = milight_rx_filter_ns.class_(
    "MiLightRxFilterComponent",
    cg.Component,
    MiLightRxRemoteEventHandler,
    MiLightRxRemoteEventSource,
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(MiLightRxFilterComponent),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(
        {
            cv.Required(CONF_SOURCE): cv.use_id(MiLightRxRemoteEventSource),
            cv.Optional(CONF_REMOTES): cv.ensure_list(
                cv.Schema(
                    {
                        cv.Required(CONF_PROTOCOL): cv.int_range(0, 254),
                        cv.Required(CONF_ID): cv.hex_uint16_t,
                        cv.Required(CONF_GROUP): cv.uint8_t,
                    }
                )
            ),
            cv.Optional(CONF_PAIRING_TIMEOUT): cv.positive_time_period_milliseconds,
        }
    )
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    radio = await cg.get_variable(config[CONF_SOURCE])
    cg.add(radio.add_remote_event_listener(var))

    # TODO figure out how to allow 0 to be set.
    if CONF_PAIRING_TIMEOUT in config:
        total_milliseconds = config[CONF_PAIRING_TIMEOUT].total_milliseconds
        cg.add(var.set_pairing_timeout(total_milliseconds))

    if CONF_REMOTES in config:
        for remote in config[CONF_REMOTES]:
            cg.add(
                var.statically_pair_remote(
                    remote[CONF_PROTOCOL], remote[CONF_ID], remote[CONF_GROUP]
                )
            )

    # TODO support triggers
