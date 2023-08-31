from threading import Timer

callback = {}

def init (rot_dir_pin, rot_step_pin, rot_en_pin, lin_dir_pin, lin_step_pin, lin_en_pin, outer_limit_pin, inner_limit_pin, rotation_limit_pin):
    print("init ", locals())

@staticmethod
def calibrate ():
    print("calibrate ", locals())

@staticmethod
def steps (lin_steps, rot_steps, delay):
    print("steps ", locals())


def run_file (tid, filename):
    print("run_file ", locals())
    print(callback['fn'])
    t = Timer(1, timer_method, {'TASK_START', tid, "GABS"})
    t.start()

def timer_method(type, task_id, msg):
    print("zdjfnljsdfjsdfjsjf")
    callback['fn'](type, task_id, msg)

@staticmethod
def stopmotors ():
    print("stopmotors ", locals())

@staticmethod 
def set_speed (speed):
    print("set_speed ", locals()) 

def set_callback(fn):
    callback['fn'] = fn
    print("set_callback ", locals())
    print(callback)
    
