from pydantic import BaseModel

class ThetaRhoFile(BaseModel):
    id: str
    start: int 
    end: int

class Task:
    repeatable: bool
    task: function
    name:str