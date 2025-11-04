#include "CircleWidget.h"
#include <QPainter>
#include <QResizeEvent>
#include <QDebug>
/*
CircleWidget::CircleWidget(QWidget *parent) : QWidget(parent)
{
    // Khởi tạo trạng thái các chấm tròn là Empty
    dotStatuses.resize(rows);
    for (int i = 0; i < rows; ++i) {
        dotStatuses[i].resize(cols);  // Chỉ định kích thước cho mỗi hàng
        dotStatuses[i].fill(Empty);   // Điền giá trị mặc định là Empty cho từng phần tử
    }
}

// Hàm để cập nhật trạng thái Dot
void CircleWidget::updateDotStatus(int row, int col, DotStatus status)
{
    if (row >= 0 && row < rows && col >= 0 && col < cols) {
        dotStatuses[row][col] = status;
        update();
    }
}

void CircleWidget::setDotDiameter(int diameter)
{
    dotDiameter = diameter;
    update();
}

void CircleWidget::setDotSpacing(int spacing)
{
    this->spacing = spacing;
    update();
}

// Hàm để điều chỉnh kích thước Dot dựa trên kích thước Widget
void CircleWidget::adjustDotSize()
{
    int totalWidth = width();
    int totalHeight = height();

    // Tính toán lại kích thước Dot để phù hợp với Grid
    dotDiameter = qMin((totalWidth - (cols - 1) * spacing) / cols,
                       (totalHeight - (rows - 1) * spacing) / rows);
}

// Hàm để vẽ các Dot
void CircleWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            int x = j * (dotDiameter + spacing);
            int y = i * (dotDiameter + spacing);

            QColor dotColor = Qt::lightGray; // Mặc định là Empty
            switch (dotStatuses[i][j]) {
                case InProgress: dotColor = Qt::yellow; break;
                case Completed: dotColor = Qt::green; break;
                case Off: dotColor = Qt::red; break;
                case Empty: dotColor = Qt::lightGray; break;
            }
            painter.setBrush(dotColor);
            painter.setPen(Qt::black); // Thêm border color
            painter.drawEllipse(x, y, dotDiameter, dotDiameter);
        }
    }
}

// Hàm resizeEvent để cập nhật kích thước Dot khi Widget thay đổi kích thước
void CircleWidget::resizeEvent(QResizeEvent *event)
{
    adjustDotSize(); // Cập nhật kích thước Dot
    update(); // Vẽ lại
}
*/

#include "CircleWidget.h"
#include <QPainter>
#include <QResizeEvent>
#include <QDebug>

CircleWidget::CircleWidget(QWidget *parent) : QWidget(parent)
{
    // Khởi tạo trạng thái các chấm tròn là Empty
    dotStatuses.resize(rows);
    for (int i = 0; i < rows; ++i) {
        dotStatuses[i].resize(cols);
        dotStatuses[i].fill(Empty);
    }
}

// Hàm để cập nhật trạng thái Dot
void CircleWidget::updateDotStatus(int row, int col, DotStatus status)
{

    //qDebug() << "Updating dot status: col=" << col << ", row=" << row;
    if (row >= 0 && row < rows && col >= 0 && col < cols) {
        dotStatuses[row][col] = status;
        qDebug() << "Updating dot status: col=" << col << ", row=" << row;
        update();

    }
}

void CircleWidget::setDotDiameter(int diameter)
{
    dotDiameter = diameter;
    update();
}

void CircleWidget::setDotSpacing(int spacing)
{
    this->spacing = spacing;
    update();
}

// Hàm để điều chỉnh kích thước Dot dựa trên kích thước Widget
void CircleWidget::adjustDotSize()
{
    int totalWidth = width();
    int totalHeight = height();

    // Tính toán lại kích thước Dot để phù hợp với Grid
    dotDiameter = qMin((totalWidth - (cols - 1) * spacing - 50) / cols,
                       (totalHeight - (rows - 1) * spacing - 50) / rows);
}

// Hàm để vẽ các Dot và các label
void CircleWidget::paintEvent(QPaintEvent *event)
{

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Kích thước cho header
    int headerOffsetX = 30;
    int headerOffsetY = 30;

    // Vẽ các label cho cột
    painter.setPen(Qt::black);
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    for (int j = 0; j < cols; ++j) {
        int x = headerOffsetX + j * (dotDiameter + spacing) + dotDiameter / 2;


        painter.drawText(x, 20, colLabels[j]);
    }

    // Vẽ các label cho hàng
    for (int i = 0; i < rows; ++i) {
        int y = headerOffsetY + i * (dotDiameter + spacing) + dotDiameter / 2;
        painter.drawText(5, y, rowLabels[i]);
    }

    // Vẽ các Dot
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            int x = headerOffsetX + j * (dotDiameter + spacing);
            int y = headerOffsetY + i * (dotDiameter + spacing);

            QColor dotColor = Qt::lightGray; // Mặc định là Empty
            switch (dotStatuses[i][j]) {
                case InProgress: dotColor = Qt::yellow; break;
                case Completed: dotColor = Qt::green; break;
                case Off: dotColor = Qt::red; break;
                case Empty: dotColor = Qt::lightGray; break;
            }

            painter.setBrush(dotColor);
            painter.setPen(Qt::black); // Border color
            painter.drawEllipse(x, y, dotDiameter, dotDiameter);
        }
    }

}

// Hàm resizeEvent để cập nhật kích thước Dot khi Widget thay đổi kích thước
void CircleWidget::resizeEvent(QResizeEvent *event)
{
    adjustDotSize(); // Cập nhật kích thước Dot
    update(); // Vẽ lại
}
