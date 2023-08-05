from pydantic import BaseModel

class ThetaRhoFile(BaseModel):
    id: str
    start: int 
    end: int