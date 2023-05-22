from utils.DRV8825 import DRV8825
import const

M_Rot = DRV8825(dir_pin=ROT_DIR_PIN, step_pin=ROT_STEP_PIN, enable_pin=ROT_EN_PIN, mode_pins=ROT_MODE_PINS)
M_Lin = DRV8825(dir_pin=LIN_DIR_PIN, step_pin=LIN_STEP_PIN, enable_pin=LIN_EN_PIN, mode_pins=LIN_MODE_PINS)