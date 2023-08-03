from models import *

class TaskQueue:
    queue = []

    def get_next():
        return queue.pop()