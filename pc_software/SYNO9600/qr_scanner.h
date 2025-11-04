#ifndef QR_SCANNER_H
#define QR_SCANNER_H
#include <QFileInfo>
#include <QString>
#include <QDialog>
#include <QDebug>
#include <QTime>
#include <QString>
namespace Ui {
class QR_Scanner;
}

class QR_Scanner : public QDialog
{
    Q_OBJECT

public:

    explicit QR_Scanner(QWidget *parent = nullptr);
    ~QR_Scanner();


private:
    QString dirpath;
    Ui::QR_Scanner *ui;
public:
    QString getDirPath();
private slots:
    void on_btn_ok_released();
    void on_btn_cancel_released();
    bool doesFileExist(const QString &path);
   // void  keyPressEvent(QKeyEvent *event);
    void keyPressEvent(QKeyEvent *event) override; // Ghi đè sự kiện phím
};

#endif // QR_SCANNER_H
