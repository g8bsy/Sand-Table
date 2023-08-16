from models import *
import uuid
import threading

class Task:
    
    task_id:str
    task=None
    name:str
    state:str="QUEUED"

    def __init__(self, name:str, task, repeat:bool):
        self.task = task
        self.name = name
        self.repeat = repeat
        self.task_id = str(uuid.uuid1())


class TaskQueue:
    
    queue = []
    queue_process_event = threading.Event()
    
    def task_process(self, event):
         while True:
            if len(self.queue) > 0:
                task = self.queue[0]
                if task.state == 'TASK_ERROR' or task.state == "TASK_COMPLETE":
                     self.queue.remove(task)
                elif task.state == "QUEUED":
                     task.state = "WAITING_TO_START"
                     task.task(task.task_id)

    def callback(self, type, task_id, msg):
        task = self.__get_by_id(task_id)
        if(task != None):
            task.state = type

    def enque(self, name, task, repeat):
        task = Task(name, task, repeat)
        self.queue.append(task)
        if(self.queue.index(task) == 0):
            task.task(task.task_id)


    def __get_by_id(self, id:str):
        for task in self.queue:
            if(task.task_id == id):
                return task
        return None

    