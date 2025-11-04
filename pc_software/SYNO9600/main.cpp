#include "syno24.h"
#include "delay.h"
#include <QApplication>
#include <QSplashScreen>
#include <QTimer>
#include <QApplication>
#include <QMovie>
#include <QtGui>
#include "QLabel"
#include "unistd.h"
delay delay_;
QString windows_name = "SYNO96 DNA MAIN RELEASE 01-08-2025";
#include <QInputDialog>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SYNO24 w;
    QDateTime now = QDateTime::currentDateTime();
    //QString str_time = now.toString("dd/MM/yyyy"); // Or any other Qt format string
    w.setWindowTitle(windows_name);
    //QPixmap pixmap(":/splash/splash/GIF-SYNO24.gif");
//    QPixmap pixmap(":/splash/splash/Gene-Circle_2.gif");
//    QSplashScreen splash(pixmap);
//    splash.show();
//    delay_.delay_ms(20000);
//    w.show();
//    splash.close();


//    QPixmap pixmap(":/splash/splash/Gene-circle-20240520_1522.gif");
//    QSplashScreen splash(pixmap);
//    QLabel label(&splash);
//    QMovie mv(":/splash/splash/Gene-circle-20240520_1522.gif");
//    label.setMovie(&mv);
//    mv.start();
//    splash.show();
//    delay_.delay_ms(1500);
//    splash.close();
    w.show();

    return a.exec();
}
// lable hien thi Gif
/*
//QLabel label(&splash);
//QMovie mv(":/splash/splash/GIF-SYNO24.gif");
//label.setMovie(&mv);
//mv.start();
*/
