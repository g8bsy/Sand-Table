import RPi.GPIO as GPIO
import time
import const
GPIO.setmode(GPIO.BCM)
GPIO.setup(const.INNER_SWITCH_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(const.OUTER_SWITCH_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)

while True:
    print("Outer:"+str(GPIO.input(const.OUTER_SWITCH_PIN)) + " - Inner:" + str(GPIO.input(const.INNER_SWITCH_PIN)))
