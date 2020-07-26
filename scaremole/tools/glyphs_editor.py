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

    def size(self):
        return len(self.lines)


class MyEditor(QtWidgets.QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)

        self.file = MyFileParser("../main/glyphs.h")

        self.segments = []
        for s in range(8):
            col = []
            for m in [0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01]:
                col.append(0)
            self.segments.append(col)

        self.colors = [QtCore.Qt.gray, QtCore.Qt.black]

        self.setMinimumSize(400, 400)

        self.glyph_index = 2
        self.read_glyph(self.glyph_index)

    def read_glyph(self, index):
        glyph = self.file.get(index)
        self.glyph_index = index

        for s in range(8):
            g = glyph[s]
            for x, m in enumerate([0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01]):
                self.segments[s][x] = (g & m) == m

        self.update()

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

    def wheelEvent(self, event: QtGui.QWheelEvent) -> None:
        if event.angleDelta().y() < 0:
            if self.glyph_index == 0:
                return
            self.glyph_index -= 1
        else:
            if self.glyph_index + 1 >= self.file.size():
                return
            self.glyph_index += 1

        self.read_glyph(self.glyph_index)


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
