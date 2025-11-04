#ifndef CIRCLEWIDGET_H
#define CIRCLEWIDGET_H

#include <QWidget>
#include <QVector>
/*
enum DotStatus {
    InProgress,
    Completed,
    Off,
    Empty
};

class CircleWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CircleWidget(QWidget *parent = nullptr);

    void updateDotStatus(int row, int col, DotStatus status);
    void setDotDiameter(int diameter);
    void setDotSpacing(int spacing);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override; // Thêm hàm này

private:
    int rows = 8;
    int cols = 12;
    int dotDiameter = 20; // Kích thước mặc định
    int spacing = 5; // Khoảng cách giữa các chấm

    QVector<QVector<DotStatus>> dotStatuses;
    void adjustDotSize(); // Hàm mới để điều chỉnh kích thước Dot
};
*/

enum DotStatus { Completed, InProgress, Off, Empty };

class CircleWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CircleWidget(QWidget *parent = nullptr);

    void updateDotStatus(int row, int col, DotStatus status);
    void setDotDiameter(int diameter);
    void setDotSpacing(int spacing);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    int rows = 8;
    int cols = 12;
    int dotDiameter = 30;
    int spacing = 5;
    QVector<QVector<DotStatus>> dotStatuses;
    QStringList rowLabels{"A", "B", "C", "D", "E", "F", "G", "H"};
    QStringList colLabels{"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12"};

    void adjustDotSize();
};



#endif // CIRCLEWIDGET_H
