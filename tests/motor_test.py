import RPi.GPIO as GPIO
from utils.DRV8825 import DRV8825
import threading
from time import sleep

from motors import M_Lin, M_Rot

def run_MRot():
    for delay in delays:
        print("\nDelay: {}".format(delay))
        for step_size in step_sizes:
            M_Rot.set_microstep('software', step_size)
            print("Step size: {}".format(step_size))

            M_Rot.turn_steps(Dir='forward', steps=12800, stepdelay=delay)
            sleep(2)
            M_Rot.turn_steps(Dir='backward', steps=12800, stepdelay=delay)
            sleep(2)

    M_Rot.stop()

def run_MLin():
    for delay in delays:
        print("\nDelay: {}".format(delay))
        for step_size in step_sizes:
            M_Lin.set_microstep('software', step_size)
            print("Step size: {}".format(step_size))

            M_Lin.turn_steps(Dir='forward', steps=1000, stepdelay=delay)
            sleep(1)
            M_Lin.turn_steps(Dir='backward', steps=1000, stepdelay=delay)
            sleep(2)

    M_Lin.stop()

delays = [0.0002]
step_sizes = ['1/8step', '1/16step']

try:
    print("\n---------- Running M_Rot ----------")
    run_MRot()
    print("\n---------- Running M_Lin ----------")
    run_MLin()
except KeyboardInterrupt:
    print("\nMotors stopped")
    M_Rot.stop()
    M_Lin.stop()
    GPIO.cleanup()
    print("Exiting...")
    exit()
