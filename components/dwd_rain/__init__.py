"""Rain (DWD): when it rains in the next two hours, from a DWD precipitation forecast sensor in Home Assistant.

The tile belongs to the sensor; the app sends its state and the fields the manifest takes out of its attributes, and
dwd_rain.cpp draws them. register_plugin() takes the id, version, tile memory and the screen's texts from the manifest
and translations/ beside this folder.
"""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import smart_display
from esphome.const import CONF_ID

DEPENDENCIES = ["smart_display"]

dwd_rain_ns = cg.esphome_ns.namespace("dwd_rain")
DwdRain = dwd_rain_ns.class_("DwdRain", cg.Component)

CONFIG_SCHEMA = cv.Schema({cv.GenerateID(): cv.declare_id(DwdRain)}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await smart_display.register_plugin(var, __file__)
