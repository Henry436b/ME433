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

def moving_average(data, X):
    filtered = []
    for i in range(len(data)):
        if i < X:
            filtered.append(sum(data[0:i+1]) / (i+1))
        else:
            filtered.append(sum(data[i-X+1:i+1]) / X)
    return filtered

def compute_fft(data, Fs):
    y = np.array(data)
    n = len(y)
    frq = np.arange(n) / (n / Fs)
    frq = frq[range(int(n / 2))]
    Y = np.fft.fft(y) / n
    Y = Y[range(int(n / 2))]
    return frq, abs(Y)

files  = ['sigA.csv', 'sigB.csv', 'sigC.csv', 'sigD.csv']
X_vals = {'sigA.csv': 60, 'sigB.csv': 50, 'sigC.csv': 10, 'sigD.csv': 30}  # tweak these

for fname in files:
    t, data = load_csv(fname)
    Fs = len(data) / t[-1]
    filtered = moving_average(data, X_vals[fname])

    frq_raw, Y_raw = compute_fft(data, Fs)
    frq_fil, Y_fil = compute_fft(filtered, Fs)

    fig, (ax1, ax2) = plt.subplots(2, 1)
    fig.suptitle(f'{fname}  |  X = {X_vals[fname]}')

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
    plt.savefig(fname.replace('.csv', '_plot_dev5.png'))
    plt.show()
    print(f'Saved {fname}')