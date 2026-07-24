import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import ICON_COUNTER, STATE_CLASS_TOTAL_INCREASING

from .. import CONF_NOVOFERM_ID, Novoferm, novoferm_ns

DEPENDENCIES = ["novoferm"]

NovofermCycleSensor = novoferm_ns.class_(
    "NovofermCycleSensor", sensor.Sensor, cg.PollingComponent
)

CONFIG_SCHEMA = (
    sensor.sensor_schema(
        NovofermCycleSensor,
        icon=ICON_COUNTER,
        accuracy_decimals=0,
        state_class=STATE_CLASS_TOTAL_INCREASING,
    )
    .extend(
        {
            cv.GenerateID(CONF_NOVOFERM_ID): cv.use_id(Novoferm),
        }
    )
    .extend(cv.polling_component_schema("60min"))
)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)

    parent = await cg.get_variable(config[CONF_NOVOFERM_ID])
    cg.add(var.set_novoferm_parent(parent))
