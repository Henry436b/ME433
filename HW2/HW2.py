import board
from digitalio import DigitalInOut
import pwmio
import time

from adafruit_motor import servo

pwm_servo  = pwmio.PWMOut(board.GP14, duty_cycle = 2**15, frequency = 50)
servo1 = servo.Servo(pwm_servo, min_pulse = 500, max_pulse = 2200)


for count in range(1000000):
    servo1.angle = abs((count%360)-180)
    time.sleep(0.01)