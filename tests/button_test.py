import RPi.GPIO as GPIO
from time import sleep
import const

GPIO.setmode(GPIO.BCM)
GPIO.setup(const.BUTTON_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)

while True:
    sleep(0.2)
    print(GPIO.input(const.BUTTON_PIN))