from PyQt5.QtWidgets import (
    QApplication,
    QWidget,
    QVBoxLayout,
    QPushButton
)
import os
import sys
import numpy as np
from tools.fwsim.py_modules import show_sim
from tools.fwsim.py_modules.qt_models import (
    FinalValueSlider,
    PrimButton,
    Visualization
)
import threading
import time

class Interface(QWidget):
    # the primitive names and their corresponding numbers as specified in primitive.c
    primitives = {
        "Stop": 0,
        "Move": 1,
        "Dribble": 2,
        "Shoot": 3,
        "Catch": 4,
        "Pivot": 5,
        "Spin": 6
    }

    # a list to store all of the value sliders in
    sliders = []

    def __init__(self):
        """
        The main interface for our Qt fwsim application. Handles creating the application and any events that occur
        within the application.
        """
        super().__init__()
        self.last_y = 10
        self.left = 10
        self.y_step = 10
        self.prim_num = 0
        self.width = 1000

        self.control = show_sim.Control(0, 0)

        self.setWindowTitle("Firmware Simulator")
        self.layout = QVBoxLayout(self)
        self.setLayout(self.layout)

        self.add_widgets()

        self.setGeometry(0, 0, self.width, self.last_y)


    def add_widgets(self):
        """
        Adds all of the widges to our application.
        :return: None
        """
        # adds a reset button to reset the sliders to 0
        reset = QPushButton("Reset", self)
        reset.setGeometry(self.left, self.last_y, 150, 30)
        reset.clicked.connect(self.reset_sliders)
        self.last_y += reset.height() + self.y_step
        # adds all of the input sliders
        self.add_sliders()
        # adds the radios that allow the user to select how to view their simulation
        self.add_radios()
        # adds a button that builds the fwsim c code
        self.buildButton = QPushButton("Build", self)
        self.buildButton.setGeometry(self.left, self.last_y, 150, 30)
        self.buildButton.clicked.connect(self.build)
        self.last_y += self.buildButton.height() + self.y_step
        # adds the buttons that correspond to which primitive the user wants to run
        self.add_primitives(self.left, self.last_y)

    def store_slider(self, slider):
        """
        Stores the given slider in our list of sliders.
        :param slider: The slider to store
        :return: None
        """
        self.sliders.append(slider)

    def reset_sliders(self):
        """
        Sets all of the sliders' values to 0
        :return: None
        """
        for slider in self.sliders:
            slider.setValue(0)

    def add_sliders(self):
        """
        Adds all of the input sliders to our application.
        :return: None
        """
        # final x position
        self.x_input = FinalValueSlider(self, self.last_y, self.width, "Final X Position", -3000, 3000)
        self.last_y += self.x_input.height()
        # final y position
        self.y_input = FinalValueSlider(self, self.last_y, self.width, "Final Y Position", -3000, 3000)
        self.last_y += self.y_input.height()
        # final rotation angle in degrees
        self.rot_input = FinalValueSlider(self, self.last_y, self.width, "Final Angle", -180, 180, scaling=1)
        self.last_y += self.rot_input.height()
        # final x velocity
        self.vx_input = FinalValueSlider(self, self.last_y, self.width, "Final X Velocity", -3000, 3000)
        self.last_y += self.vx_input.height()
        # final y velocity
        self.vy_input = FinalValueSlider(self, self.last_y, self.width, "Final Y Velocity", -3000, 3000)
        self.last_y += self.vy_input.height()
        # final angular velocity
        self.vrot_input = FinalValueSlider(self, self.last_y, self.width, "Final Angular Speed", -3000, 3000)
        self.last_y += self.vrot_input.height() + self.y_step

    def add_radios(self):
        """
        Adds the radios that allow the user to select how they want to visualize their data.
        :return: None
        """
        width, height = self.width / 6, 20
        # allows the user to animate their simulation.
        animate = Visualization("Animate", self, self.control.animate)
        animate.setGeometry(self.left, self.last_y, width, height)
        # set this one as our default checked radio
        animate.setChecked(True)
        self.visualization = animate.visualization
        # allows the user to plot xy positions on a 2D plane
        plot_xy = Visualization("Plot XY", self, self.control.plot_xy)
        plot_xy.setGeometry(animate.geometry().right(), self.last_y, width, height)
        # allows the user to plot the angle of the robot as a function of time
        plot_theta = Visualization("Plot Angle", self, self.control.plot_theta)
        plot_theta.setGeometry(plot_xy.geometry().right(), self.last_y, width, height)
        # allows the user to plot the x and y positions of the robot as functions of time
        plot_xy_vs_t = Visualization("Plot XY vs Time", self, self.control.plot_xy_vs_t)
        plot_xy_vs_t.setGeometry(plot_theta.geometry().right(), self.last_y, width, height)
        # allows the user to plot the x and y velocities as a function of time
        plot_vxvy_vs_t = Visualization("Plot Vx, Vy vs Time", self, self.control.plot_vxvy_vs_t)
        plot_vxvy_vs_t.setGeometry(plot_xy_vs_t.geometry().right(), self.last_y, width, height)

        self.last_y += height + self.y_step

    def add_primitives(self, x, y):
        """
        Add the buttons that correspond to each of the primitives
        :param x: The initial x position for the buttons in the window
        :param y: The y position for the buttons in the window
        :return: None
        """
        height = 20
        width = 75
        step = width
        for prim_name in self.primitives:
            prim_button = PrimButton(prim_name, self.primitives[prim_name], self)
            prim_button.setGeometry(x, y, width, height)
            x += step
        self.last_y += height + self.y_step

    def start_interface(self):
        """
        Starts the program.
        :return: None
        """
        self.show()

    def build(self):
        """
        Builds the simulator c code
        :return: None
        """
        # store the original button text
        original_text = self.buildButton.text()
        # use this so we can delay the original text being reset on the button after the build finishes. This function
        # will go on its own thread so it doesn't stop the application.
        def messager():
            time.sleep(2)
            self.buildButton.setText(original_text)
        # change the button text to be informative
        self.buildButton.setText("Building Simulator...")
        # build the code using command line
        current_location = os.getcwd()
        os.chdir("..")
        os.system("cmake -DCMAKE_BUILD_TYPE=Debug -Bcmake-build-debug -H.")
        os.system("cmake --build cmake-build-debug --target sim -- -j 4")
        os.chdir(current_location)
        # change the button text
        self.buildButton.setText("Done!")
        # delay changing it back to the original text
        threading.Thread(target=messager).start()

    def run(self):
        """
        Run the simulator using the visualization tool specified by the radio buttons
        :return: None
        """
        # the name of the output csv
        output = "sim.csv"
        run_args = [
            output,
            self.prim_num,
            self.x_input.value(),
            self.y_input.value(),
            self.rot_input.value() * np.pi / 180 * 100,
            self.vx_input.value(),
            self.vy_input.value(),
            self.vrot_input.value()]
        # use command line to run the program
        os.system("../sim {} {} {} {} {} {} {} {}".format(*run_args))
        # reset our control object
        self.control.__init__(self.x_input.value() / 1000, self.y_input.value() / 1000)
        # get the data into the control object
        self.control.main(output)
        # visualize the code using the function set by the radios
        self.visualization()


if __name__ == "__main__":
    QApp = QApplication(sys.argv)
    interface = Interface()
    interface.start_interface()
    sys.exit(QApp.exec_())
