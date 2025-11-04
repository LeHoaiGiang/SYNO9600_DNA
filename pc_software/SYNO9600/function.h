#ifndef FUNCTION_H
#define FUNCTION_H
#include <QDebug>
#include <QTime>
#include <QString>
#include "QScreen"
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QDataStream>
#include <QDir>
#include <QFileDialog>
#include <QStandardItemModel>
#include <QAbstractTableModel>
#include "struct.h"
#include "filemanager.h"
class function : public  Filemanager
{
public:
    function();
    quint8 char2quint8_Amidite(char DataIn);
    //bool save_str_amidite_toJson(global_var_t* global_var, QString Json_Path);
    bool read_str_amidite_fromJson(global_var_t* global_var, QString Json_Path);
    void read_protocol_fromJson(protocol_t* p_protocol, QString Json_Path);
    void save_protocol_toJson(protocol_t* p_protocol, QString Json_Path);

    void getData2AmiditeProcess(global_var_t* global_var);
    //void save_parameter_valve_calib(global_var_t* global_var, QString Json_Path);
    //void read_parameter_valve(global_var_t* global_var, QString Json_Path);
    quint16 valve_calculator_timefill(global_var_t* global_var,uint8_t type_sulphite,uint16_t u16_Volume);
    void saveProtocolJson(protocol_t* p_protocol, QString Json_Path);
    void saveJsonProtocolToFile(const QJsonObject& jsonObject, const QString& filePath);
    QJsonObject createProtocolJson(const protocol_t* p_protocol);
    QJsonObject createSubJson(const protocol_t* p_protocol, uint8_t subIndex);
    QJsonObject createStepJson(const protocol_t* p_protocol, uint8_t subIndex, uint8_t stepIndex);
    void loadProtocol(const QString& filePath, protocol_t* p_protocol);
    QString generateNumberString(int number);
    void ConvertBytesToBoolArray(uint8_t input[12], uint8_t output[96]);
    void ConvertBoolArrayToByteArray(const bool input[96], uint8_t output[12]);
};

#endif // FUNCTION_H
