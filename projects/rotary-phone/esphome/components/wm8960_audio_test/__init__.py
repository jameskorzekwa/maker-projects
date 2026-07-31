import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_ID

CONF_ADC_PIN = "adc_pin"
CONF_BCLK_PIN = "bclk_pin"
CONF_DAC_PIN = "dac_pin"
CONF_LRCLK_PIN = "lrclk_pin"
CONF_OUTPUT = "output"

DEPENDENCIES = ["i2c"]

wm8960_audio_test_ns = cg.esphome_ns.namespace("wm8960_audio_test")
WM8960AudioTest = wm8960_audio_test_ns.class_(
    "WM8960AudioTest", cg.Component, i2c.I2CDevice
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(WM8960AudioTest),
            cv.Required(CONF_BCLK_PIN): cv.int_range(min=0, max=30),
            cv.Required(CONF_LRCLK_PIN): cv.int_range(min=0, max=30),
            cv.Required(CONF_ADC_PIN): cv.int_range(min=0, max=30),
            cv.Required(CONF_DAC_PIN): cv.int_range(min=0, max=30),
            cv.Optional(CONF_OUTPUT, default="speaker"): cv.one_of(
                "speaker", "headphone", lower=True
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x1A))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    cg.add(var.set_bclk_pin(config[CONF_BCLK_PIN]))
    cg.add(var.set_lrclk_pin(config[CONF_LRCLK_PIN]))
    cg.add(var.set_adc_pin(config[CONF_ADC_PIN]))
    cg.add(var.set_dac_pin(config[CONF_DAC_PIN]))
    cg.add(var.set_headphone_output(config[CONF_OUTPUT] == "headphone"))
