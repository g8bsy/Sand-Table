import time
import RPi.GPIO as GPIO

GPIO.setmode(GPIO.BCM)

GPIO.setup(18, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(22, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(17, GPIO.IN, pull_up_down=GPIO.PUD_UP)


while True:
    time.sleep(0.2)
    print("Outer:"+str(GPIO.input(18)) + " - Inner:" + str(GPIO.input(22)) + " - Rotate:" + str(GPIO.input(17)))

