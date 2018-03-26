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
        slider_label = self.Label(label, scaling, interface)
        slider_label.setGeometry(x, y, self.LABEL_WIDTH, self.HEIGHT)
        self.setGeometry(x + self.LABEL_WIDTH, y, width - 1.1 * (x + self.LABEL_WIDTH), self.HEIGHT)
        self.setOrientation(self.HORIZONTAL)
        self.setMinimum(minval)
        self.setMaximum(maxval)
        self.valueChanged.connect(lambda: slider_label.setText(self.value()))
        interface.store_slider(self)



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
        self.interface.prim_num = self.num
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
        self.clicked.connect(self.set_parent_visualization)

    def set_parent_visualization(self):
        """
        Sets the visualization function in the main application.
        :return: None
        """
        self.interface.visualization = self.visualization
