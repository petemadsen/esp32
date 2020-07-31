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

        lines.append("#ifndef PROJECT_GLYPHS_H")
        lines.append("#define PROJECT_GLYPHS_H")
        lines.append("")
        lines.append("const uint8_t glyphs[] = {")

        for i, glyphs in enumerate(self.glyphs):
            line = ", ".join(f"0x{n:02x}" for n in glyphs)
            comment = f"\t//{i}" if (i % 10) == 0 else ""
            lines.append(f"\t{line},{comment}")

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

    def add(self, index):
        self.glyphs.insert(index, [0 for x in range(8)])

    def remove(self, index):
        del self.glyphs[index]

    def size(self):
        return len(self.glyphs)


class MyEditor(QtWidgets.QWidget):
    selectionChanged = QtCore.pyqtSignal()

    def __init__(self, parent=None):
        super().__init__(parent)

        self.file = MyFileParser("../main/glyphs.h")
        self.bit_mask = [0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80]

        self.segments = []
        for s in range(8):
            col = []
            for m in self.bit_mask:
                col.append(0)
            self.segments.append(col)

        self.colors = [QtCore.Qt.gray, QtCore.Qt.black]
        self.current_color = 0
        self.current_pos = (-1, -1)

        self.setMinimumSize(400, 400)
        self.setFocus()

        self.glyph_index = -1
        self.read_glyph(2)

    def read_glyph(self, index):
        if self.glyph_index != -1:
            self.save_glyph()

        glyph = self.file.get(index)
        self.glyph_index = index

        for s in range(8):
            g = glyph[s]
            for x, m in enumerate(self.bit_mask):
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
            for x, m in enumerate(self.bit_mask):
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
        x, y = self.get_xy(event.pos())
        if x is None:
            return

        if event.button() == QtCore.Qt.LeftButton:
            self.current_color = 1
        elif event.button() == QtCore.Qt.RightButton:
            self.current_color = 0
        else:
            return

        self.segments[x][y] = self.current_color

        self.update()

    def mouseMoveEvent(self, event: QtGui.QMouseEvent) -> None:
        x, y = self.get_xy(event.pos())
        if x is None:
            return

        self.segments[x][y] = self.current_color
        self.update()

    def mouseReleaseEvent(self, event: QtGui.QMouseEvent) -> None:
        self.current_pos = (-1, -1)

    def get_xy(self, point):
        x = int(point.x() / self.tile_size)
        y = int(point.y() / self.tile_size)

        if self.current_pos == (x, y):
            return None, None
        if x < 0 or x >= 8:
            return None, None
        if y < 0 or y >= 8:
            return None, None

        self.current_pos = (x, y)
        return x, y

#    def wheelEvent(self, event: QtGui.QWheelEvent) -> None:
#        self.next_glyph(event.angleDelta().y() > 0)

    def keyPressEvent(self, event: QtGui.QKeyEvent) -> None:
        if event.key() == QtCore.Qt.Key_Down:
            self.next_glyph(False)
        elif event.key() == QtCore.Qt.Key_Up:
            self.next_glyph(True)

        super().keyPressEvent(event)

    def next_glyph(self, back):
        index = self.glyph_index
        if back:
            if index == 0:
                return
            index -= 1
        else:
            if index + 1 >= self.file.size():
                return
            index += 1
        self.read_glyph(index)

    def set_glyph(self, pos):
        if pos >= 0 and pos < self.file.size():
            self.read_glyph(pos)

    def add(self):
        self.file.add(self.glyph_index)
        self.selectionChanged.emit()

    def remove(self):
        self.file.remove(self.glyph_index)
        self.selectionChanged.emit()


class MainWindow(QtWidgets.QMainWindow):
    def __init__(self, parent=None):
        super().__init__(parent)

        # GUI
        self.editor = MyEditor()
        self.editor.selectionChanged.connect(self.__new_selection)

        # actions
        self.quit = QtWidgets.QAction(self.tr("&Quit"))
        self.quit.setShortcut('Ctrl+Q')
        self.quit.triggered.connect(self.close)

        self.add = QtWidgets.QAction(self.tr("&Add"))
        self.add.triggered.connect(self.editor.add)

        self.remove = QtWidgets.QAction(self.tr("&Remove"))
        self.remove.triggered.connect(self.editor.remove)

        self.status = QtWidgets.QLabel()

        self.pos = QtWidgets.QSpinBox()
        self.pos.setMinimum(0)
        self.pos.setMaximum(127)
        self.pos.valueChanged.connect(self.editor.set_glyph)

        tool_bar = self.addToolBar(self.tr("Tools"))
        tool_bar.addAction(self.quit)
        tool_bar.addAction(self.add)
        tool_bar.addAction(self.remove)
        tool_bar.addWidget(self.status)
        tool_bar.addWidget(self.pos)

        # layout
        self.setCentralWidget(self.editor)
        self.__new_selection()

    def __new_selection(self):
        self.status.setText(f"{self.editor.glyph_index} / #{self.editor.num_glyphs}")

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
