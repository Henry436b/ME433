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

def compute_fft(data, Fs):
    y = np.array(data)
    n = len(y)
    frq = np.arange(n) / (n / Fs)
    frq = frq[range(int(n / 2))]
    Y = np.fft.fft(y) / n
    Y = Y[range(int(n / 2))]
    return frq, abs(Y)

def fir_filter(data, coeffs):
    return list(np.convolve(data, coeffs, mode='same'))


def load_coeffs(filename):
    with open(filename) as f:
        return [float(line.strip()) for line in f if line.strip()]

coeffs_A = load_coeffs('coeffs_A.txt')
coeffs_B = load_coeffs('coeffs_B.txt')
coeffs_D = load_coeffs('coeffs_D.txt')
coeffs_C = [1.0]# no filter

filter_info = {
    'sigA.csv': (coeffs_A, 'Blackman, fc=100Hz, bw=100Hz'),
    'sigB.csv': (coeffs_B, 'Blackman, fc=33hz, bw=33hz'),
    'sigC.csv': (coeffs_C, 'no filter'),
    'sigD.csv': (coeffs_D, 'Blackman, fc=4Hz, bw=10Hz'),
}

files = ['sigA.csv', 'sigB.csv', 'sigC.csv', 'sigD.csv']

for fname in files:
    t, data = load_csv(fname)
    Fs = len(data) / t[-1]
    coeffs, info = filter_info[fname]
    filtered = fir_filter(data, coeffs)

    frq_raw, Y_raw = compute_fft(data, Fs)
    frq_fil, Y_fil = compute_fft(filtered, Fs)

    fig, (ax1, ax2) = plt.subplots(2, 1)
    fig.suptitle(f'{fname}, NumCoeff: {len(coeffs)} Filter type: {info}')

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
    plt.savefig(fname.replace('.csv', '_plot_dev7.png'))
    plt.show()