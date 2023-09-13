import MotorDriver
import os
import time
import threading;
import RPi.GPIO as GPIO
from threading import Event

exit_event_obj = threading.Event()
running = True

def stop():
    print("Stop")
    running = False
    exit_event_obj.set()

def set_speed():
    new_speed = int(input("New Speed"))
    MotorDriver.set_speed(new_speed)
    callback("TASK_COMPLETE", 0, "")

def move():
    move_lin = int(input("Linear steps? "))
    move_rot = int(input("Rotate steps? "))
    MotorDriver.steps("0", move_lin, move_rot, 1)

def callback(type, task_id, msg):
    print("callback ", locals())

    if type == 'TASK_COMPLETE' :

        choices = {}

        choices['C'] = {'name' : "Calibrate", 'f' : lambda : MotorDriver.calibrate('g')}# 
        choices['X'] = {'name' : "Exit", 'f' : lambda : stop()} 
        choices['S'] = {'name' : "Speed", 'f' : lambda : set_speed()}
        choices['M'] = {'name' : "Move", 'f' : lambda : move()}

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

while not exit_event_obj.is_set():
    try:
        print("waiting for event")
        exit_event_obj.wait()
    except KeyboardInterrupt: # If CTRL+C is pressed, exit cleanly:
        print("Keyboard interrupt")
        print(MotorDriver.stopmotors())
