from PyQt5.QtWidgets import (
    QSlider,
    QLabel,
    QPushButton,
    QRadioButton
)

class FinalValueSlider(QSlider):
    HORIZONTAL = 1  # this is the number that makes the sliders horizontal
    LABEL_WIDTH = 175
    HEIGHT = 20

    class Label(QLabel):
        def __init__(self, label, scaling, interface):
            """
            A label for each of the input sliders
            :param label: the text for the label
            :param scaling: the scaling for the value being changed by the slider
            :param interface: the main application
            """
            if scaling != 1:
                self.base = "{}: {:.2f}"
            else:
                self.base = "{}: {:.0f}"
            super().__init__(self.base.format(label, 0), interface)
            self.scaling = scaling
            self.label = label


        def setText(self, value):
            """
            Set the text using the selected base
            :param value: the value of the slider
            :return: None
            """
            super().setText(self.base.format(self.label, value / self.scaling))


    def __init__(self, interface, y, width, label, minval, maxval, x=10, scaling=1000):
        """
        A slider to specify the final value for one of the inputs to the simulator.
        :param interface: The main application
        :param y: The y position in the application for this slider
        :param width: The width of the slider
        :param label: The label text
        :param minval: The minimum value
        :param maxval: The maximum value
        :param x: The x position for the slider, which will be offset by the LABEL_WIDTH
        :param scaling: The scaling for the values of the slider to be divided by
        """
        super().__init__(interface)
        sliderLabel = self.Label(label, scaling, interface)
        sliderLabel.setGeometry(x, y, self.LABEL_WIDTH, self.HEIGHT)
        self.setGeometry(x + self.LABEL_WIDTH, y, width / 1.5 - 1.1 * (x + self.LABEL_WIDTH), self.HEIGHT)
        self.setOrientation(self.HORIZONTAL)
        self.setMinimum(minval)
        self.setMaximum(maxval)
        self.valueChanged.connect(self.updateSlider)
        interface.storeSlider(self)
        self.scaling = scaling
        self.sliderLabel = sliderLabel
        self.interface = interface

    def updateSlider(self):
        """
        Update the text of the slider to display its new value, and then also update the box so that the robot is now
        position at the new location given by the slider.

        :return: None
        """
        self.sliderLabel.setText(self.value())
        rect = self.interface.field.rect
        xInput, yInput = self.interface.xInput, self.interface.yInput
        self.interface.field.lastx = rect.width() * (1 + xInput.value() / xInput.maximum()) / 2
        self.interface.field.lasty = rect.height() * (1 - yInput.value() / yInput.maximum()) / 2
        self.interface.field.update()


class PrimButton(QPushButton):
    def __init__(self, name, num, interface):
        """
        A button that corresponds to the primitive the user will want to run. When pressed, it runs the simulation
        for that primitive.
        :param name: The text to put on the button
        :param num: The primitive number that should correspond to those in primitive.c
        :param interface: The main application
        """
        super().__init__(name, interface)
        self.interface = interface
        self.num = num
        self.clicked.connect(self.run)

    def run(self):
        """
        Sets the main applications primitive number and then runs the simulation.
        :return: None
        """
        self.interface.primNum = self.num
        self.interface.run()

class Visualization(QRadioButton):
    def __init__(self, label, interface, visualization):
        """
        Specifies what kind of visualization will be used for the simulation.
        :param label: The text to go on the radio
        :param interface: The main application
        :param visualization: A function in show_sim.py that corresponds to some kind of visualization that will be done.
        """
        super().__init__(label, interface)
        self.label = label
        self.interface = interface
        self.visualization = visualization
        self.clicked.connect(self.setParentVisualization)

    def setParentVisualization(self):
        """
        Sets the visualization function in the main application.
        :return: None
        """
        self.interface.visualization = self.visualization
