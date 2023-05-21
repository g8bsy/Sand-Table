import RPi.GPIO as GPIO
from time import sleep
import sys


try:
    pin = int(sys.argv[1])
    GPIO.setmode(GPIO.BCM)
    GPIO.setwarnings(True)
    GPIO.setup(pin, GPIO.OUT)
    while True:
        print("pin:"+str(pin) + " on")
        GPIO.output(pin, 1)
        sleep(0.5)
        print("pin:"+str(pin) + " off")
        GPIO.output(pin, 0)
        sleep(0.5)
except KeyboardInterrupt:
    print("\nMotors stopped")
    GPIO.cleanup()
    print("Exiting...")
    exit()
