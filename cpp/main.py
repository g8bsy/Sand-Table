import MotorDriver
import time;
import RPi.GPIO as GPIO

LIN_MODE_PINS=(8, 11, 10)
ROT_MODE_PINS=(20, 19, 13)

microstep = {'1/2': (1, 0, 1),
                     '1/4': (0,1, 1),
                     '1/8': (0, 0, 1),
                     '1/16': (1, 1, 1)}

GPIO.setmode(GPIO.BCM)

ROT_RES = microstep["1/4"]
LIN_RES = microstep["1/8"]

GPIO.setup(LIN_MODE_PINS, GPIO.OUT)
GPIO.setup(ROT_MODE_PINS, GPIO.OUT)

GPIO.output(LIN_MODE_PINS, LIN_RES)
GPIO.output(ROT_MODE_PINS, ROT_RES)

# while True:
#     time.sleep(0.2)
#     print("Outer:"+str(GPIO.input(18)) + " - Inner:" + str(GPIO.input(22)))


print(MotorDriver.init(5,6,21,24,25,7,18,22))
#MotorDriver.steps(10, 10, 10);
#print(MotorDriver.calibrate())
#MotorDriver.move(-1000, 100, 0, 0);
# MotorDriver.move(-10000, 100, -10000, 100);
# MotorDriver.move(10000, 100, 10000, 100);
#exit()

print(MotorDriver.drivemotors("/home/gabrielp/Sand-Table/processed/max_disp=5727/STEPS_PER_REV=4000/MICROSTEP_SIZE=8/LIN_RESOLUTION=1_8step/ROT_RESOLUTION=1_4step/SineVsBezier2.txt"))
print("time.sleep(20)")
time.sleep(2000)
print(MotorDriver.stopmotors())
print("stopped")
time.sleep(20)
print("done")
