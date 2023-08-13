import os

def callback(type, task_id, msg):

    choices = {}

    choices['C'] = {'name' : "Calibrate", 'f' : lambda : print("Gabs")}# MotorDriver.calibrate()

    dir_path = '/home/gabrielp/Sand-Table/pending/'
    counter = 0
    # Iterate directory
    for file_path in os.listdir(dir_path):
        if os.path.isfile(os.path.join(dir_path, file_path)):
            # add filename to list
            counter += 1
            choices[str(counter)] = {'name' : file_path, 'f' : lambda : print(os.path.join(dir_path, file_path))}
    
    for key, value in choices.items():
        print(key + " " + value["name"])

    inp = -1

    while inp not in choices.keys():
        inp = input("Enter your choice: ")

    choices[inp]['f']()

callback(0,0,0)
            