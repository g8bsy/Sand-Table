import RPi.GPIO as GPIO
from utils.DRV8825 import DRV8825
import threading
from time import sleep
import const

GPIO.setmode(GPIO.BCM)
GPIO.setup(const.MOTOR_RELAY_PIN, GPIO.OUT)

from motors import M_Lin, M_Rot


class MotorThreads:

    def __init__(self):
        self.running = True

    def run_MRot(self, delay, step_size):
        M_Rot.set_microstep('software', step_size)

        while self.running:
            M_Rot.turn_steps(Dir='forward', steps=1, stepdelay=delay)

        M_Rot.stop()

    def run_MLin(self, delay, step_size):
        M_Lin.set_microstep('software', step_size)

        M_Lin.turn_steps(Dir='forward', steps=5000, stepdelay=delay)
        sleep(0.1)
        M_Lin.turn_steps(Dir='backward', steps=5000, stepdelay=delay)

        # for s in range(100):
        #     d_new = (delay * (1 + s)) / 100
        #     print(d_new)
        #     M_Lin.turn_steps(Dir='forward', steps=50, stepdelay=d_new)
        # sleep(0.1)
        # for s in range(100):
        #     d_new = (delay * (1 + s)) / 100
        #     M_Lin.turn_steps(Dir='backward', steps=50, stepdelay=d_new)

        M_Lin.stop()
        self.running = False

delays = [0.0001, 0.0002]

try:
    GPIO.output(const.MOTOR_RELAY_PIN, GPIO.LOW)
    motors = MotorThreads()
    for d in delays:
        print("Delays: {}".format(d))

        MRot = threading.Thread(target=motors.run_MRot, args=(d, const.ROT_RESOLUTION,))
        MLin = threading.Thread(target=motors.run_MLin, args=(d, const.LIN_RESOLUTION,))

        print("...")
        MRot.start()
        MLin.start()

        MRot.join()
        MLin.join()

        motors.running = True
except KeyboardInterrupt:
    print("\nMotors stopped")
    M_Rot.stop()
    M_Lin.stop()
    GPIO.cleanup()
    print("Exiting...")
    exit()
