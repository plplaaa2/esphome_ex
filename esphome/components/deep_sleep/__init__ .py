import esphome.codegen as cg
from esphome.components import time
import esphome.config_validation as cv
from esphome import pins, automation
from esphome.const import (
    CONF_HOUR,
    CONF_ID,
    CONF_MINUTE,
    CONF_MODE,
    CONF_NUMBER,
    CONF_PINS,
    CONF_RUN_DURATION,
    CONF_SECOND,
    CONF_SLEEP_DURATION,
    CONF_TIME_ID,
    CONF_WAKEUP_PIN,
)

deep_sleep_ns = cg.esphome_ns.namespace("deep_sleep")
DeepSleepComponent = deep_sleep_ns.class_("DeepSleepComponent", cg.Component)
EnterDeepSleepAction = deep_sleep_ns.class_("EnterDeepSleepAction", automation.Action)
PreventDeepSleepAction = deep_sleep_ns.class_(
    "PreventDeepSleepAction",
    automation.Action,
    cg.Parented.template(DeepSleepComponent),
)
AllowDeepSleepAction = deep_sleep_ns.class_(
    "AllowDeepSleepAction",
    automation.Action,
    cg.Parented.template(DeepSleepComponent),
)

CONF_UNTIL = "until"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(DeepSleepComponent),
        cv.Optional(CONF_RUN_DURATION): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_SLEEP_DURATION): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_WAKEUP_PIN): pins.internal_gpio_input_pin_schema,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    if CONF_SLEEP_DURATION in config:
        cg.add(var.set_sleep_duration(config[CONF_SLEEP_DURATION]))
    if CONF_WAKEUP_PIN in config:
        pin = await cg.gpio_pin_expression(config[CONF_WAKEUP_PIN])
        cg.add(var.set_wakeup_pin(pin))

    cg.add_define("USE_DEEP_SLEEP")
