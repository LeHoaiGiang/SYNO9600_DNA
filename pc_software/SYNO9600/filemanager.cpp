#include "filemanager.h"
//#define DEBUG_FILEMANAGER
#include <QCoreApplication> // Cần thiết nếu initializeApplicationStorage là thành viên
#include <QJsonDocument>    // Cần thiết nếu initializeApplicationStorage là thành viên
#include <QJsonObject>      // Cần thiết nếu initializeApplicationStorage là thành viên

Filemanager::Filemanager()
{
    applicationDirpath = QCoreApplication::applicationDirPath();
    dataFolderDir = applicationDirpath + "/data"; // Dir chi thu muc
    systemfilePath = applicationDirpath + "/system/system.json"; // path chi duong dan cu the nao do
    valveSetting_Path = applicationDirpath + "/system/valve_setting.json"; // duong dan file setting valve

    amidite_sequence_Dir = dataFolderDir + "/00_sequence"; // Dir chi thu muc
    protocol_Dir = dataFolderDir + "/01_protocol"; // Dir chi thu muc
    history_Dir = dataFolderDir+ "/02_history_run";
//    QString amidite_sequence_Dir;
//    QString protocol_Dir;
//    QString history_Dir;
#ifdef DEBUG_FILEMANAGER
    qDebug() << "Filemanager init dataFolderDir: " << dataFolderDir;
    qDebug() << "Filemanager init systemfilePath: " << systemfilePath;
    qDebug() << "Filemanager init valveSetting_Path: " << valveSetting_Path;
    qDebug() << "Filemanager init amidite_sequence_Dir: " << amidite_sequence_Dir;
    qDebug() << "Filemanager init protocol_Dir: " << protocol_Dir;
    qDebug() << "Filemanager init history_Dir: " << history_Dir;

#endif
}


// Hàm trợ giúp để đảm bảo một thư mục tồn tại
// Trả về true nếu thư mục tồn tại hoặc được tạo thành công, ngược lại trả về false
bool Filemanager::ensureDirectoryExists(const QString& dirPath) {
    QDir dir(dirPath);
    if (dir.exists()) {
        return true; // Thư mục đã tồn tại
    }

    // Thử tạo thư mục (mkpath tạo cả các thư mục cha nếu cần)
    if (dir.mkpath(".")) { // "." chỉ định tạo đường dẫn trong đối tượng QDir
        qInfo() << "Đã tạo thư mục:" << dirPath;
        return true;
    } else {
        qWarning() << "Không thể tạo thư mục:" << dirPath;
        return false;
    }
}

// Hàm trợ giúp để đảm bảo một tệp tồn tại (và thư mục cha của nó)
// Có thể tùy chọn ghi nội dung mặc định vào tệp nếu nó được tạo mới
// Trả về true nếu tệp tồn tại hoặc được tạo thành công, ngược lại trả về false
// ĐỊNH NGHĨA - KHÔNG có giá trị mặc định ở đây
bool Filemanager::ensureFileExists(const QString& filePath, const QByteArray& defaultContent) {
    QFile file(filePath);
    if (file.exists()) {
        return true; // Tệp đã tồn tại
    }

    // 1. Đảm bảo thư mục cha tồn tại
    QFileInfo fileInfo(filePath);
    QString parentDirPath = fileInfo.absolutePath();
    // Giả sử ensureDirectoryExists cũng là thành viên hoặc là hàm toàn cục/static
    if (!ensureDirectoryExists(parentDirPath)) {
        qWarning() << "Không thể tạo tệp vì không thể tạo thư mục cha:" << parentDirPath;
        return false;
    }

    // 2. Thử tạo tệp bằng cách mở nó để ghi
    if (file.open(QIODevice::WriteOnly)) {
        qInfo() << "Đã tạo tệp:" << filePath;
        if (!defaultContent.isEmpty()) {
            file.write(defaultContent);
        }
        file.close(); // Đóng tệp sau khi tạo/ghi
        return true;
    } else {
        qWarning() << "Không thể tạo hoặc mở tệp để ghi:" << filePath << "Lỗi:" << file.errorString();
        return false;
    }
}

// Hàm chính để khởi tạo cấu trúc thư mục và tệp
void Filemanager:: initializeApplicationStorage() {
    QString applicationDirPath = QCoreApplication::applicationDirPath();

    // Xác định các đường dẫn
    QString dataFolderDir = applicationDirPath + "/data";
    QString systemDir = applicationDirPath + "/system"; // Thư mục chứa các file hệ thống
    QString systemfilePath = systemDir + "/system.json";
    QString valveSetting_Path = systemDir + "/valve_setting.json";
    QString amidite_sequence_Dir = dataFolderDir + "/00_sequence";
    QString protocol_Dir = dataFolderDir + "/01_protocol";
    QString history_Dir = dataFolderDir + "/02_history_run";

    qInfo() << "Kiểm tra và khởi tạo cấu trúc lưu trữ ứng dụng...";
    qInfo() << "Thư mục ứng dụng:" << applicationDirPath;

    // --- Đảm bảo các thư mục tồn tại ---
    // Lưu ý: ensureDirectoryExists sẽ tự động tạo thư mục cha nếu cần
    // Ví dụ: gọi cho amidite_sequence_Dir cũng sẽ tạo dataFolderDir nếu chưa có
    ensureDirectoryExists(amidite_sequence_Dir);
    ensureDirectoryExists(protocol_Dir);
    ensureDirectoryExists(history_Dir);
    // Thư mục system cũng cần được tạo rõ ràng nếu các file json cần nó
    ensureDirectoryExists(systemDir); // Đảm bảo thư mục system tồn tại trước khi tạo file bên trong

    // --- Đảm bảo các tệp tồn tại ---
    // Tạo nội dung JSON mặc định (ví dụ: đối tượng trống)
    QJsonObject emptyJsonObject;
    QJsonDocument defaultJsonDoc(emptyJsonObject);
    QByteArray defaultJsonData = defaultJsonDoc.toJson();

    ensureFileExists(systemfilePath, defaultJsonData);
    ensureFileExists(valveSetting_Path, defaultJsonData); // Bạn có thể thay đổi nội dung mặc định cho file này
    ensureFileExists(systemfilePath, defaultJsonData);
    ensureFileExists(valveSetting_Path, defaultJsonData); // Bạn có thể thay đổi nội dung mặc định cho file này
    qInfo() << "Quá trình khởi tạo cấu trúc lưu trữ hoàn tất.";
}
bool Filemanager::save()
{
    // Tạo đường dẫn tới thư mục "data"

    // Trong hàm save() nếu bạn thực sự cần kiểm tra thư mục cha
    QFileInfo fileInfo(systemfilePath);
    QDir parentDir(fileInfo.absolutePath());
    if (!parentDir.exists()) {
        // Có thể tạo hoặc báo lỗi, nhưng initializeApplicationStorage nên đảm bảo nó tồn tại rồi
        // parentDir.mkpath("."); // Không nên gọi ở đây nếu initializeApplicationStorage đã chạy
         qWarning() << "Thư mục cha không tồn tại để lưu file:" << parentDir.path();
         return false; // Hoặc xử lý khác
    }
    // Tạo đường dẫn tới file "system.json"
    //QString filePath = dataFolderDir + "/system.json";

    // Tạo một đối tượng QJsonObject để lưu trữ dữ liệu
    QJsonObject jsonObject;
    /*
    QString applicationDirpath;
    QString amidite_sequence_Path;
    QString protocol_Path;
    QString valveSetting_Path;
    QString setting_Path;
     */
    //jsonObject["applicationDirpath"] = applicationDirpath;
    jsonObject["amidite_sequence_Path"] = amidite_sequence_Path;
    jsonObject["protocol_Path"] = protocol_Path;
    //jsonObject["valveSetting_Path"] = valveSetting_Path;
    //jsonObject["setting_Path"] = setting_Path;

    // Tạo một đối tượng QJsonDocument từ QJsonObject
    QJsonDocument jsonDoc(jsonObject);

    // Mở file để ghi
    QFile file(systemfilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        // Ghi dữ liệu vào file
        file.write(jsonDoc.toJson());
        file.close();
#ifdef DEBUG_FILEMANAGER
        qDebug() << "saved to file: " << systemfilePath;
#endif
        return true;
    }
    else
    {
        return false;
#ifdef DEBUG_FILEMANAGER
        qDebug() << "Failed to save data to file: " << systemfilePath;
#endif

    }
}

bool Filemanager::load()
{
    // Mở file để đọc
    bool error = false;
    QFile file(systemfilePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        // Đọc nội dung file
        QByteArray fileData = file.readAll();
        file.close();

        // Tạo một đối tượng QJsonDocument từ dữ liệu file
        QJsonDocument jsonDoc = QJsonDocument::fromJson(fileData);
        if (!jsonDoc.isNull() && jsonDoc.isObject())
        {
            // Truy cập vào đối tượng QJsonObject chứa dữ liệu
            QJsonObject jsonObject = jsonDoc.object();
            /*
            QString applicationDirpath;
            QString amidite_sequence_Path;
            QString protocol_Path;
            QString valveSetting_Path;
            QString setting_Path;
             */
            //applicationDirpath = jsonObject["applicationDirpath"].toString();
            amidite_sequence_Path = jsonObject["amidite_sequence_Path"].toString();
            protocol_Path = jsonObject["protocol_Path"].toString();
            //valveSetting_Path = jsonObject["valveSetting_Path"].toString();
            //setting_Path = jsonObject["setting_Path"].toString();
#ifdef DEBUG_FILEMANAGER

            qDebug() << "loaded from file systemfilePath : " << systemfilePath;
#endif
            error = true;
        }
        else
        {
#ifdef DEBUG_FILEMANAGER
            qDebug() << "Failed to parse JSON data from file: " << systemfilePath;
            error = false;
#endif
        }
    }
    else
    {
#ifdef DEBUG_FILEMANAGER
        qDebug() << "Failed to load file systemfilePath: " << systemfilePath;
#endif
        error = false;
    }
    return error;
}
// kiem tra xem file da ton tai chua
bool Filemanager::fileExists(const QString& filePath) {
    QFileInfo checkFile(filePath);
    return checkFile.exists() && checkFile.isFile();
}
// tu tao file neu file da ton tai roi
void Filemanager::createFileIfNotExists() {
    if (!fileExists(systemfilePath)) {
        QFile file(systemfilePath);
        if (file.open(QIODevice::WriteOnly)) {
            qDebug() << "File created: " << systemfilePath;
            file.close();
        } else {
            qDebug() << "Failed to create file: " << systemfilePath;
        }
    } else {
        qDebug() << "File already exists: " << systemfilePath;
    }
}
