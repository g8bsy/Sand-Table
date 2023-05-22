from utils.DRV8825 import DRV8825
import const

M_Rot = DRV8825(dir_pin=const.ROT_DIR_PIN, step_pin=const.ROT_STEP_PIN, enable_pin=const.ROT_EN_PIN, mode_pins=const.ROT_MODE_PINS)
M_Lin = DRV8825(dir_pin=const.LIN_DIR_PIN, step_pin=const.LIN_STEP_PIN, enable_pin=const.LIN_EN_PIN, mode_pins=const.LIN_MODE_PINS)