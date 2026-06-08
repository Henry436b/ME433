import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque

PORT = "COM10"
BAUD = 115200
WINDOW = 200

times  = deque(maxlen=WINDOW)
angles = deque(maxlen=WINDOW)

ser = serial.Serial(PORT, BAUD, timeout=1)

fig, ax = plt.subplots(figsize=(10, 4))
fig.suptitle("HW17 - Paddle Position")

def update(frame):
    while ser.in_waiting:
        line = ser.readline().decode().strip()
        try:
            t, ang = line.split(",")
            times.append(float(t) / 1000.0)
            angles.append(float(ang))
        except:
            pass

    if len(times) < 2:
        return

    t0 = times[0]
    ts = [t - t0 for t in times]

    ax.cla()
    ax.plot(ts, angles, color="steelblue")
    ax.set_ylabel("Angle (degrees)")
    ax.set_xlabel("Time (s)")
    ax.set_ylim(0, 360)
    ax.grid(True)
    plt.tight_layout()

ani = animation.FuncAnimation(fig, update, interval=50)
plt.show()
ser.close()