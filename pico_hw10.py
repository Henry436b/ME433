import machine
import time
import json
from machine import I2C, Pin

# IMU setup
I2C_PORT = I2C(0, sda=Pin(12), scl=Pin(13), freq=400000)
MPU_ADDR = 0x68

BTN = Pin(16, Pin.IN, Pin.PULL_UP)

# MPU6050 init
def imu_write(reg, val):
    I2C_PORT.writeto_mem(MPU_ADDR, reg, bytes([val]))

def imu_read(reg, n):
    return I2C_PORT.readfrom_mem(MPU_ADDR, reg, n)

def combine(hi, lo):
    val = (hi << 8) | lo
    if val >= 32768:
        val -= 65536
    return val

def mpu_init():
    who = imu_read(0x75, 1)[0]
    if who not in (0x68, 0x98):
        # hang if IMU not found
        while True:
            pass
    imu_write(0x6B, 0x00)  # wake up
    imu_write(0x1C, 0x00)  # accel +-2g
    imu_write(0x1B, 0x18)  # gyro +-2000dps

def mpu_read():
    data = imu_read(0x3B, 14)
    ax = combine(data[0], data[1]) * 0.000061
    ay = combine(data[2], data[3]) * 0.000061
    return ax, ay

mpu_init()

while True:
    ax, ay = mpu_read()
    btn = 0 if BTN.value() else 1  # active low
    packet = json.dumps({"ax": round(ax, 3), "ay": round(ay, 3), "btn": btn})
    print(packet)
    time.sleep(1/30)
