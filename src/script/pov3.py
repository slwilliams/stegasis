import sys
import math
from scipy import integrate, special


file_bytes = open(sys.argv[1], "rb").read()

header = file_bytes[:2]
if header != "P6":
    print "File is not a P6 ppm..."
    sys.exit(0)

pos = 3
width = ""
while True:
    b = file_bytes[pos]
    pos += 1
    if b == " ":
        break
    width += b

height = ""
while True:
    b = file_bytes[pos]
    pos += 1
    if b == " ":
        break
    height += b

max_pixel_val = ""
while True:
    b = file_bytes[pos]
    pos += 1
    if b == " ":
        break
    max_pixel_val += b

output = [0]*100

# pos is now on the first pixel red byte
frameStart = pos
for h in range(1, 100):
    pos = frameStart
    totalPixels = math.floor((h/100.0)*int(width)*int(height))
    X = [0]*(128*3) # X[k] = frequency(2k)
    Y = [0]*(128*3) # Y[k] = frequeincy(2k+1)
    Z = [0.0]*(128*3)

    # Populate the frequency arrays
    end = pos + totalPixels*3
    while pos < end:
        b = ord(file_bytes[pos])
        if b % 2 == 0:
            X[b/2] += 1
        else:
            Y[(b-1)/2] += 1
        pos += 1

    # calculate theoretically expected frequency
    for i in range(len(Z)):
        Z[i] = (X[i] + Y[i]) / 2.0

    n = 128
    for k in range(127):
        if X[k] + Y[k] <= 4:
            X[k] = 0
            Y[k] = 0
            n -= 1

    X2 = 0.0
    for i in range(128):
        if Z[i] == 0:
            continue
        X2 += ((X[i] - Z[i])**2) / Z[i]

    p = 1.0 - special.gammainc((n-1)/2.0, X2/2.0)
    output[h] = p


for i in range(1, len(output)):
    print str(i) + " " + str(output[i])

