import MotorDriver
import threading;
import RPi.GPIO as GPIO

LIN_MODE_PINS=(8, 11, 10)
ROT_MODE_PINS=(20, 19, 13)

microstep = {'1/2': (1, 0, 1),
                     '1/4': (0,1, 1),
                     '1/8': (0, 0, 1),
                     '1/16': (1, 1, 1)}

GPIO.setmode(GPIO.BCM)

ROT_RES = microstep["1/16"]
LIN_RES = microstep["1/16"]

GPIO.setup(LIN_MODE_PINS, GPIO.OUT)
GPIO.setup(ROT_MODE_PINS, GPIO.OUT)

GPIO.output(LIN_MODE_PINS, LIN_RES)
GPIO.output(ROT_MODE_PINS, ROT_RES)

# while True:
#     time.sleep(0.2)
#     print("Outer:"+str(GPIO.input(18)) + " - Inner:" + str(GPIO.input(22)))

def thread_function(fname):
    MotorDriver.run_file(fname)
    print ("thread_function done");

print(MotorDriver.init(5,6,21,24,25,7,22,18,17))
print(MotorDriver.calibrate());

x = threading.Thread(target=thread_function, args=("/home/gabrielp/Sand-Table/pending/BurstyBezier.txt",))
x.start()

# MotorDriver.steps(0, 31960, 1);
#MotorDriver.move(-1000, 100, 0, 0);
# MotorDriver.move(-10000, 100, -10000, 100);
# MotorDriver.move(10000, 100, 10000, 100);
#exit()

# print(MotorDriver.drivemotors("/home/gabrielp/Sand-Table/processed/max_disp=5727/STEPS_PER_REV=4000/MICROSTEP_SIZE=8/LIN_RESOLUTION=1_8step/ROT_RESOLUTION=1_4step/SineVsBezier2.txt"))
# print("time.sleep(20)")
# time.sleep(2000)
# print(MotorDriver.stopmotors())
# print("stopped")
# time.sleep(20)
# print("done")

input("Press to stop");
print(MotorDriver.stopmotors())
input("Press to exit");
