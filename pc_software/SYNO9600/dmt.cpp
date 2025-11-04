#include "dmt.h"

dmt::dmt()
{

}
void dmt::setPath(QString Path)
{
    dmtSettingPath = Path;
}
void dmt::save_DMT(const global_var_t& global_var)
{
    QJsonObject json;
    json["strspecialBase"] = global_var.DMT_step.strspecialBase;
    json["bSingnal_DMTOff"] = global_var.DMT_step.bSingnal_DMTOff;

    QJsonObject fillChemical;
    fillChemical["u8_first_type_chemical"] = global_var.DMT_step.fill_chemical.u8_first_type_chemical;

    QJsonArray typeChemicalArray;
    for (int i = 0; i < 3; ++i) {
        typeChemicalArray.append(global_var.DMT_step.fill_chemical.mix_funtion.u8_type_chemical[i]);
    }
    fillChemical["u8_type_chemical"] = typeChemicalArray;

    QJsonArray volumeArray;
    for (int i = 0; i < 3; ++i) {
        volumeArray.append(global_var.DMT_step.fill_chemical.mix_funtion.u16tb_Volume[i].Data);
    }
    fillChemical["u16tb_Volume"] = volumeArray;

    fillChemical["u16tb_wait_after_fill"] = global_var.DMT_step.fill_chemical.u16tb_wait_after_fill.Data;
    json["fill_chemical"] = fillChemical;

    QJsonObject controlPressure;
    QJsonArray optionPressureArray, procsTimeArray, waittingAfterTimeArray;
    for (int i = 0; i < 10; ++i) {
        optionPressureArray.append(global_var.DMT_step.control_pressure.u8_option_pressure[i]);
        procsTimeArray.append(global_var.DMT_step.control_pressure.u16tb_procs_time[i].Data);
        waittingAfterTimeArray.append(global_var.DMT_step.control_pressure.u16tb_waitting_after_time[i].Data);
    }
    controlPressure["u8_option_pressure"] = optionPressureArray;
    controlPressure["u16tb_procs_time"] = procsTimeArray;
    controlPressure["u16tb_waitting_after_time"] = waittingAfterTimeArray;
    json["control_pressure"] = controlPressure;

    QFile file(dmtSettingPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(json).toJson());
        file.close();
    }
}

void dmt::read_DMT(global_var_t& global_var)
{
    QFile file(dmtSettingPath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject json = doc.object();

        global_var.DMT_step.bSingnal_DMTOff = json["bSingnal_DMTOff"].toBool();
        global_var.DMT_step.strspecialBase = json["strspecialBase"].toString();
        QJsonObject fillChemical = json["fill_chemical"].toObject();
        global_var.DMT_step.fill_chemical.u8_first_type_chemical = fillChemical["u8_first_type_chemical"].toInt();

        QJsonArray typeChemicalArray = fillChemical["u8_type_chemical"].toArray();
        for (int i = 0; i < 3; ++i) {
            global_var.DMT_step.fill_chemical.mix_funtion.u8_type_chemical[i] = typeChemicalArray[i].toInt();
        }

        QJsonArray volumeArray = fillChemical["u16tb_Volume"].toArray();
        for (int i = 0; i < 3; ++i) {
            global_var.DMT_step.fill_chemical.mix_funtion.u16tb_Volume[i].Data = volumeArray[i].toInt();
        }

        global_var.DMT_step.fill_chemical.u16tb_wait_after_fill.Data = fillChemical["u16tb_wait_after_fill"].toInt();

        QJsonObject controlPressure = json["control_pressure"].toObject();
        QJsonArray optionPressureArray = controlPressure["u8_option_pressure"].toArray();
        QJsonArray procsTimeArray = controlPressure["u16tb_procs_time"].toArray();
        QJsonArray waittingAfterTimeArray = controlPressure["u16tb_waitting_after_time"].toArray();
        for (int i = 0; i < 10; ++i) {
            global_var.DMT_step.control_pressure.u8_option_pressure[i] = optionPressureArray[i].toInt();
            global_var.DMT_step.control_pressure.u16tb_procs_time[i].Data = procsTimeArray[i].toInt();
            global_var.DMT_step.control_pressure.u16tb_waitting_after_time[i].Data = waittingAfterTimeArray[i].toInt();
        }
    }
}
