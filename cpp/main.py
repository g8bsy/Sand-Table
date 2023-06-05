import MotorDriver
import time;


print(MotorDriver.init(1,2,3,4,5,6, 7,8))
print(MotorDriver.drivemotors("/Users/gabrielp/Documents/GitHub/Sand-Table/processed/max_disp=2000/STEPS_PER_REV=4000/MICROSTEP_SIZE=8/LIN_RESOLUTION=1_8step/ROT_RESOLUTION=1_4step/BurstyBezier.txt"))
print("time.sleep(20)")
time.sleep(6)
print("done")
print(MotorDriver.stopmotors())