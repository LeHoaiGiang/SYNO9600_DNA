#include "qr_scanner.h"
#include "ui_qr_scanner.h"
#include <QScreen>
#include <QFileDialog>
#include <QKeyEvent> // Để xử lý sự kiện phím
#include "QMessageBox"
QR_Scanner::QR_Scanner(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QR_Scanner)
{
    ui->setupUi(this);
    move(QGuiApplication::screens().at(0)->geometry().center() - frameGeometry().center()); //can giua man hinh cho mainwindows
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);// xoa title bar
}

QR_Scanner::~QR_Scanner()
{
    delete ui;
}

QString QR_Scanner :: getDirPath()
{
    return dirpath;
}

void QR_Scanner::on_btn_ok_released()
{
//    if(ui->lineEdit_path->text().isEmpty())
//    {
//        //dirpath = "";
//        dirpath.clear();
//        accept();
//    }
//    else
//    {
//        if (doesFileExist(ui->lineEdit_path->text()))
//        {
//            dirpath = ui->lineEdit_path->text();
//        }
//        //dirpath.clear();
//        accept();
//    }
    if (ui->lineEdit_path->text().isEmpty()) {
        dirpath.clear();  // Không có đường dẫn
        accept();         // Đóng cửa sổ
    } else {
        if (doesFileExist(ui->lineEdit_path->text())) {
            dirpath = ui->lineEdit_path->text();  // Lưu đường dẫn hợp lệ
            accept();                             // Đóng cửa sổ
        } else {
            QMessageBox::warning(this, "Error", "The path is invalid!");
             dirpath.clear();  // Không có đường dẫn
            // Không đóng cửa sổ, yêu cầu nhập lại
        }
    }
}


void QR_Scanner::on_btn_cancel_released()
{
    dirpath.clear();
    accept();
}

bool  QR_Scanner:: doesFileExist(const QString &path) {
    QFileInfo checkFile(path);
    // Kiểm tra xem tệp có tồn tại và có thể đọc được không
    return (checkFile.exists() && checkFile.isFile());
}
void QR_Scanner::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        // Bỏ qua phím Enter hoặc Return
        on_btn_ok_released();
        //event->ignore();
    } else {
        // Xử lý phím khác bình thường
        QDialog::keyPressEvent(event);
    }
}
