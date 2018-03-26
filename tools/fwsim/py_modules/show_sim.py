import matplotlib.pyplot as plt
import sys
import numpy as np
from matplotlib.animation import FuncAnimation
import csv
import re

class Control:
    xmin, ymin = -3, -3
    xmax, ymax = 3, 3

    def __init__(self, x, y):
        self.x = x
        self.y = y
        self.animator = Animate(self)

    def animate(self):
        self.animator.animate_xy()
        try:
            plt.show()
        except:
            pass

    def main(self, file):
        """Select your plot in here."""
        arr = np.float_(np.loadtxt(file, delimiter=",", dtype=str)[1:][:,1:])
        self.first_points, self.last_points = 5, 5
        self.arr = arr

    def create_quiver(self, skip=10):
        """Creates a set of quivers to be plotted."""
        for_quiver = []
        for i in range(0, len(self.arr), skip):
            if not np.any(np.isnan(self.arr[i])):
                for_quiver.append([*self.arr[i][1:4]])
            else:
                for_quiver.append([0, 0, 0])
        for_quiver = np.array(for_quiver)
        return for_quiver

    def plot_xy(self):
        """Plots the xy positions of the bot and the ball."""
        fig, ax = plt.subplots()
        ax.scatter(
            self.arr[:,1][:self.first_points],
            self.arr[:,2][:self.first_points],
            c='g',
            label='First {} points'.format(self.first_points),
            s=1)
        ax.scatter(
            self.arr[:,1][self.first_points:-self.last_points],
            self.arr[:,2][self.first_points:-self.last_points],
            c='b',
            label='Position on global xy plane',
            s=1)
        ax.scatter(
            self.arr[:,1][-self.last_points:],
            self.arr[:,2][-self.last_points:],
            c='r',
            label='Last {} points'.format(self.last_points),
            s=1)
        ax.scatter(self.x, self.y, c='k', label='Destination')
        for_quiver = self.create_quiver()
        ax.quiver(
            for_quiver[:,0],
            for_quiver[:,1],
            np.cos(for_quiver[:,2]),
            np.sin(for_quiver[:,2]),
            width=0.0025)
        ax.set_xlim(self.xmin, self.xmax)
        ax.set_ylim(self.ymin, self.ymax)
        plt.legend()
        plt.title("XY Position")
        plt.xlabel("x")
        plt.ylabel("y")
        plt.show()

    def plot_theta(self):
        """Plot theta relative to global x-axis as a function of time"""
        plt.figure()
        plt.plot(self.arr[:,0], self.arr[:,3] * 180 / np.pi)
        plt.xlabel("t (seconds)")
        plt.ylabel(r"$\theta(t) (degrees)$")
        plt.title("Robot Angle")
        plt.show()

    def plot_xy_vs_t(self):
        """Plot x and y as a function of time on separate plots"""
        plt.figure()
        plt.subplot(211)
        plt.plot(self.arr[:,0], self.arr[:,1])
        plt.title('X Position')
        plt.xlabel("t (seconds)")
        plt.ylabel("x(t) (meters)")
        plt.subplot(212)
        plt.plot(self.arr[:,0], self.arr[:,2])
        plt.title('Y Position')
        plt.xlabel("t (seconds)")
        plt.ylabel("y(t) (meters)")
        plt.tight_layout()
        plt.show()

    def plot_vxvy_vs_t(self):
        """Plot V_x and V_y as a function of time on separate plots"""
        plt.figure()
        plt.subplot(211)
        plt.plot(self.arr[:,0], self.arr[:,4])
        plt.title('X Velocity')
        plt.xlabel("t (seconds)")
        plt.ylabel("vx(t) (meters)")
        plt.subplot(212)
        plt.plot(self.arr[:,0], self.arr[:,5])
        plt.title('Y Velocity')
        plt.xlabel("t (seconds)")
        plt.ylabel("vy(t) (meters)")
        plt.tight_layout()
        plt.show()

class Animate():
    """
    Manages a controllable Matplotlib animation utility

    Attributes:
        index (int): The index of the current frame in the animation.
        playing (boolean): True if the animation is playing, false otherwise.
        ball_size (float): The size of the ball on the plot.
        size (float): The size of the bot on the plot.
    """
    def __init__(self, control):
        self.index = 0
        self.playing = True
        self.ball_size = 20
        self.size = self.ball_size * 8.5
        self.control = control

    def animate_xy(self):
        """
        Initializes the plot with scatter and quiver data then starts the
        animation. Also binds mouse and key events to the figure canvas.
        """
        self.fig, ax = plt.subplots()
        self.for_quiver = self.control.create_quiver(skip=1)
        self.scat = ax.scatter(self.control.arr[:,1][0], self.control.arr[:,2][0], s=self.size)
        self.quiv = plt.quiver(
            self.for_quiver[0][0],
            self.for_quiver[0][1],
            np.cos(self.for_quiver[0][2]),
            np.sin(self.for_quiver[0][2]))
        ax.scatter(self.control.x, self.control.y, c='k', label="ball", s=self.ball_size)
        ax.set_xlim(self.control.xmin, self.control.xmax)
        ax.set_ylim(self.control.ymin, self.control.ymax)
        self.t = ax.text(ax.get_xlim()[0], ax.get_ylim()[1], "0.0 seconds")
        self.animation = FuncAnimation(
            self.fig,
            self.update,
            interval=1,
            frames=len(self.control.arr) - 1)
        self.fig.canvas.mpl_connect("scroll_event", self.move)
        self.fig.canvas.mpl_connect("key_press_event", self.playpause)
        self.fig.canvas.mpl_connect("key_press_event", self.restart)
        self.fig.canvas.mpl_connect('close_event', self.close_plots)

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
        self.index = max(0, min(self.index, len(self.control.arr) - 1))
        self.quiv.set_UVC(
            np.cos(self.for_quiver[self.index][2]),
            np.sin(self.for_quiver[self.index][2]))
        self.quiv.set_offsets(
            [self.for_quiver[self.index][0],
             self.for_quiver[self.index][1]])
        self.scat.set_offsets(
            [self.control.arr[:,1][self.index],
             self.control.arr[:,2][self.index]])
        self.t.set_text('{:.1f} seconds'.format(self.control.arr[:,0][self.index]))
        if is_slider:
            self.fig.canvas.draw()
        else:
            self.index += 1
        plt.legend()
        plt.title("Robot Motion")
        plt.xlabel("x")
        plt.ylabel("y")

    def close_plots(self, e):
        self.playing = False
        plt.close()

