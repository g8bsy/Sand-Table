import RPi.GPIO as GPIO
from utils.DRV8825 import DRV8825
import threading
import const
from time import sleep
import sys

M_Rot = DRV8825(dir_pin=24, step_pin=18, enable_pin=4, mode_pins=(21, 22))

def run_MRot(steps):
    for resolution in resolutions:
        for delay in delays:
            print("Delay: {}, Resolution:{}, Steps:{}".format(delay, resolution, steps))
            M_Rot.set_microstep('software', resolution)
            sleep(0.2)
            print("Resolution: {}".format(resolution))
            M_Rot.turn_steps(Dir='forward', steps=steps, stepdelay=delay)
            M_Rot.turn_steps(Dir='backward', steps=steps, stepdelay=delay)

    M_Rot.stop()


delays = [0.0002, 0.002, 0.02, 0.2]
resolutions = ['halfstep', '1/4step', '1/8step', '1/16step']


try:
    steps = int(sys.argv[1])
    print("steps="+str(steps))
    run_MRot(steps)
except KeyboardInterrupt:
    print("\nMotors stopped")
    M_Rot.stop()
    GPIO.cleanup()
    print("Exiting...")
    exit()
