from pydantic import BaseModel, Field
import uuid

class CustomBaseModel(BaseModel):
    def dict(self, **kwargs):
        hidden_fields = set(
            attribute_name
            for attribute_name, model_field in self.__fields__.items()
            if model_field.field_info.extra.get("hidden") is True
        )
        kwargs.setdefault("exclude", hidden_fields)
        return super().dict(**kwargs)

class ThetaRhoFile(CustomBaseModel):
    id: str
    name: str
    start: int 
    end: int

class Task(CustomBaseModel):
    
    task_id:str=str(uuid.uuid1())
    tasks:list = [None]
    name:str
    state:str="QUEUED"

    def run_task(self):
        print(self)
        self.tasks[0](self.task_id)

