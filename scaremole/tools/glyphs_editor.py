#!/usr/bin/env python3

import sys

from PyQt5 import QtCore, QtWidgets, QtGui
from PyQt5.QtWidgets import QMessageBox


class MyFileParser:
    def __init__(self, filename):
        self.glyphs = []
        self.filename = filename

        with open(filename, "r") as file:
            for line in file:
                self.parse_line(line)

    def parse_line(self, line):
        fields = line.split(",")
        if len(fields) == 9:
            b8 = []
            for i in range(8):
                b = int(fields[i].strip(), 16)
                b8.append(b)
            print(b8)
            self.glyphs.append(b8)

    def to_file(self, filename=None):
        lines = []

        lines.append("#ifndef PROJECT_GLYPH_H")
        lines.append("#define PROJECT_GLYPH_H")
        lines.append("")
        lines.append("const uint8_t glyph[] = {")

        for glyphs in self.glyphs:
            line = ", ".join(f"0x{n:02x}" for n in glyphs)
            lines.append(f"\t{line},")

        lines.append("};")
        lines.append("#endif\n")

        if filename is None:
            filename = self.filename

        with open(filename, "w") as file:
            file.write("\n".join(lines))

        print("--saved to file", filename)

    def get(self, index):
        return self.glyphs[index]

    def set(self, index, glyph):
        self.glyphs[index] = glyph
#        self.to_file(self.filename)

    def size(self):
        return len(self.glyphs)


class MyEditor(QtWidgets.QWidget):
    selectionChanged = QtCore.pyqtSignal()

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

        self.glyph_index = -1
        self.read_glyph(2)

    def read_glyph(self, index):
        if self.glyph_index != -1:
            self.save_glyph()

        glyph = self.file.get(index)
        self.glyph_index = index

        for s in range(8):
            g = glyph[s]
            for x, m in enumerate([0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01]):
                self.segments[s][x] = (g & m) == m

        self.update()
        self.selectionChanged.emit()

        print("--read-g", glyph)
        self.save_glyph()
        glyph = self.file.get(index)
        print("--Read-g", glyph)

    def save_glyph(self):
        print("--save", self.glyph_index)

        glyph = []
        for s in range(8):
            b8 = 0
            for x, m in enumerate([0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01]):
                if self.segments[s][x]:
                    b8 |= m
            glyph.append(b8)

        self.file.set(self.glyph_index, glyph)

    def save(self):
        if self.glyph_index != -1:
            self.save_glyph()
        self.file.to_file()

    @property
    def tile_size(self):
        return int(min(self.width(), self.height()) / 8)
    
    @property
    def num_glyphs(self):
        return self.file.size()

    def paintEvent(self, event: QtGui.QPaintEvent) -> None:
        painter = QtGui.QPainter(self)

        size = self.tile_size

        for x, col in enumerate(self.segments):
            for y, line in enumerate(col):
                c = QtGui.QBrush(self.colors[line])
                painter.fillRect(x * size, y * size, size, size, c)

    def mousePressEvent(self, event: QtGui.QMouseEvent) -> None:
        x = int(event.pos().x() / self.tile_size)
        y = int(event.pos().y() / self.tile_size)
        print(event.pos(), x, y)

        self.segments[x][y] = (self.segments[x][y] + 1) % 2
        self.update()

    def wheelEvent(self, event: QtGui.QWheelEvent) -> None:
        index = self.glyph_index
        if event.angleDelta().y() > 0:
            if index == 0:
                return
            index -= 1
        else:
            if index + 1 >= self.file.size():
                return
            index += 1

        self.read_glyph(index)


class MainWindow(QtWidgets.QMainWindow):
    def __init__(self, parent=None):
        super().__init__(parent)

        self.quit = QtWidgets.QAction(self.tr("&Quit"))
        self.quit.setShortcut('Ctrl+Q')
        self.quit.triggered.connect(self.close)

        self.status = QtWidgets.QLabel()

        tool_bar = self.addToolBar(self.tr("Tools"))
        tool_bar.addAction(self.quit)
        tool_bar.addWidget(self.status)

        self.editor = MyEditor()
        self.editor.selectionChanged.connect(self.__new_selection)

        self.setCentralWidget(self.editor)

        self.__new_selection()

    def __new_selection(self):
        self.status.setText(f"{self.editor.glyph_index} / {self.editor.num_glyphs}")

    def closeEvent(self, event: QtGui.QCloseEvent) -> None:
        self.editor.save()
        super().closeEvent(event)


def main():
    app = QtWidgets.QApplication(sys.argv)

    wnd = MainWindow()
    wnd.show()
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
