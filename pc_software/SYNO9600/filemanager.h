#ifndef FILEMANAGER_H
#define FILEMANAGER_H
#include "QString"
#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include "QDebug"
class Filemanager
{
public:
    QString applicationDirpath;
    QString amidite_sequence_Path;
    QString protocol_Path;
    QString valveSetting_Path;
    QString setting_Path;

    QString amidite_sequence_Dir;
    QString protocol_Dir;
    QString history_Dir;

    Filemanager();
    bool save();
    bool load();
    bool fileExists(const QString& filePath);
    void createFileIfNotExists();
    bool ensureFileExists(const QString& filePath, const QByteArray& defaultContent = QByteArray());

    bool ensureDirectoryExists(const QString& dirPath); // Khai báo các hàm khác nếu chúng cũng là thành viên
    void initializeApplicationStorage();          // Khai báo các hàm khác nếu chúng cũng là thành viên
private:
    QString systemfilePath;
    QString dataFolderDir;
};

#endif // FILEMANAGER_H
