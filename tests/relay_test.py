import RPi.GPIO as GPIO
from time import sleep
import const

GPIO.setmode(GPIO.BOARD)
GPIO.setup(const.MOTOR_RELAY_PIN, GPIO.OUT)
GPIO.setup(const.LED_RELAY_PIN, GPIO.OUT)

GPIO.output(const.MOTOR_RELAY_PIN, GPIO.LOW)
print("Motors ON")
sleep(2)
GPIO.output(const.MOTOR_RELAY_PIN, GPIO.HIGH)
print("Motors OFF")

sleep(1)

GPIO.output(const.LED_RELAY_PIN, GPIO.LOW)
print("LEDs ON")
sleep(2)
GPIO.output(const.LED_RELAY_PIN, GPIO.HIGH)
print("LEDs OFF")

GPIO.cleanup()