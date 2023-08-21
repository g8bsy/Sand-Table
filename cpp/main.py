import MotorDriver
import os
import time
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

running = True

def stop():
    running = False

def callback(type, task_id, msg):
    print("callback ", locals())

    if type == 'TASK_COMPLETE' :

        choices = {}

        choices['C'] = {'name' : "Calibrate", 'f' : lambda : MotorDriver.calibrate('g')}# 
        choices['X'] = {'name' : "Exit", 'f' : lambda : stop()}# 

        dir_path = '/home/gabrielp/Sand-Table/tracks/'
        counter = 0
        # Iterate directory
        onlyfiles =  os.listdir(dir_path)

        for file_path in sorted(onlyfiles, key=str.casefold):
            if os.path.isfile(os.path.join(dir_path, file_path)):
                # add filename to list
                counter += 1
                filename = os.path.join(dir_path, file_path)
                choices[str(counter)] = {'name' : file_path, 'f' : lambda f=filename : MotorDriver.run_file("c", f)}
        
        for key, value in choices.items():
            print(key + " " + value["name"])

        inp = -1

        while inp not in choices.keys():
            inp = input("Enter your choice: ")

        choices[inp]['f']()

# while True:
#     time.sleep(0.2)
#     print("Outer:"+str(GPIO.input(18)) + " - Inner:" + str(GPIO.input(22)))

def thread_function(fname):
    MotorDriver.run_file(fname)
    print ("thread_function done");

MotorDriver.set_callback(callback)

print(MotorDriver.init(5,6,21,24,25,7,22,18,17))
x = threading.Thread(target=callback, args=("TASK_COMPLETE", "0", "0"))
x.start()
x.join()

while running:
    time.sleep(10000)

input("Press to stop");
print(MotorDriver.stopmotors())
input("Press to exit");
