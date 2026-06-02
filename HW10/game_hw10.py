#-----------------------#
# Pico Game Curtosy of my good friend Claude Ai
#-----------------------#

import pgzrun
import serial
import json
import random
import math
import time

ser = serial.Serial('COM10', 115200, timeout=0.1)

WIDTH  = 600
HEIGHT = 600

CIRCLE_R    = 40
CURSOR_R    = 8
SPEED_STEPS = [(0.1, 0), (0.3, 2), (0.6, 5), (1.0, 10)]
WIN_SCORE   = 10

cursor_x   = WIDTH  // 2
cursor_y   = HEIGHT // 2
circle_x   = random.randint(CIRCLE_R + 20, WIDTH  - CIRCLE_R - 20)
circle_y   = random.randint(CIRCLE_R + 20, HEIGHT - CIRCLE_R - 20)
score      = 0
last_btn   = 0
start_time = None   # set on first hit
elapsed    = 0.0
game_over  = False

def accel_to_delta(a):
    mag = abs(a)
    speed = 0
    for threshold, s in SPEED_STEPS:
        if mag < threshold:
            speed = s
            break
    else:
        speed = 10
    return -speed if a < 0 else speed

def new_circle():
    global circle_x, circle_y
    circle_x = random.randint(CIRCLE_R + 20, WIDTH  - CIRCLE_R - 20)
    circle_y = random.randint(CIRCLE_R + 20, HEIGHT - CIRCLE_R - 20)

def update():
    global cursor_x, cursor_y, score, last_btn, start_time, elapsed, game_over

    if game_over:
        return

    # update stopwatch
    if start_time is not None:
        elapsed = time.time() - start_time

    try:
        raw = ser.readline().decode('utf-8').strip()
        if not raw:
            return
        data = json.loads(raw)
        ax  = float(data.get("ax", 0))
        ay  = float(data.get("ay", 0))
        btn = int(data.get("btn", 0))

        cursor_x = max(0, min(WIDTH,  cursor_x - accel_to_delta(ax)))
        cursor_y = max(0, min(HEIGHT, cursor_y - accel_to_delta(ay)))

        if btn == 1 and last_btn == 0:
            dist = math.hypot(cursor_x - circle_x, cursor_y - circle_y)
            if dist <= CIRCLE_R:
                if start_time is None:
                    start_time = time.time()
                score += 1
                if score >= WIN_SCORE:
                    elapsed = time.time() - start_time
                    game_over = True
                else:
                    new_circle()
        last_btn = btn

    except Exception:
        pass

def draw():
    screen.fill((30, 30, 30))

    if game_over:
        screen.draw.text(f"Finished!", center=(WIDTH // 2, HEIGHT // 2 - 40),
                         fontsize=56, color=(0, 255, 120))
        screen.draw.text(f"Time: {elapsed:.2f}s", center=(WIDTH // 2, HEIGHT // 2 + 20),
                         fontsize=40, color=(255, 255, 255))
        return

    # target circle
    screen.draw.circle((circle_x, circle_y), CIRCLE_R, (0, 200, 100))

    # cursor dot
    screen.draw.filled_circle((int(cursor_x), int(cursor_y)), CURSOR_R, (220, 80, 80))

    # score and stopwatch
    screen.draw.text(f"Hits: {score} / {WIN_SCORE}", (10, 10),
                     fontsize=28, color=(255, 255, 255))
    screen.draw.text(f"Time: {elapsed:.2f}s", (WIDTH - 160, 10),
                     fontsize=28, color=(255, 255, 255))

    # instructions
    screen.draw.text("Move board to steer  |  Press button when inside circle",
                     (10, HEIGHT - 25), fontsize=16, color=(150, 150, 150))

pgzrun.go()