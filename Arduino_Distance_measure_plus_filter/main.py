import serial
import time
import threading
import tkinter as tk
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import matplotlib.pyplot as plt
from collections import deque
import numpy as np

SERIAL_PORT = 'COM3'   # Adjust if needed
BAUD_RATE = 9600

MAX_POINTS = 100   # Plot this many most recent points
EMA_ALPHA = 0.3    # Set to 0 for no EMA smoothing; 0.3 is responsive
USE_MEDIAN = True  # Set True for median filter, False to disable
MEDIAN_WINDOW = 3  # Median filter window (odd number recommended, 1=no filter)

class DistancePlotter:
    def __init__(self, master):
        self.master = master
        self.master.title("Real-Time Distance Plot (Optimized)")
        self.distances = deque(maxlen=MAX_POINTS)
        self.filtered_distance = None
        self.recent_window = deque(maxlen=MEDIAN_WINDOW)
        self.running = True

        # Matplotlib figure
        self.fig, self.ax = plt.subplots(figsize=(6,3))
        self.line, = self.ax.plot([], [], 'b-')
        self.ax.set_ylim(0, 200)  # Default range, will auto-scale
        self.ax.set_xlim(0, MAX_POINTS)
        self.ax.set_ylabel('Distance (cm)')
        self.ax.set_xlabel('Sample')
        self.ax.grid(True)
        self.canvas = FigureCanvasTkAgg(self.fig, master=self.master)
        self.canvas.get_tk_widget().pack()

        # Value label
        self.label = tk.Label(master, text="Distance: -- cm", font=("Arial", 18))
        self.label.pack(pady=10)

        # Serial reading thread
        self.thread = threading.Thread(target=self.read_serial)
        self.thread.daemon = True
        self.thread.start()
        self.update_plot()

    def apply_filters(self, value):
        # Median filtering
        if USE_MEDIAN and MEDIAN_WINDOW > 1:
            self.recent_window.append(value)
            median_val = float(np.median(self.recent_window))
        else:
            median_val = value

        # EMA filtering
        if EMA_ALPHA > 0 and self.filtered_distance is not None:
            filtered = EMA_ALPHA * median_val + (1 - EMA_ALPHA) * self.filtered_distance
        else:
            filtered = median_val

        self.filtered_distance = filtered
        return filtered

    def read_serial(self):
        try:
            with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
                time.sleep(2)
                while self.running:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if not line:
                        continue
                    try:
                        value = float(line)
                        filtered = self.apply_filters(value)
                        self.distances.append(filtered)
                    except ValueError:
                        continue
        except serial.SerialException as e:
            print(f"[Error] Serial port problem: {e}")

    def update_plot(self):
        if self.distances:
            x = list(range(len(self.distances)))
            y = list(self.distances)
            self.line.set_data(x, y)
            if y:
                ymin = min(y)
                ymax = max(y)
                self.ax.set_ylim(min(0, ymin - 10), ymax + 20)  # Autoscale
            self.ax.set_xlim(0, max(MAX_POINTS, len(x)))
            self.canvas.draw()
            self.label.config(text=f"Distance: {self.distances[-1]:.2f} cm")
        self.master.after(25, self.update_plot)  # Update faster (25 ms)

    def close(self):
        self.running = False
        self.master.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    app = DistancePlotter(root)
    root.protocol("WM_DELETE_WINDOW", app.close)
    root.mainloop()
