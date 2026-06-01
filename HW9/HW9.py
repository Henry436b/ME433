import csv

t = [] # column 0
data = [] # column 1

with open('sigA.csv') as f:
    # open the csv file
    reader = csv.reader(f)
    for row in reader:
        # read the rows 1 one by one
        t.append(float(row[0])) # leftmost column
        data.append(float(row[1])) # second column

# sample rate
sample_rate = len(t) / t[-1]
print("Sample rate:", sample_rate, "Hz")

for i in range(len(t)):
    print(str(t[i]) + ", " + str(data[i]))



