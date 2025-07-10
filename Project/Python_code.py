import tkinter as tk
from tkinter import filedialog, messagebox
import threading
import queue
import serial
import sounddevice as sd
import numpy as np
import matplotlib
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import time
import csv

# ---- SETTINGS ----
ARDUINO_PORT = 'COM3'   # <---- Set to your actual Arduino port
ARDUINO_BAUD = 9600
AUDIO_DURATION = 0.05   # seconds (50ms for each amplitude measurement)
AUDIO_RATE = 44100

# ---- Globals ----
data_queue = queue.Queue()
recording = False
data = []  # (distance, amplitude) pairs

def serial_reader(stop_event):
    """Read distance from Arduino serial and push into queue."""
    try:
        ser = serial.Serial(ARDUINO_PORT, ARDUINO_BAUD, timeout=1)
    except Exception as e:
        data_queue.put(("error", f"Cannot open serial port: {e}"))
        return
    while not stop_event.is_set():
        try:
            line = ser.readline().decode().strip()
            if line:
                try:
                    distance = float(line)
                    data_queue.put(("distance", distance))
                except ValueError:
                    pass
        except Exception:
            pass
    ser.close()

def get_mic_rms():
    """Get RMS amplitude from mic."""
    try:
        audio = sd.rec(int(AUDIO_DURATION * AUDIO_RATE), samplerate=AUDIO_RATE, channels=1, dtype='float32')
        sd.wait()
        rms = np.sqrt(np.mean(audio ** 2))
        return float(rms)
    except Exception:
        return 0.0

class App:
    def __init__(self, root):
        self.root = root
        self.root.title("Distance vs Amplitude Recorder")
        self.fig, self.ax = plt.subplots()
        self.scat = self.ax.scatter([], [])
        self.ax.set_xlim(0, 80)
        self.ax.set_ylim(0, 1)
        self.ax.set_xlabel("Distance (cm)")
        self.ax.set_ylabel("Amplitude (RMS)")
        self.ax.set_title("Real-time Distance vs Amplitude")
        self.canvas = FigureCanvasTkAgg(self.fig, master=root)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=1)

        btn_frame = tk.Frame(root)
        btn_frame.pack(pady=8)
        self.btn_start = tk.Button(btn_frame, text="Start Recording", width=15, command=self.start)
        self.btn_stop = tk.Button(btn_frame, text="Stop Recording", width=15, command=self.stop, state=tk.DISABLED)
        self.btn_export = tk.Button(btn_frame, text="Export CSV", width=15, command=self.export, state=tk.DISABLED)
        self.btn_start.pack(side=tk.LEFT, padx=6)
        self.btn_stop.pack(side=tk.LEFT, padx=6)
        self.btn_export.pack(side=tk.LEFT, padx=6)

        self.status_var = tk.StringVar(value="Ready.")
        self.status_label = tk.Label(root, textvariable=self.status_var, anchor="w")
        self.status_label.pack(fill=tk.X)

        self.dist_val = None
        self.stop_event = threading.Event()
        self.thread = None
        self.after_id = None

    def start(self):
        global recording, data
        data.clear()
        self.ax.clear()
        self.ax.set_xlim(0, 80)
        self.ax.set_ylim(0, 1)
        self.ax.set_xlabel("Distance (cm)")
        self.ax.set_ylabel("Amplitude (RMS)")
        self.ax.set_title("Real-time Distance vs Amplitude")
        self.scat = self.ax.scatter([], [])
        self.canvas.draw()
        self.status_var.set("Recording...")
        recording = True
        self.btn_start.config(state=tk.DISABLED)
        self.btn_stop.config(state=tk.NORMAL)
        self.btn_export.config(state=tk.DISABLED)
        self.stop_event.clear()
        self.thread = threading.Thread(target=serial_reader, args=(self.stop_event,))
        self.thread.daemon = True
        self.thread.start()
        self.update_plot()

    def stop(self):
        global recording
        recording = False
        self.stop_event.set()
        self.btn_start.config(state=tk.NORMAL)
        self.btn_stop.config(state=tk.DISABLED)
        self.btn_export.config(state=tk.NORMAL)
        self.status_var.set("Stopped. You may export the data.")

    def export(self):
        if not data:
            messagebox.showwarning("Export", "No data to export.")
            return
        filename = filedialog.asksaveasfilename(defaultextension=".csv",
                                                filetypes=[("CSV Files","*.csv")])
        if filename:
            with open(filename, "w", newline="") as f:
                writer = csv.writer(f)
                writer.writerow(["Distance (cm)", "Amplitude (RMS)"])
                writer.writerows(data)
            messagebox.showinfo("Export", f"Data exported to {filename}")

    def update_plot(self):
        # Get any distance values
        try:
            while not data_queue.empty():
                msg_type, val = data_queue.get_nowait()
                if msg_type == "error":
                    self.status_var.set(val)
                    self.stop()
                    return
                elif msg_type == "distance":
                    self.dist_val = val
        except Exception:
            pass

        if recording and self.dist_val is not None:
            amplitude = get_mic_rms()
            data.append((self.dist_val, amplitude))
            xs, ys = zip(*data)
            self.ax.clear()
            self.ax.set_xlim(0, 80)
            self.ax.set_ylim(0, 1)
            self.ax.set_xlabel("Distance (cm)")
            self.ax.set_ylabel("Amplitude (RMS)")
            self.ax.set_title("Real-time Distance vs Amplitude")
            self.scat = self.ax.scatter(xs, ys)
            self.canvas.draw()
            self.status_var.set(f"Last Distance: {self.dist_val:.2f} cm | Last Amplitude: {amplitude:.3f}")

        if recording:
            self.after_id = self.root.after(50, self.update_plot)
        else:
            self.after_id = None

def main():
    root = tk.Tk()
    app = App(root)
    root.mainloop()

if __name__ == "__main__":
    main()
