#!/usr/bin/env python3

import sys

from PyQt5 import QtCore, QtWidgets, QtGui
from PyQt5.QtWidgets import QMessageBox


class MyFileParser():
    def __init__(self, filename):
        self.lines = []

        with open(filename, "r") as file:
            for line in file:
                self.parse_line(line)

    def parse_line(self, line):
        fields = line.split(",")
        if len(fields) == 9:
            bytes = []
            for i in range(8):
                b = int(fields[i].strip(), 16)
                bytes.append(b)
            print(bytes)
            self.lines.append(bytes)

    def to_file(self, filename):
        with open(filename, "w") as file:
            for line in self.lines:
                file.write("--L\n")

    def get(self, index):
        return self.lines[index]


class MyEditor(QtWidgets.QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)

        self.file = MyFileParser("../main/glyphs.h")

        self.segments = []
        for cols in range(8):
            col = []
            for x in range(8):
                col.append(0)
            self.segments.append(col)

        self.colors = [QtCore.Qt.gray, QtCore.Qt.black]

        self.setMinimumSize(400, 400)

    @property
    def size(self):
        return int(min(self.width(), self.height()) / 8)

    def paintEvent(self, event: QtGui.QPaintEvent) -> None:
        painter = QtGui.QPainter(self)

        size = self.size

        for x, col in enumerate(self.segments):
            for y, line in enumerate(col):
                c = QtGui.QBrush(self.colors[line])
                painter.fillRect(x * size, y * size, size, size, c)

    def mousePressEvent(self, event: QtGui.QMouseEvent) -> None:
        x = int(event.pos().x() / self.size)
        y = int(event.pos().y() / self.size)
        print(event.pos(), x, y)

        self.segments[x][y] = (self.segments[x][y] + 1) % 2
        self.update()


class MainWindow(QtWidgets.QMainWindow):
    def __init__(self, parent=None):
        super().__init__(parent)

        self.quit = QtWidgets.QAction(self.tr("&Quit"))
        self.quit.setShortcut('Ctrl+Q')
        self.quit.triggered.connect(self.close)

        tool_bar = self.addToolBar(self.tr("Tools"))
        tool_bar.addAction(self.quit)

        self.editor = MyEditor()
        self.setCentralWidget(self.editor)


def main():
    app = QtWidgets.QApplication(sys.argv)

    wnd = MainWindow()
    wnd.show()
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
