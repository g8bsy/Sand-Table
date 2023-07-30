from __future__ import annotations
from fastapi import FastAPI
from fastapi.staticfiles import StaticFiles

from threading import Thread;
import yaml

from models import *
from utils import pi_version

if not pi_version():
    import DummyMotorDriver as MD
else:
    import MotorDriver as MD

from threading import Lock
lock = Lock()

app = FastAPI()
api = FastAPI()
app.mount("/api", api)

# app.mount(
#     "/",
#     StaticFiles(
#         directory="/home/gabrielp/Sand-Table/frontend/dist", html=True
#     ),
#     name="frontend",
# )


cfg = ""


with open("config.yaml", "r") as stream:
    try:
        cfg = yaml.safe_load(stream)
    except yaml.YAMLError as exc:
        print(exc)

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


def locked(func):
    successfully_acquired = lock.acquire(False)
    if successfully_acquired:
        print("Got Lock")
        try:
            x = Thread(target=func)
            print("Start thread")
            x.start()
            print("Thread started")
            return {"message": "OK"}
        finally:
            lock.release()
    else:
        print("Can't get Lock")
        return {"message": "Locked"}
    

@api.get("/")
async def root():
    return {"message": "Hello World"}

@api.get("/calibrate")
def calibrate():
    MD.calibrate()
    return {"message": "OK"}

@api.get("/steps/{lin_steps}/{rot_steps}/{delay}")
def steps (lin_steps : int, rot_steps : int, delay : int):
    return locked(lambda : MD.steps(lin_steps, rot_steps, delay))

@api.get("/run_file/{filename}")
def run_file (filename):
    MD.run_file(filename)
    return {"message": "OK"}

@api.get("/stopmotors")
async def stopmotors ():
    MD.stopmotors()
    return {"message": "OK"}

@api.get("/set_speed/{speed}")
async def set_speed (speed):
    MD.set_speed(speed)
    return {"message": "OK"}

@api.get("/files", response_model=list[ThetaRhoFile])
async def files():
    from os import listdir
    from os.path import isfile, join

    print(cfg)
    onlyfiles = [
        f
        for f in listdir(cfg["theta_rho_dir"])
        if isfile(join(cfg["theta_rho_dir"], f))
    ]

    trf = []
    for f in onlyfiles:
        trf.append(ThetaRhoFile(id=f, start=0, end=1))

    return trf
