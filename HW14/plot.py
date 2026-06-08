import serial
import numpy as np
import matplotlib.pyplot as plt

PORT = "COM10"   # adjust for your system (Windows: "COM3" etc.)
BAUD = 115200
N    = 400              # number of samples (~5 seconds at 80Hz)

with serial.Serial(PORT, BAUD, timeout=10) as ser:
    ser.reset_input_buffer()
    ser.write(f"{N}\n".encode())

    lines = []
    while len(lines) < N:
        line = ser.readline().decode().strip()
        if line:
            lines.append(line)

times, raw, filtered = [], [], []
for line in lines:
    parts = line.split()
    times.append(int(parts[0]))
    raw.append(int(parts[1]))
    filtered.append(int(parts[2]))

times    = np.array(times, dtype=float)
raw      = np.array(raw,   dtype=float)
filtered = np.array(filtered, dtype=float)

# Shift time to start at 0
times -= times[0]
times_s = times / 1000.0   # ms -> seconds

# Estimate actual sample rate from timestamps
dt_avg = np.mean(np.diff(times_s))
fs = 1.0 / dt_avg
print(f"Estimated sample rate: {fs:.1f} Hz  (Nyquist: {fs/2:.1f} Hz)")

# --- Time-domain plot ---
fig, axes = plt.subplots(2, 1, figsize=(10, 6))
axes[0].plot(times_s, raw,      label="Raw",      alpha=0.7)
axes[0].plot(times_s, filtered, label="Filtered", linewidth=2)
axes[0].set_xlabel("Time (s)")
axes[0].set_ylabel("ADC Value")
axes[0].set_title("HX711 Raw vs IIR Filtered")
axes[0].legend()
axes[0].grid(True)

# --- FFT ---
def compute_fft(signal, fs):
    n    = len(signal)
    sig  = signal - np.mean(signal)          # remove DC
    fft  = np.abs(np.fft.rfft(sig)) / n
    freq = np.fft.rfftfreq(n, d=1.0/fs)
    return freq, fft

freq_r, fft_r = compute_fft(raw,      fs)
freq_f, fft_f = compute_fft(filtered, fs)

axes[1].plot(freq_r, fft_r, label="Raw FFT",      alpha=0.7)
axes[1].plot(freq_f, fft_f, label="Filtered FFT", linewidth=2)
axes[1].axvline(fs/2, color="red", linestyle="--", label=f"Nyquist ({fs/2:.0f} Hz)")
axes[1].set_xlabel("Frequency (Hz)")
axes[1].set_ylabel("Magnitude")
axes[1].set_title("FFT of Raw vs Filtered")
axes[1].legend()
axes[1].grid(True)

plt.tight_layout()
plt.savefig("hx711_data.png", dpi=150)
plt.show()