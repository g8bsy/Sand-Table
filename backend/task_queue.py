from models import Task
import threading

class TaskQueue:
    
    queue = []
    running = False

    def start(self):
        TaskQueue.running = True
        def set_interval(func, sec):
            def func_wrapper():
                set_interval(func, sec)
                func()
            if(TaskQueue.running):
                t = threading.Timer(sec, func_wrapper)
                t.daemon = True
                t.start()
        set_interval(self.task_process, 10)
    
    def stop(self):
        TaskQueue.running = False
      
    def task_process(self):
        #  print("task_process:", locals())
        if len(self.queue) > 0:
            task = self.queue[0]
            if task.state == "TASK_COMPLETE":
                if(len(task.tasks)>0):
                        task.run_task()
                        task.state = "WAITING_TO_COMPLETE"
                else:
                    self.queue.remove(task)
            elif task.state == "QUEUED":
                    task.state = "WAITING_TO_START"
                    task.run_task()

    def callback(self, type, task_id, msg):
        print("callback:", locals())
        task = self.__get_by_id(task_id)
        if(task != None):
            if(type == "TASK_START"):
                task.tasks.pop(0)
            task.state = type

    def enque(self, name, tasks):
        print("enque:", locals())
        task = Task(name=name, tasks=tasks)
        self.queue.append(task)
        if(self.queue.index(task) == 0):
            task.run_task()

    
    def __get_by_id(self, id:str):
        for task in self.queue:
            if(task.task_id == id):
                return task
        return None

    