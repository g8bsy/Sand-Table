from __future__ import annotations
from fastapi import FastAPI
from fastapi.staticfiles import StaticFiles
from fastapi.encoders import jsonable_encoder
from fastapi.responses import JSONResponse

from threading import Thread;
import yaml, os

from models import *
from task_queue import *
from utils import pi_version

if not pi_version():
    import DummyMotorDriver as MD
else:
    import MotorDriver as MD

app = FastAPI()
api = FastAPI()
app.mount("/api", api)

app.mount(
    "/rendered/",
    StaticFiles(
        directory=os.path.dirname(os.getcwd())+"/rendered", html=False
    ),
    name="rendered",
)

app.mount(
    "/",
    StaticFiles(
        directory=os.path.dirname(os.getcwd())+"/frontend/dist", html=True
    ),
    name="frontend",
)


cfg = ""


with open("config.yaml", "r") as stream:
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


@api.get("/")
async def root():
    return {"message": "Hello World"}

@api.get("/calibrate")
def calibrate():
    task_queue.enque("Calibrate", lambda tid:MD.calibrate(tid), False)
    return {"message": "OK"}

@api.get("/steps/{lin_steps}/{rot_steps}/{delay}")
def steps (lin_steps : int, rot_steps : int, delay : int):
    print(task_queue)
    task_queue.enque("steps ", lambda tid:MD.steps(tid, lin_steps, rot_steps, delay), False)
    return {"message": "OK"}

@api.get("/run_file/{filename}")
def run_file (filename):
    task_queue.enque("run_file ", [lambda tid:MD.run_file(tid, os.path.dirname(os.getcwd())+"/tracks/" + filename)])
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
    #return task_queue.queue

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
        for f in listdir(os.path.dirname(os.getcwd())+"/tracks/")
        if isfile(join(os.path.dirname(os.getcwd())+"/tracks/", f))
    ]

    trf = []
    for f in sorted(onlyfiles, key=str.casefold):
        trf.append(ThetaRhoFile(id=f, name=f.split('.')[0], start=0, end=1))

    return trf
