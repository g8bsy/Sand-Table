from fastapi import FastAPI
from fastapi.staticfiles import StaticFiles

import yaml
from models import *

app = FastAPI()
api = FastAPI()
app.mount("/api", api)

app.mount(
    "/",
    StaticFiles(
        directory="/Users/gabrielp/Documents/GitHub/Sand-Table/frontend/dist", html=True
    ),
    name="frontend",
)


cfg = ""


with open("config.yaml", "r") as stream:
    try:
        cfg = yaml.safe_load(stream)
    except yaml.YAMLError as exc:
        print(exc)


@api.get("/")
async def root():
    return {"message": "Hello World"}


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
