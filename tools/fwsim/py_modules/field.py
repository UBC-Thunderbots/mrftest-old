from PyQt5.QtGui import (
    QPainter,
    QPen
)
from PyQt5.QtWidgets import (
    QWidget
)
from PyQt5.QtCore import (
    QRect
)
import random
from PyQt5.QtCore import Qt

class Field(QWidget):
    def __init__(self, interface):
        """
        A box on the GUI that represents the field. It can be clicked on to change the destination x, y positions.

        :param interface: the main application
        """
        super().__init__(interface)
        self.interface = interface
        self.setGeometry()
        self.rect = QRect(0, 0, self.geometry().width() - 1, self.geometry().height() - 1)
        self.lastx, self.lasty = self.rect.width() / 2, self.rect.height() / 2
        self.qp = QPainter()

    def setGeometry(self):
        """
        Initializes the geometry for the box based on where the sliders are so we can put it next to them. It should
        also be as tall as the position of the last slider we added to the GUI.

        :return: None
        """
        left = self.interface.sliders[0].geometry().right() + 10
        right = self.interface.geometry().right() - 10
        width = right - left
        super().setGeometry(left, 10, width, self.interface.sliders[-1].geometry().bottom())
        return self.geometry()

    def paintEvent(self, event):
        """
        Basic function that draws shapes onto the field when the self.update() function is called.

        :param event: the event from the update
        :return: None
        """
        self.qp.begin(self)
        self.drawBox()
        self.addTicks()
        self.addPoint()
        self.qp.end()

    def drawBox(self):
        """
        Draws the box onto the GUI

        :return: None
        """
        self.qp.setBrush(Qt.white)
        self.qp.drawRect(self.rect)

    def addPoint(self):
        """
        Adds a little square at the position of lastx, lasty that represents the destination of the robot.
        :return: None
        """
        self.qp.setBrush(Qt.black)
        size = 10
        shift = size / 2
        self.qp.drawRect(QRect(self.lastx - shift, self.lasty - shift, size, size))

    def addTicks(self):
        """
        Adds the axis lines onto our box every 1-meter in the x and y directions.

        :return: None
        """

        def drawGridLine(pos, x1, y1, x2, y2):
            """
            Draws a line from (x1, y1) to (x2, y2). If the position of the line lies along the y or x axis, make it red,
            otherwise make it black.

            :param pos: the position in meters to put the line at
            :param x1: the initial x coord
            :param y1: the initial y coord
            :param x2: the final x coord
            :param y2: the final y coord
            :return:
            """
            if pos == 0:
                self.qp.setPen(Qt.red)
            else:
                self.qp.setPen(Qt.black)
            self.qp.drawLine(x1, y1, x2, y2)

        # draw the y axis (vertical) lines
        xmin = int(self.interface.xInput.minimum() / self.interface.xInput.scaling)
        xmax = int(self.interface.xInput.maximum() / self.interface.xInput.scaling)
        for xPos in range(xmin, xmax + 1):
            x = int((xPos / xmax + 1) * self.rect.width()) / 2
            top = int(self.rect.height())
            drawGridLine(xPos, x, 0, x, top)

        # draw the x axis (horizontal) lines
        ymin = int(self.interface.yInput.minimum() / self.interface.yInput.scaling)
        ymax = int(self.interface.yInput.maximum() / self.interface.yInput.scaling)
        for yPos in range(ymin, ymax + 1):
            y = int((yPos / ymax + 1) * self.rect.height()) / 2
            right = int(self.rect.width())
            drawGridLine(yPos, 0, y, right, y)

    def mousePressEvent(self, QMouseEvent):
        """
        Update the position of the robot on the box when the box is clicked. We also need to update the values of the
        sliders that belong to our interface.

        :param QMouseEvent: the event from clicking the box
        :return: None
        """
        x, y = QMouseEvent.x(), QMouseEvent.y()
        scaledX = (2 * x / self.rect.width() - 1) * self.interface.xInput.maximum()
        scaledY = (1 - 2 * y / self.rect.height()) * self.interface.yInput.maximum()
        self.interface.xInput.setValue(scaledX)
        self.interface.yInput.setValue(scaledY)
        self.lastx = x
        self.lasty = y
        self.update()

    def reset(self):
        """
        Resets the field so that the box is centered again.

        :return: None
        """
        self.lastx, self.lasty = self.rect.width() / 2, self.rect.height() / 2
        self.update()


