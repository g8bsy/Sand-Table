import RPi.GPIO as GPIO
import time

MotorDir = [
    'forward',
    'backward',
]

ControlMode = [
    'hardware',
    'software',
]

GPIO.setmode(GPIO.BCM)


class DRV8825():

    def __init__(self, dir_pin, step_pin, enable_pin, mode_pins):
        self.dir_pin = dir_pin
        self.step_pin = step_pin
        self.enable_pin = enable_pin
        self.mode_pins = mode_pins
        self.running = True

        GPIO.setmode(GPIO.BCM)
        GPIO.setwarnings(True)
        GPIO.setup(self.dir_pin, GPIO.OUT)
        GPIO.setup(self.step_pin, GPIO.OUT)
        GPIO.setup(self.enable_pin, GPIO.OUT)
        GPIO.setup(self.mode_pins, GPIO.OUT)


    def digital_write(self, pin, value):
        GPIO.output(pin, value)


    def stop(self):
        self.digital_write(self.enable_pin, 1)


    def set_microstep(self, mode, stepformat):
        """
        (1) mode
            'hardware' :    Use the switch on the module to control the microstep
            'software' :    Use software to control microstep pin levels
                Need to put the All switch to 0
        (2) stepformat
            ('fullstep', 'halfstep', '1/4step', '1/8step', '1/16step', '1/32step')
        """
        microstep = {'halfstep': (1, 0, 1),
                     '1/4step': (0,1, 1),
                     '1/8step': (0, 0, 1),
                     '1/16step': (1, 1, 1)}
        
        # microstep = {'halfstep': (1, 0),
        #              '1/4step': (0,1),
        #              'fullstep': (0, 0),
        #              '1/16step': (1, 1)}

        print("Control mode: ",mode)
        if (mode == ControlMode[1]):
            print("set pins: {}".format(stepformat))
            self.digital_write(self.mode_pins, microstep[stepformat])


    def turn_steps(self, Dir, steps, stepdelay):
        print("turn_steps")
        if (Dir == MotorDir[0]):
            # print("forward")
            self.digital_write(self.enable_pin, 0)
            self.digital_write(self.dir_pin, 0)
        elif (Dir == MotorDir[1]):
            # print("backward")
            self.digital_write(self.enable_pin, 0)
            self.digital_write(self.dir_pin, 1)
        else:
            print("the dir must be : 'forward' or 'backward'")
            self.digital_write(self.enable_pin, 1)
            return

        if (steps == 0):
            return

        print("turn step: ",steps)
        print("delay: ",stepdelay)
        while steps > 0 and self.running:
            self.digital_write(self.step_pin, GPIO.HIGH)
            time.sleep(stepdelay)
            self.digital_write(self.step_pin, False)
            time.sleep(stepdelay)
            steps -= 1


    def turn_until_switch(self, Dir, limit_switch, stepdelay):
        print("turn_until_switch")
        if (Dir == MotorDir[0]):
            print("forward")
            self.digital_write(self.enable_pin, 0)
            self.digital_write(self.dir_pin, 0)
        elif (Dir == MotorDir[1]):
            print("backward")
            self.digital_write(self.enable_pin, 0)
            self.digital_write(self.dir_pin, 1)
        else:
            print("the dir must be : 'forward' or 'backward'")
            self.digital_write(self.enable_pin, 1)
            return

        print("delay: ",stepdelay)
        pos = 0
        while GPIO.input(limit_switch) == 1 and self.running:
            self.digital_write(self.step_pin, GPIO.HIGH)
            time.sleep(stepdelay)
            self.digital_write(self.step_pin, False)
            time.sleep(stepdelay)
            pos += 1

        self.digital_write(self.enable_pin, 1)
        if Dir == MotorDir[0]:
            return pos
        else:
            return -1*pos

    def turn_check_cali(self, Dir, steps, limit_switch, stepdelay):
        print("turn_check_cali")
        if (Dir == MotorDir[0]):
            print("forward")
            self.digital_write(self.enable_pin, 0)
            self.digital_write(self.dir_pin, 0)
        elif (Dir == MotorDir[1]):
            print("backward")
            self.digital_write(self.enable_pin, 0)
            self.digital_write(self.dir_pin, 1)
        else:
            print("the dir must be : 'forward' or 'backward'")
            self.digital_write(self.enable_pin, 1)
            return

        if (steps == 0):
            return

        print("turn step: ",steps)
        print("delay: ",stepdelay)
        while steps > 0 and self.running:
            if GPIO.input(limit_switch) == 0:
                return False
            self.digital_write(self.step_pin, GPIO.HIGH)
            time.sleep(stepdelay)
            self.digital_write(self.step_pin, False)
            time.sleep(stepdelay)
            steps -= 1
        self.digital_write(self.enable_pin, 1)
        print("turn_check_cali done")
        return True
