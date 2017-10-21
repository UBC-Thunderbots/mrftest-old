import matplotlib.pyplot as plt
import os
import sys
import numpy as np

X_BALL, Y_BALL = 5, 5

f = sys.argv[1]
file = open(f)
text = file.read()
rows = text.split('\n')
fmt_rows = []
cols = rows[0].split(', ')[1:]
print(cols)
for row in rows:
    nums = row.replace(' ', '').split(',')
    to_append = []
    for x in nums:
        try:
            num = float(x)
            to_append.append(num)
        except ValueError:
            pass
    if len(to_append):
        fmt_rows.append(to_append)
arr = np.array(fmt_rows)
first_points = 5
last_points = 5

def plot_xy():
    plt.figure()
    plt.scatter(arr[:,1][:first_points], arr[:,2][:first_points], c='g', label='first {} points'.format(first_points), s=1)
    plt.scatter(arr[:,1][first_points:-last_points], arr[:,2][first_points:-last_points], c='b', label='position on global xy plane', s=1)
    plt.scatter(arr[:,1][-last_points:], arr[:,2][-last_points:], c='r', label='last {} points'.format(last_points), s=1)
    plt.scatter(X_BALL, Y_BALL, c='k', label='ball')
    for_quiver = []
    for i in range(0, len(arr), 20):
        for_quiver.append([*arr[i][1:4]])
    for_quiver = np.array(for_quiver)
    plt.quiver(for_quiver[:,0], for_quiver[:,1], np.cos(for_quiver[:,2]), np.sin(for_quiver[:,2]), width=0.0025)
    plt.xlabel('X')
    plt.ylabel('Y')
    plt.legend()

def plot_theta():
    plt.figure()
    plt.plot(arr[:,0], arr[:,3])
    plt.xlabel("T")
    plt.ylabel("Theta")

def plot_xy_vs_t():
    plt.figure()
    plt.subplot(211)
    plt.plot(arr[:,0], arr[:,1])
    plt.title('X vs T')
    plt.subplot(212)
    plt.plot(arr[:,0], arr[:,2])
    plt.title('Y vs T')
    plt.tight_layout()

def plot_vxvy_vs_t():
    plt.figure()
    plt.subplot(211)
    plt.plot(arr[:,0], arr[:,4])
    plt.title('VX vs T')
    plt.subplot(212)
    plt.plot(arr[:,0], arr[:,5])
    plt.title('VY vs T')
    plt.tight_layout()


try:
    plot_xy()
    # plot_vxvy_vs_t()

    #plot_xy_vs_t()
    # plot_theta()
    plt.show()
except IndexError:
    print("Error plotting data.")
