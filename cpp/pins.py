import RPi.GPIO as GPIO

import time

outer_limit_pin = 22
inner_limit_pin =18 
rot_limit_pin = 17

GPIO.setmode(GPIO.BCM)

GPIO.setup(inner_limit_pin, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(outer_limit_pin, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(rot_limit_pin, GPIO.IN, pull_up_down=GPIO.PUD_UP)

try:
     while True:
        print("outer_limit_pin:", GPIO.input(outer_limit_pin), " ", "inner_limit_pin:", GPIO.input(inner_limit_pin), " ", "rot_limit_pin:", GPIO.input(rot_limit_pin))
        time.sleep(1)
except KeyboardInterrupt: # If CTRL+C is pressed, exit cleanly:
   print("Keyboard interrupt")

except:
   print("some error") 

finally:
   print("clean up") 
   GPIO.cleanup() # cleanup all GPIO 

