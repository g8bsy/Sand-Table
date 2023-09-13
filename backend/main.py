from __future__ import annotations

import uvicorn
import pathlib

from fastapi import FastAPI
from fastapi.staticfiles import StaticFiles
from fastapi.encoders import jsonable_encoder
from fastapi.responses import JSONResponse

from threading import Thread;
import yaml, os

from models import *
from task_queue import *
from utils import pi_version

import time

from led_strip import LedStripThread, LedMethod

led = LedStripThread()

if not pi_version():
    import DummyMotorDriver as MD
else:
    import MotorDriver as MD 

app = FastAPI()
api = FastAPI()
app.mount("/api", api)

this_file_path = str(pathlib.Path(__file__).parent.parent.resolve())

app.mount(
    "/rendered/",
    StaticFiles(
        directory=this_file_path+"/rendered", html=False
    ),
    name="rendered",
)

app.mount(
    "/",
    StaticFiles(
        directory=this_file_path+"/frontend/dist", html=True
    ),
    name="frontend",
)


cfg = ""


with open(this_file_path+"/backend/config.yaml", "r") as stream:
    try:
        cfg = yaml.safe_load(stream)
    except yaml.YAMLError as exc:
        print(exc)

task_queue = TaskQueue()     
MD.set_callback(task_queue.callback)   
task_queue.start()

MD.init(
    cfg["pi_pins"]["rot_dir_pin"],
    cfg["pi_pins"]["rot_step_pin"],
    cfg["pi_pins"]["rot_en_pin"],
    cfg["pi_pins"]["lin_dir_pin"],
    cfg["pi_pins"]["lin_step_pin"],
    cfg["pi_pins"]["lin_en_pin"],
    cfg["pi_pins"]["outer_limit_pin"],
    cfg["pi_pins"]["inner_limit_pin"], 
    cfg["pi_pins"]["rotation_limit_pin"])

@app.on_event("startup")
def startup():
    print('@app.on_event("startup")') 
    led.start()

@app.on_event("shutdown")
def shutdown():
    print('@api.on_event("shutdown")')
    led.stop()

@api.get("/")
async def root():
    return {"message": "Hello World"} 

@api.get("/calibrate")
def calibrate():
    task_queue.enque("Calibrate", [lambda tid:MD.calibrate(tid)])
    return {"message": "OK"}

@api.get("/steps/{lin_steps}/{rot_steps}/{delay}")
def steps (lin_steps : int, rot_steps : int, delay : int):
    print(task_queue)
    task_queue.enque("steps ", lambda tid:MD.steps(tid, lin_steps, rot_steps, delay), False)
    return {"message": "OK"}

@api.get("/run_file/{filename}")
def run_file (filename):
    full_filename = this_file_path+"/tracks/" + filename
    task_queue.enque(full_filename, [lambda tid:MD.run_file(tid, full_filename)])
    return {"message": "OK"}

@api.get("/stopmotors")
async def stopmotors ():
    MD.stopmotors()
    return {"message": "OK"}

@api.get("/set_speed/{speed}")
async def set_speed (speed):
    MD.set_speed(speed)
    return {"message": "OK"}

@api.get("/tasks", 
         response_model_exclude={"name"})
async def tasks ():
    return JSONResponse(jsonable_encoder(task_queue.queue, exclude={'tasks'}))

@api.get("/led/cfg")
async def led_cfg ():
    return led.get_cfg()

@api.get("/led/put_method/{method}")
async def led_set_method (method):
    led.set_method(LedMethod[method])

@api.get("/led/put_color/{color}")
async def led_set_color (color):
    led.set_color(color)    

@api.get("/led/put_brightness/{brightness}")
async def led_set_brightness (brightness:int):
    led.set_brightness(brightness)        

@api.get("/task/{task_id}/{status}")
async def task_move (task_id, status):
    task_queue.callback(status, task_id, "msg")
    return {"message": "OK"}

@api.get("/files", response_model=list[ThetaRhoFile])
async def files():
    from os import listdir
    from os.path import isfile, join

    print(cfg)
    onlyfiles = [
        f
        for f in listdir(this_file_path+"/tracks/")
        if isfile(this_file_path+"/tracks/"+f)
    ]

    trf = []
    for f in sorted(onlyfiles, key=str.casefold):
        trf.append(ThetaRhoFile(id=f, name=f.split('.')[0], start=0, end=1))

    return trf


if __name__ == "__main__":
    uvicorn.run("main:app", host="0.0.0.0",reload=True, port=8000)
    
    

