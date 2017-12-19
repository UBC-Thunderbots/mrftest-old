import matplotlib.pyplot as plt
import os
import sys
import numpy as np
from matplotlib.animation import FuncAnimation
import tkinter as tk
from threading import Thread
import csv
import re

"""
This script is used to plot and/or animate robot movement from a primite. It 
loads in a 7 column CSV assumed to have the columns as
    [TIME, X, Y, THETA, VX, VY, VA]
The CSV should be fed in through sys.argv[1];
    i.e. python show_sim.py data.csv
You will need to adjust X_BALL and Y_BALL to match the ball position that you
specified in your simulation parameters.
"""
# parses constants from x and y position of the ball from the main.c file
f = open("main.c")
txt = f.readlines()
X_BALL =  1.0 # None
Y_BALL =  1.0 # None
#for line in txt:
#    if re.search("^const float X_BALL", line):
#        X_BALL = float(line.split('=')[-1].replace(' ', '').split(';')[0])
#    if re.search("^const float Y_BALL", line):
#       Y_BALL = float(line.split('=')[-1].replace(' ', '').split(';')[0])
#
#if (X_BALL is None or Y_BALL is None):
#    print("X and Y positions not found for ball.\n"
#        + "Please specify them in main.c as:\n"
#        + "    const float X_BALL = ...;\n"
#        + "    const float Y_BALL = ...;\n"
#        + "and replace ... with numbers.")
#    sys.exit()

with open(sys.argv[1]) as file:
    fmt_rows = []
    rows = csv.reader(file, delimiter=',')
    for row in rows:
        to_append = []
        for x in row:
            try:
                num = float(x)
                to_append.append(num)
            except ValueError:
                pass
        if len(to_append):
            fmt_rows.append(to_append)
arr = np.array(fmt_rows)
xmin = min(X_BALL, np.min(arr[:,1])) - 1
xmax = max(X_BALL, np.max(arr[:,1])) + 1
ymin = min(Y_BALL, np.min(arr[:,2])) - 1
ymax = max(Y_BALL, np.max(arr[:,2])) + 1
first_points, last_points = 5, 5

def main():
    """Select your plot in here."""
    try:
        plot_xy()
        plot_vxvy_vs_t()
        plot_xy_vs_t()
        # plot_theta()
        anim = Animate()
        anim.animate_xy()
        plt.show()
    except IndexError:
        print("Error plotting data.")

def create_quiver(skip=20):
    """Creates a set of quivers to be plotted."""
    for_quiver = []
    for i in range(0, len(arr), skip):
        for_quiver.append([*arr[i][1:4]])
    for_quiver = np.array(for_quiver)
    return for_quiver

def plot_xy():
    """Plots the xy positions of the bot and the ball."""
    fig, ax = plt.subplots()
    ax.scatter(
        arr[:,1][:first_points], 
        arr[:,2][:first_points], 
        c='g', 
        label='first {} points'.format(first_points), 
        s=1)
    ax.scatter(
        arr[:,1][first_points:-last_points], 
        arr[:,2][first_points:-last_points], 
        c='b', 
        label='position on global xy plane', 
        s=1)
    ax.scatter(
        arr[:,1][-last_points:], 
        arr[:,2][-last_points:], 
        c='r', 
        label='last {} points'.format(last_points), 
        s=1)
    ax.scatter(X_BALL, Y_BALL, c='k', label='ball')
    for_quiver = create_quiver()
    ax.quiver(
        for_quiver[:,0], 
        for_quiver[:,1], 
        np.cos(for_quiver[:,2]), 
        np.sin(for_quiver[:,2]), 
        width=0.0025)
    ax.set_xlim(xmin, xmax)
    ax.set_ylim(ymin, ymax)

def plot_theta():
    """Plot theta relative to global x-axis as a function of time"""
    plt.figure()
    plt.plot(arr[:,0], arr[:,3])
    plt.xlabel("T")
    plt.ylabel("Theta")

def plot_xy_vs_t():
    """Plot x and y as a function of time on separate plots"""
    plt.figure()
    plt.subplot(211)
    plt.plot(arr[:,0], arr[:,1])
    plt.title('X vs T')
    plt.subplot(212)
    plt.plot(arr[:,0], arr[:,2])
    plt.title('Y vs T')
    plt.tight_layout()

def plot_vxvy_vs_t():
    """Plot V_x and V_y as a function of time on separate plots"""
    plt.figure()
    plt.subplot(211)
    plt.plot(arr[:,0], arr[:,4])
    plt.title('VX vs T')
    plt.subplot(212)
    plt.plot(arr[:,0], arr[:,5])
    plt.title('VY vs T')
    plt.tight_layout()

class Animate():
    """
    Manages a controllable Matplotlib animation utility

    Attributes:
        index (int): The index of the current frame in the animation.
        playing (boolean): True if the animation is playing, false otherwise.
        ball_size (float): The size of the ball on the plot.
        size (float): The size of the bot on the plot.
    """
    def __init__(self):
        self.index = 0
        self.playing = True
        self.ball_size = 20
        self.size = self.ball_size * 8.5

    def animate_xy(self):
        """
        Initializes the plot with scatter and quiver data then starts the 
        animation. Also binds mouse and key events to the figure canvas.
        """
        self.fig, ax = plt.subplots()
        self.for_quiver = create_quiver(skip=1)
        self.scat = ax.scatter(arr[:,1][0], arr[:,2][0], s=self.size)
        self.quiv = plt.quiver(
            self.for_quiver[0][0], 
            self.for_quiver[0][1], 
            np.cos(self.for_quiver[0][2]), 
            np.sin(self.for_quiver[0][2]))
        ax.scatter(X_BALL, Y_BALL, c='k', label="ball", s=self.ball_size)
        ax.set_xlim(xmin, xmax)
        ax.set_ylim(ymin, ymax)
        self.t = ax.text(ax.get_xlim()[0], ax.get_ylim()[1], "0.0 seconds")
        self.animation = FuncAnimation(
            self.fig, 
            self.update, 
            interval=1, 
            frames=len(arr) - 1)
        self.fig.canvas.mpl_connect("scroll_event", self.move)
        self.fig.canvas.mpl_connect("key_press_event", self.playpause)
        self.fig.canvas.mpl_connect("key_press_event", self.restart)
        plt.legend()
        plt.show()

    def move(self, e):
        """
        Moves the bot on the plot a frame.
            Moves forward in time if the mouse is scrolled up.
            Moves backwards in time if the mouse is scrolled down.
        Updates the plot.

        Args:
            e (Matplotlib event): MouseEvent object with info from the scroll.
        """
        if e.button == 'up':
            self.index += 1
            self.update(self.index, True)
        else:
            self.index -= 1
            self.update(self.index, True)

    def playpause(self, e):
        """
        If the spacebar is pressed, controls the playing state of the animation.
            If the animation is playing, it is stopped and self.playing is set
            to False.
            Else, the animation is started and self.playing is set to True.

        Args:
            e (Matplotlib event): KeyEvent object with info from the press.
        """
        if (e.key == ' '):
            if (self.playing):
                self.animation.event_source.stop()
                self.playing = False
            else:
                self.animation.event_source.start()
                self.playing = True

    def restart(self, e):
        """
        Restarts the animation when the r key is pressed.

        Args:
            e (Matplotlib event): KeyEvent object with info from the press.
        """
        if (e.key == 'r'):
            self.index = 0
            self.update(self.index, True)

    def update(self, i, is_slider=False):
        """
        Updates the animation. 
        Constrain the self.index to be less then len(arr) - 1 and update the 
        quiver and scatter plots with the next point in the array. Also set
        the text label to update the time of the simulation.
        
        If is_slider is True, then it is assumed that the user is scrolling
        through the animation so the canvas should be redrawn.
        Else, the index should be incremented to match the animation index.

        Args:
            i (int): The zero-index for the animation to get data from the 
                arrays.
            is_slider (boolean, Optional): Determines whether the user is 
                scrolling or not.
        """
        self.index = max(0, min(self.index, len(arr) - 1))
        self.quiv.set_UVC(
            np.cos(self.for_quiver[self.index][2]), 
            np.sin(self.for_quiver[self.index][2]))
        self.quiv.set_offsets(
            [self.for_quiver[self.index][0], 
             self.for_quiver[self.index][1]])
        self.scat.set_offsets(
            [arr[:,1][self.index], 
             arr[:,2][self.index]])
        self.t.set_text('{:.1f} seconds'.format(arr[:,0][self.index]))
        if is_slider:
            self.fig.canvas.draw()
        else:
            self.index += 1

if __name__ == '__main__':
    main()

