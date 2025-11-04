#ifndef AMIDITE_H
#define AMIDITE_H
#include <QDebug>
#include <QTime>
#include <QString>
#include "QScreen"
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDataStream>
#include <QDir>
#include <QFileDialog>
#include <QStandardItemModel>
#include <QAbstractTableModel>
#include "struct.h"
#include "qlineedit.h"

class amidite
{
public:
    amidite();
    const QStringList bottleNamesFirstList = {"A", "T", "G", "C", "I", "U"}; // 6 AMIDITE không thay đổi
    const QStringList bottleNamesLastList = {"Activator", "TCA", "WASH", "OXidation", "CAPB", "CAPA"};// hóa chất chung cũng không thay đổi
    const QStringList bottleNamesFull = {"A", "T", "G", "C", "I", "U"}; // hiển thị toàn bộ các chai có trong máy để calibration
};

#endif // AMIDITE_H
