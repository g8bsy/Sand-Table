import RPi.GPIO as GPIO
from utils.DRV8825 import DRV8825
import threading
import const
from time import sleep
import sys

from motors import M_Rot

def run_MRot(steps, delay):
    print("\nDelay: {}".format(delay))
    M_Rot.set_microstep('software', const.ROT_RESOLUTION)
    print("Step size: {}".format(const.ROT_RESOLUTION))

    M_Rot.turn_steps(Dir='forward', steps=steps, stepdelay=delay)
    sleep(2)
    M_Rot.turn_steps(Dir='backward', steps=steps, stepdelay=delay)
    sleep(2)

    M_Rot.stop()


delays = [0.0002]


try:
    steps = int(sys.argv[1])
    delay = int(sys.argv[2])/1000.0
    print("steps="+str(steps) + " delay=" + str(delay))
    run_MRot(steps, delay)
except KeyboardInterrupt:
    print("\nMotors stopped")
    M_Rot.stop()
    GPIO.cleanup()
    print("Exiting...")
    exit()
