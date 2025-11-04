#ifndef DMT_H
#define DMT_H
#include "qglobal.h"
#include "struct.h"
#include "macro.h"
#include "QString"
#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include "QDebug"
#include "QSpinBox"
#include "QPushButton"
#include <QtWidgets>
#include "struct.h"
class dmt
{
public:
    dmt();
    QString dmtSettingPath;
    void setPath(QString Path);
    void save_DMT(const global_var_t& global_var);
    void read_DMT(global_var_t& global_var);
};

#endif // DMT_H
