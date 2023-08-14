from pydantic import BaseModel

class ThetaRhoFile(BaseModel):
    id: str
    name: str
    start: int 
    end: int