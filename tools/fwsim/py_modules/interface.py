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
from tools.fwsim.py_modules.field import Field
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
        self.lastY = 10
        self.left = 10
        self.yStep = 10
        self.primNum = 0
        self.staticWidth = 800

        self.control = show_sim.Control(0, 0)

        self.setWindowTitle("Firmware Simulator")
        self.layout = QVBoxLayout(self)
        self.setLayout(self.layout)

        self.addWidgets()

        self.setGeometry(0, 0, self.staticWidth, self.lastY)

        self.field = Field(self)


    def addWidgets(self):
        """
        Adds all of the widges to our application.
        :return: None
        """
        # adds a reset button to reset the sliders to 0
        reset = QPushButton("Reset", self)
        reset.setGeometry(self.left, self.lastY, 150, 30)
        reset.clicked.connect(self.resetSliders)
        self.lastY += reset.height() + self.yStep
        # adds all of the input sliders
        self.addSliders()
        # adds the radios that allow the user to select how to view their simulation
        self.addRadios()
        # adds a button that builds the fwsim c code
        self.buildButton = QPushButton("Build", self)
        self.buildButton.setGeometry(self.left, self.lastY, 150, 30)
        self.buildButton.clicked.connect(self.build)
        self.lastY += self.buildButton.height() + self.yStep
        # adds the buttons that correspond to which primitive the user wants to run
        self.addPrimitives(self.left, self.lastY)


    def storeSlider(self, slider):
        """
        Stores the given slider in our list of sliders.
        :param slider: The slider to store
        :return: None
        """
        self.sliders.append(slider)

    def resetSliders(self):
        """
        Sets all of the sliders' values to 0
        :return: None
        """
        for slider in self.sliders:
            slider.setValue(0)
        self.field.reset()

    def addSliders(self):
        """
        Adds all of the input sliders to our application.
        :return: None
        """
        # final x position
        self.xInput = FinalValueSlider(self, self.lastY, self.staticWidth, "Final X Position", -3000, 3000)
        self.lastY += self.xInput.height()
        # final y position
        self.yInput = FinalValueSlider(self, self.lastY, self.staticWidth, "Final Y Position", -3000, 3000)
        self.lastY += self.yInput.height()
        # final rotation angle in degrees
        self.rotInput = FinalValueSlider(self, self.lastY, self.staticWidth, "Final Angle", -180, 180, scaling=1)
        self.lastY += self.rotInput.height()
        # final x velocity
        self.vxInput = FinalValueSlider(self, self.lastY, self.staticWidth, "Final X Velocity", -3000, 3000)
        self.lastY += self.vxInput.height()
        # final y velocity
        self.vyInput = FinalValueSlider(self, self.lastY, self.staticWidth, "Final Y Velocity", -3000, 3000)
        self.lastY += self.vyInput.height()
        # final angular velocity
        self.vrotInput = FinalValueSlider(self, self.lastY, self.staticWidth, "Final Angular Speed", -3000, 3000)
        self.lastY += self.vrotInput.height() + self.yStep

    def addRadios(self):
        """
        Adds the radios that allow the user to select how they want to visualize their data.
        :return: None
        """
        width, height = self.staticWidth / 5.1, 20
        # allows the user to animate their simulation.
        animate = Visualization("Animate", self, self.control.animate)
        animate.setGeometry(self.left, self.lastY, width, height)
        # set this one as our default checked radio
        animate.setChecked(True)
        self.visualization = animate.visualization
        # allows the user to plot xy positions on a 2D plane
        plotXY = Visualization("Plot XY", self, self.control.plotXY)
        plotXY.setGeometry(animate.geometry().right(), self.lastY, width, height)
        # allows the user to plot the angle of the robot as a function of time
        plotTheta = Visualization("Plot Angle", self, self.control.plotTheta)
        plotTheta.setGeometry(plotXY.geometry().right(), self.lastY, width, height)
        # allows the user to plot the x and y positions of the robot as functions of time
        plotXYvsT = Visualization("Plot XY vs Time", self, self.control.plotXYvsT)
        plotXYvsT.setGeometry(plotTheta.geometry().right(), self.lastY, width, height)
        # allows the user to plot the x and y velocities as a function of time
        plotVXVYvsT = Visualization("Plot Vx, Vy vs Time", self, self.control.plotVXVYvsT)
        plotVXVYvsT.setGeometry(plotXYvsT.geometry().right(), self.lastY, width, height)

        self.lastY += height + self.yStep

    def addPrimitives(self, x, y):
        """
        Add the buttons that correspond to each of the primitives
        :param x: The initial x position for the buttons in the window
        :param y: The y position for the buttons in the window
        :return: None
        """
        height = 20
        width = 75
        step = width
        for primName in self.primitives:
            primButton = PrimButton(primName, self.primitives[primName], self)
            primButton.setGeometry(x, y, width, height)
            x += step
        self.lastY += height + self.yStep

    def startInterface(self):
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
        originalText = self.buildButton.text()
        # use this so we can delay the original text being reset on the button after the build finishes. This function
        # will go on its own thread so it doesn't stop the application.
        def messager():
            time.sleep(2)
            self.buildButton.setText(originalText)
        # change the button text to be informative
        self.buildButton.setText("Building Simulator...")
        # build the code using command line
        currentLocation = os.getcwd()
        os.chdir("..")
        os.system("cmake -DCMAKE_BUILD_TYPE=Debug -Bcmake-build-debug -H.")
        os.system("cmake --build cmake-build-debug --target sim -- -j 4")
        os.chdir(currentLocation)
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
            self.primNum,
            self.xInput.value(),
            self.yInput.value(),
            self.rotInput.value() * np.pi / 180 * 100,
            self.vxInput.value(),
            self.vyInput.value(),
            self.vrotInput.value()]
        # use command line to run the program
        os.system("../sim {} {} {} {} {} {} {} {}".format(*run_args))
        # reset our control object
        self.control.__init__(self.xInput.value() / 1000, self.yInput.value() / 1000)
        # get the data into the control object
        self.control.main(output)
        # visualize the code using the function set by the radios
        self.visualization()


if __name__ == "__main__":
    QApp = QApplication(sys.argv)
    interface = Interface()
    interface.startInterface()
    sys.exit(QApp.exec_())
