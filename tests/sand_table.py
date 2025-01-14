import RPi.GPIO as GPIO
from DRV8825 import DRV8825
import threading
import math
from time import sleep
import const
from rpi_ws281x import PixelStrip, Color
import led_strip # from led_strip.py
import const

stop_motors = False # Flag for stopping motors at collision
stop_threads = False # Flag for stopping all threads

# Motor driver object init
from motors import M_Lin, M_Rot

# Create NeoPixel object with appropriate configuration.
strip = led_strip.strip_init()

# Setup for limit switches
outer_switch = 5
inner_switch = 6

GPIO.setmode(GPIO.BCM)
GPIO.setup(outer_switch, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(inner_switch, GPIO.IN, pull_up_down=GPIO.PUD_UP)


# Run through the LED strip routine
def run_LedStrip():
    print("LED state: {}".format(not stop_all_threads))
    strip.begin()

    while not stop_all_threads:
        print('LED Color wipe')
        led_strip.colorWipe(strip, led_strip.Color(255, 0, 0))  # Red wipe
        led_strip.colorWipe(strip, led_strip.Color(0, 255, 0))  # Blue wipe
        led_strip.colorWipe(strip, led_strip.Color(0, 0, 255))  # Green wipe
        print('LED Theater chase')
        led_strip.theaterChase(strip, led_strip.Color(127, 127, 127))  # White theater chase
        led_strip.theaterChase(strip, led_strip.Color(127, 0, 0))  # Red theater chase
        led_strip.theaterChase(strip, led_strip.Color(0, 0, 127))  # Blue theater chase
        print('LED Rainbow animations')
        led_strip.rainbow(strip)
        led_strip.rainbowCycle(strip)
        led_strip.theaterChaseRainbow(strip)

    print("LED state: {}".format(not stop_all_threads))


# Functions defined for each motor thread
def run_MRotate():
    print("M_Rot state: {}".format(not stop_motors))
    M_Rot.SetMicroStep('software',const.ROT_RESOLUTION)
    rot_delay = 0.0015
    forward_steps = 500
    backward_steps = 200

    while not stop_threads:
        M_Rot.TurnStep_ROT(Dir='forward', steps=forward_steps, stepdelay=rot_delay)
        M_Rot.TurnStep_ROT(Dir='backward', steps=backward_steps, stepdelay=rot_delay)

    M_Rot.Stop()
    print("M_Rot state: {}".format(not stop_motors))


def run_MLinear(num_steps, delay):
    M_Lin.SetMicroStep('software',const.LIN_RESOLUTION)

    if num_steps > 0:
        M_Lin.TurnStep(Dir='forward', steps=num_steps, stepdelay=delay)
    else:
        M_Lin.TurnStep(Dir='backward', steps=abs(num_steps), stepdelay=delay)


# Calibrates the linear slide arm before starting the main program routine
def calibrate_slide():
    delay = 0.001
    center_to_min = 20
    outer_to_max = 20

    calibrated = False

    while not calibrated:
        minPos = M_Lin.Turn_until_switch(Dir='backward', limit_switch=inner_switch, stepdelay=delay)
        maxPos = M_Lin.Turn_until_switch(Dir='forward', limit_switch=outer_switch, stepdelay=delay) + minPos

        positions = (minPos, maxPos)
        print(positions)
        totalDist = maxPos - minPos - center_to_min - outer_to_max
        print ("Travel distance: " + str(totalDist))

        sleep(.5)

        test_inner = M_Lin.Turn_check_cali(Dir='backward', steps=totalDist + outer_to_max, limit_switch=inner_switch, stepdelay=delay)
        minPos = 0
        sleep(.5)
        test_outer = M_Lin.Turn_check_cali(Dir='forward', steps=totalDist, limit_switch=outer_switch, stepdelay=delay)
        maxPos = totalDist
        test_inner = M_Lin.Turn_check_cali(Dir='backward', steps=totalDist, limit_switch=inner_switch, stepdelay=delay)

        if test_inner and test_outer:
            calibrated = True
        else:
            print("Calibration Failed! Trying again...")

    print("Calibration Passed!")

    return totalDist


# Stops the motors and LED strip, and joins the threads
def stop_program():
    stop_threads = True

    led_strip.colorWipe(strip, Color(0, 0, 0), 10)
    MRot.join()
    LStrip.join()
    End_stops.join()

    M_Rot.Stop()
    M_Lin.Stop()
    print("\nMotors stopped")
    GPIO.cleanup()
    print("Exiting...")
    exit()


def stop_all_threads():
    stop_threads = True
    M_Rot.stop_thread()


# Create a function and return the linear postion for the arm (r coordinate val)
def draw_function(maxDisp, currentTheta, rev_steps):
    # Function that can be modified to create different patterns
    pos = round(maxDisp * abs(math.cos(math.radians(360 * currentTheta / rev_steps))))
    return pos


def check_collision():
    while not stop_threads:
        if GPIO.input(inner_switch) == 0 or GPIO.input(outer_switch) == 0:
            M_Rot.Stop()
            M_Lin.Stop()
            stop_motors()


# Create seperate threads
MRot = threading.Thread(target=run_MRotate)
LStrip = threading.Thread(target=run_LedStrip)
End_stops = threading.Thread(target=check_collision)


def main():
    # Sand Table hardware constants
    rev_steps = const.STEPS_PER_REV

    currentTheta = 0 # Theta coordinate val - currently just an incrementer
    theta_steps = 100
    linPos = 0
    lastLinPos = 0
    lin_delay = 0.00125

    try:
        maxDisp = calibrate_slide() - 200

        # Start rotation, split into 3 threads (the main thread will process linear movements for MLin)
        # global MRot
        # MRot = threading.Thread(target=run_MRotate)
        # global LStrip
        # LStrip = threading.Thread(target=run_LedStrip)
        # global End_stops
        # End_stops = threading.Thread(target=check_collision)

        MRot.start()
        print("\nROT Thread Started")
        # LStrip.start()
        # print("\nLed Strip Thread Started")
        End_stops.start()
        print("\nChecking collision")

        while not stop_threads:
            lastLinPos = linPos
            linPos = draw_function(maxDisp, currentTheta, rev_steps) # Set the LinPos to the calculated position
            currentTheta += theta_steps

            print(str(linPos))

            nextPos = linPos - lastLinPos
            run_MLinear(nextPos, lin_delay)

    except KeyboardInterrupt:
        stop_program()


main()
