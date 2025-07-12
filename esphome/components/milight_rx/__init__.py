import esphome.codegen as cg

milight_rx_ns = cg.esphome_ns.namespace("milight_rx")

MiLightRxRadioEventSource = milight_rx_ns.class_("MiLightRxRadioEventSource")
MiLightRxRadioEventHandler = milight_rx_ns.class_("MiLightRxRadioEventHandler")

MiLightRxRemoteEventSource = milight_rx_ns.class_("MiLightRxRemoteEventSource")
MiLightRxRemoteEventHandler = milight_rx_ns.class_("MiLightRxRemoteEventHandler")
