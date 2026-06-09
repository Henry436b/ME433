"""
Haptic paddle — live tuning monitor
Reads the serial stream from main.c:
  printf("%.2f,%.1f,%.1f,%d,%ld\n", position_deg, desired_current, actual_current, pwm_duty, hx_force);

Displays:
  - Force vs time       (desired + actual current)
  - Load cell force vs time
  - Force vs position   (phase plot)

Usage:
  pip install pyserial matplotlib
  python tuning_monitor.py
"""

import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque

# ── Config ────────────────────────────────────────────────────────────────────
PORT      = 'COM10'
BAUD      = 115200
WINDOW    = 200
PHASE_MAX = 500

WALL_LOW  = 45.0
WALL_HIGH = 110.0

# ── Buffers ───────────────────────────────────────────────────────────────────
positions       = deque([0.0] * WINDOW, maxlen=WINDOW)
desired_forces  = deque([0.0] * WINDOW, maxlen=WINDOW)
actual_currents = deque([0.0] * WINDOW, maxlen=WINDOW)
hx_forces       = deque([0.0] * WINDOW, maxlen=WINDOW)

phase_pos   = deque(maxlen=PHASE_MAX)
phase_force = deque(maxlen=PHASE_MAX)

# ── Serial ────────────────────────────────────────────────────────────────────
ser = serial.Serial(PORT, BAUD, timeout=1)
print(f'Opened {ser.name}')

# ── Layout ────────────────────────────────────────────────────────────────────
fig = plt.figure(figsize=(14, 8))
fig.suptitle('Haptic paddle — tuning monitor', fontsize=13)

ax_force  = fig.add_subplot(2, 3, 1)   # top left
ax_hx     = fig.add_subplot(2, 3, 2)   # top middle
ax_pos    = fig.add_subplot(2, 3, 3)   # top right
ax_phase  = fig.add_subplot(2, 1, 2)   # full bottom row

# ── Force vs time ─────────────────────────────────────────────────────────────
ax_force.set_title('Motor force vs time', fontsize=10)
ax_force.set_ylabel('mA')
ax_force.set_ylim(-550, 550)
ax_force.axhline(0, color='gray', linewidth=0.5)

t_axis = list(range(WINDOW))
line_desired, = ax_force.plot(t_axis, list(desired_forces),  color='steelblue',     lw=1.2, label='desired')
line_actual,  = ax_force.plot(t_axis, list(actual_currents), color='mediumseagreen', lw=1.0, linestyle='--', label='actual')
ax_force.legend(fontsize=8, loc='upper right')

# ── Load cell force vs time ───────────────────────────────────────────────────
ax_hx.set_title('Load cell force vs time', fontsize=10)
ax_hx.set_ylabel('raw counts')
ax_hx.set_ylim(-50000, 50000)
ax_hx.axhline(0, color='gray', linewidth=0.5)

line_hx, = ax_hx.plot(t_axis, list(hx_forces), color='coral', lw=1.2)

# ── Position vs time ─────────────────────────────────────────────────────────
ax_pos.set_title('Position vs time', fontsize=10)
ax_pos.set_ylabel('degrees')
ax_pos.set_ylim(0, 180)

line_pos, = ax_pos.plot(t_axis, list(positions), color='tomato', lw=1.2)

# ── Force vs position (phase plot) ───────────────────────────────────────────
ax_phase.set_title('Force vs position  (phase plot)', fontsize=10)
ax_phase.set_xlabel('position (°)')
ax_phase.set_ylabel('desired force (mA)')
ax_phase.set_xlim(0, 180)
ax_phase.set_ylim(-550, 550)
ax_phase.axhline(0, color='gray', linewidth=0.5)
ax_phase.axvline(WALL_LOW,  color='darkorange', linewidth=1, linestyle='--', label=f'wall low  {WALL_LOW}°')
ax_phase.axvline(WALL_HIGH, color='darkorange', linewidth=1, linestyle='--', label=f'wall high {WALL_HIGH}°')
ax_phase.legend(fontsize=8, loc='upper right')

scatter_phase, = ax_phase.plot([], [], color='mediumpurple', lw=0.8,
                                alpha=0.6, marker='o', markersize=1.5)

plt.tight_layout()

# ── Parse ─────────────────────────────────────────────────────────────────────
def read_sample():
    try:
        line = ser.readline().decode('ascii', errors='ignore').strip()
        parts = line.split(',')
        if len(parts) == 5:
            pos     = float(parts[0])
            desired = float(parts[1])
            actual  = float(parts[2])
            pwm     = int(parts[3])
            hx      = float(parts[4])
            return pos, desired, actual, pwm, hx
        elif len(parts) == 4:
            # backwards compat if HX711 not yet in firmware
            pos     = float(parts[0])
            desired = float(parts[1])
            actual  = float(parts[2])
            pwm     = int(parts[3])
            return pos, desired, actual, pwm, 0.0
    except (ValueError, serial.SerialException):
        return None

# ── Animate ───────────────────────────────────────────────────────────────────
def update(_frame):
    for _ in range(5):
        sample = read_sample()
        if sample is None:
            break
        pos, desired, actual, pwm, hx = sample
        positions.append(pos)
        desired_forces.append(desired)
        actual_currents.append(actual)
        hx_forces.append(hx)
        phase_pos.append(pos)
        phase_force.append(desired)

    line_desired.set_ydata(list(desired_forces))
    line_actual.set_ydata(list(actual_currents))
    line_pos.set_ydata(list(positions))
    line_hx.set_ydata(list(hx_forces))
    scatter_phase.set_data(list(phase_pos), list(phase_force))

    # auto-scale load cell y axis to actual data range
    hx_list = list(hx_forces)
    hx_min, hx_max = min(hx_list), max(hx_list)
    margin = max(abs(hx_min), abs(hx_max)) * 0.2 or 5000
    ax_hx.set_ylim(hx_min - margin, hx_max + margin)

    return line_desired, line_actual, line_pos, line_hx, scatter_phase

ani = animation.FuncAnimation(fig, update, interval=40, blit=False, cache_frame_data=False)

try:
    plt.show()
finally:
    ser.close()
    print('Serial port closed.')