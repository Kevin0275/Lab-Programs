import tkinter as tk
from tkinter import filedialog, messagebox, simpledialog
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
ARDUINO_PORT = 'COM4'   # <---- Set to your actual Arduino port
ARDUINO_BAUD = 9600
AUDIO_DURATION = 0.05   # seconds (50ms for each amplitude measurement)
AUDIO_RATE = 44100

# ---- Globals ----
data_queue = queue.Queue()
recording = False
data = []  # (distance, amplitude) pairs
mic_device_index = None

def list_input_devices():
    devices = sd.query_devices()
    result = []
    for i, dev in enumerate(devices):
        if dev['max_input_channels'] > 0:
            result.append(f"{i}: {dev['name']} (Channels: {dev['max_input_channels']})")
    return "\n".join(result)

def prompt_mic_index(root):
    devices = sd.query_devices()
    device_list = [f"{i}: {dev['name']} (Channels: {dev['max_input_channels']})"
                   for i, dev in enumerate(devices) if dev['max_input_channels'] > 0]
    msg = "Available Input Devices:\n" + "\n".join(device_list) + "\n\nEnter microphone device index:"
    idx = None
    while True:
        idx = simpledialog.askinteger("Select Microphone", msg, parent=root)
        if idx is None:
            messagebox.showerror("Error", "Microphone selection cancelled. Exiting.")
            root.quit()
            exit()
        try:
            dev = sd.query_devices(idx)
            if dev['max_input_channels'] > 0:
                return idx
        except Exception:
            pass
        messagebox.showerror("Error", f"Index {idx} is not a valid input device. Try again.")

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
    try:
        audio = sd.rec(int(AUDIO_DURATION * AUDIO_RATE), samplerate=AUDIO_RATE,
                       channels=1, dtype='float32', device=mic_device_index)
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
    global mic_device_index
    root = tk.Tk()
    root.withdraw()  # Hide window while selecting mic
    mic_device_index = prompt_mic_index(root)
    root.deiconify()
    app = App(root)
    root.mainloop()

if __name__ == "__main__":
    main()
