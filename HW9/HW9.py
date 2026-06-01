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

def plot_signal_fft(ax1, ax2, t, data, title):\
    
    #sample rate = number of data points / total time of samples
    # Access total time by going to last item in t list
    Fs = len(data) / t[-1] # Sample rate
    
    y = np.array(data)
    n = len(y)
    k = np.arange(n)
    frq = k / (n / Fs)
    frq = frq[range(int(n / 2))]
    Y = np.fft.fft(y) / n
    Y = Y[range(int(n / 2))]

    ax1.plot(t, data, 'b')
    ax1.set_xlabel('Time [s]')
    ax1.set_ylabel('Amplitude')
    ax1.set_title(title)

    ax2.loglog(frq, abs(Y), 'b')
    ax2.set_xlabel('Freq (Hz)')
    ax2.set_ylabel('|Y(freq)|')

files = ['sigA.csv', 'sigB.csv', 'sigC.csv', 'sigD.csv']

for fname in files:
    t, data = load_csv(fname)
    fig, (ax1, ax2) = plt.subplots(2, 1)
    plot_signal_fft(ax1, ax2, t, data, fname)
    plt.tight_layout()
    savename = fname.replace('.csv', '_plot_dev4.png')
    plt.savefig(savename)
    plt.show()