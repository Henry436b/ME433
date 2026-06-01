import csv
import numpy as np
import matplotlib.pyplot as plt

def load_csv(filename):
    t, data = [], []
    with open(filename) as f:
        reader = csv.reader(f)
        for row in reader:
            t.append(float(row[0]))
            data.append(float(row[1]))
    return t, data

def iir_filter(data, A, B):
    filtered = [data[0]]  # start with first value
    for i in range(1, len(data)):
        filtered.append(A * filtered[i-1] + B * data[i])
    return filtered

# tweak these
AB_vals = {
    'sigA.csv': (0.95, 0.05),
    'sigB.csv': (0.9, 0.1),
    'sigC.csv': (0.9, 0.1),
    'sigD.csv': (0.9, 0.1)
}

def compute_fft(data, Fs):
    y = np.array(data)
    n = len(y)
    frq = np.arange(n) / (n / Fs)
    frq = frq[range(int(n / 2))]
    Y = np.fft.fft(y) / n
    Y = Y[range(int(n / 2))]
    return frq, abs(Y)

files  = ['sigA.csv', 'sigB.csv', 'sigC.csv', 'sigD.csv']
X_vals = {'sigA.csv': 60, 'sigB.csv': 50, 'sigC.csv': 10, 'sigD.csv': 30}

for fname in files:
    t, data = load_csv(fname)
    Fs = len(data) / t[-1]
    A, B = AB_vals[fname]
    filtered = iir_filter(data, A, B)

    frq_raw, Y_raw = compute_fft(data, Fs)
    frq_fil, Y_fil = compute_fft(filtered, Fs)

    fig, (ax1, ax2) = plt.subplots(2, 1)
    fig.suptitle(f'{fname}  |  A={A}  B={B}')

    ax1.plot(t, data, 'k', label='raw')
    ax1.plot(t, filtered, 'r', label='filtered')
    ax1.set_xlabel('Time [s]')
    ax1.set_ylabel('Amplitude')
    ax1.legend()

    ax2.loglog(frq_raw, Y_raw, 'k', label='raw')
    ax2.loglog(frq_fil, Y_fil, 'r', label='filtered')
    ax2.set_xlabel('Freq (Hz)')
    ax2.set_ylabel('|Y(freq)|')
    ax2.legend()

    plt.tight_layout()
    plt.savefig(fname.replace('.csv', '_plot_dev6.png'))
    plt.show()