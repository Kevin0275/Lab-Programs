import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque
import time
import re

# ---------------------
# CONFIGURATION
# ---------------------
PORT = 'COM3'           # ← Change this to your Arduino's port
BAUD = 9600             # Match the Arduino Serial.begin()
DURATION = 10           # 10 seconds sliding window

# ---------------------
# INITIALIZE
# ---------------------
ser = serial.Serial(PORT, BAUD, timeout=1)
time.sleep(2)  # Allow time for Arduino to reset

# Buffers to store time and distance
times = deque()
distances = deque()

# Track start time
start_time = time.time()

# Pattern to match the Arduino output
pattern = re.compile(r'Filtered Distance:\s*([\d.]+)\s*cm')

# ---------------------
# PLOT UPDATE FUNCTION
# ---------------------
def update_plot(frame):
    global start_time

    current_time = time.time()
    elapsed = current_time - start_time

    try:
        line = ser.readline().decode('utf-8').strip()
        match = pattern.search(line)
        if match:
            distance = float(match.group(1))
            times.append(elapsed)
            distances.append(distance)

            # Trim to keep only last DURATION seconds
            while times and (elapsed - times[0] > DURATION):
                times.popleft()
                distances.popleft()

    except Exception as e:
        print("Read error:", e)

    # Plotting
    ax.clear()
    ax.plot(times, distances, label='Distance (cm)', color='blue')
    ax.set_xlim(max(0, elapsed - DURATION), elapsed)
    ax.set_ylim(0, max(100, max(distances, default=10) + 10))  # Auto scale Y
    ax.set_xlabel("Time (s)")
    ax.set_ylabel("Distance (cm)")
    ax.set_title("Live Distance vs Time")
    ax.grid(True)
    ax.legend()

# ---------------------
# LAUNCH PLOT
# ---------------------
fig, ax = plt.subplots()
ani = animation.FuncAnimation(fig, update_plot, interval=50)
plt.tight_layout()
plt.show()

# ---------------------
# CLEANUP
# ---------------------
ser.close()
