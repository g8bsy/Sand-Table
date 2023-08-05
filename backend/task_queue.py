from models import *
import uuid

class Task:
    
    task_id:str
    task=None
    name:str
    state="QUEUED"
    repeat:bool
    error_count=0

    def __init__(self, name:str, task, repeat:bool):
        self.task = task
        self.name = name
        self.repeat = repeat
        self.task_id = str(uuid.uuid1())


class TaskQueue:
    
    queue = []

    def callback(self, type, task_id, msg):
        
        if type == "TASK_START":
            task = self.__get_by_id(task_id)
            if(task != None):
                task.state = "RUNNING"
            return

        elif type == "TASK_COMPLETE":
            task = self.__get_by_id(task_id)
            if(task != None):
                task.state = "COMPLETE"
                self.queue.remove(task)
                if(task.repeat):
                    self.queue.append(task)
                if(len(self.queue)>0):
                    self.queue[0].task(self.queue[0].task_id)

        elif type == "TASK_ERROR":
            task = self.__get_by_id(task_id)
            if(task != None):
                task.state = "ERROR"
                task.error_count += 1

        print("callback ", locals())

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

    