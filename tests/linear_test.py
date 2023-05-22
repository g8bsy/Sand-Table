import RPi.GPIO as GPIO
from utils.DRV8825 import DRV8825
import threading
import const
from time import sleep
import sys

from const import M_Lin, M_Rot

def run_MLin(steps):
    for resolution in resolutions:
        for delay in delays:
            print("Delay: {}, Resolution:{}, Steps:{}".format(delay, resolution, steps))
            M_Lin.set_microstep('software', resolution)
            sleep(0.2)
            print("Resolution: {}".format(resolution))
            M_Lin.turn_steps(Dir='forward', steps=steps, stepdelay=delay)
            M_Lin.turn_steps(Dir='backward', steps=steps, stepdelay=delay)

    M_Lin.stop()


delays = [0.0002, 0.002, 0.02, 0.2]
resolutions = ['halfstep', '1/4step', '1/8step', '1/16step']


try:
    steps = int(sys.argv[1])
    print("steps="+str(steps))
    run_MLin(steps)
except KeyboardInterrupt:
    print("\nMotors stopped")
    M_Lin.stop()
    GPIO.cleanup()
    print("Exiting...")
    exit()
