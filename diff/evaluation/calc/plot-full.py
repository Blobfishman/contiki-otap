#!/bin/python3

import numpy as np
import matplotlib.pyplot as plt
import csv
import statistics

# load data
bsdiff = []
bsdiff_x = []
fastlz = []
fastlz_x = []
uncompressed = []
uncompressed_x = []
xor = []
xor_x = []

with open('values-full.csv') as csvfile:
    reader = csv.reader(csvfile)
    for row in reader:
        bsdiff.append(int(row[0]))
        bsdiff_x.append(1)
        fastlz.append(int(row[1]))
        fastlz_x.append(2)
        uncompressed.append(int(row[2]))
        uncompressed_x.append(3)
        xor.append(int(row[3]))
        xor_x.append(4)

# calc average
bsdiff_mean = statistics.mean(bsdiff)
fastlz_mean = statistics.mean(fastlz)
uncompressed_mean = statistics.mean(uncompressed)
xor_mean = statistics.mean(xor)

# plot
fig1, ax1 = plt.subplots()
# ax1.boxplot([bsdiff, fastlz, uncompressed, xor], labels=["BSDIFF", "FASTLZ", "RAW", "XOR"])
ax1.scatter(bsdiff_x, bsdiff, label="BSDIFF")
ax1.scatter(fastlz_x, fastlz, label="FASTLZ")
ax1.scatter(uncompressed_x, uncompressed, label="RAW")
ax1.scatter(xor_x, xor, label="XOR")
# plot mean
ax1.scatter(1, bsdiff_mean, c='k', s=200, marker="_")
ax1.scatter(2, fastlz_mean, c='k', s=200, marker="_")
ax1.scatter(3, uncompressed_mean, c='k', s=200, marker="_")
ax1.scatter(4, xor_mean, c='k', s=200, marker="_")
# config
ax1.set_title('Sehr große Änderungen')
ax1.set_ylabel('Bytes')
plt.xticks(np.arange(5), ('', 'BSDIFF', 'FASTLZ', 'RAW', 'XOR'))
fig1.savefig('full.png')
