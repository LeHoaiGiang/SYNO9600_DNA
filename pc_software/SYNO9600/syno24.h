#ifndef SYNO24_H
#define SYNO24_H
#include <vector>
#include <QMainWindow>
#include "QSerialPort"
#include "struct.h"
#include "QStandardItemModel"
#include "QMessageBox"
#include "function.h"
#include "struct.h"
#include "delay.h"
#include "qevent.h"
#include <QJsonObject>
#include <QJsonDocument>
#include "qradiobutton.h"
#include <QtWidgets>
#include "xlsxdocument.h"
#include "xlsxchartsheet.h"
#include "xlsxcellrange.h"
#include "xlsxchart.h"
#include "xlsxrichstring.h"
#include "xlsxworkbook.h"
#include "filemanager.h"
#include "state_machine.h"
#include "trityl.h"
#include "circlewidget.h"
#include "progressbar_timer.h"
#include <QMainWindow>
#include<QtCharts>
#include<QChartView>
#include<QLineSeries>
QT_BEGIN_NAMESPACE
namespace Ui { class SYNO24; }
QT_END_NAMESPACE

class SYNO24 : public QMainWindow
{
    Q_OBJECT

public:
    SYNO24(QWidget *parent = nullptr);
    ~SYNO24();

    state_machine syno24_machine;
    QSerialPort *serialPort;
    QStandardItemModel* model_table_well;
    //path_file_manager_t path_file_manager;
    ProgressBarTimer *m_progressPush;
    ProgressBarTimer *m_progressWait;

    data_ui_t data_ui;
    function fnc;
    delay delay_ui;
    trityl m_trityl;
    trityl m_baseVacuumBox;
    Filemanager filemanager;
    void load_protocol_to_ui(quint8 u8_current_sub, quint8 u8_current_step);
    QCheckBox* checkbx_manual_well[96];
    CircleWidget *circleWidget; // Đối tượng CircleWidget

    QTimer *timermsgTrityl;
    QMessageBox *msgBoxTrityl;  // Con trỏ đến QMessageBox, để có thể đóng nó
    bool tritylSelectionResult; // Lưu kết quả: true (Yes), false (No/Timeout)
    void stopTritylTimer();
private slots:
    void initUIVolumeMNG();
    void serialReceived();
    void fnc_openSerialPort();
    void on_btn_start_update_fw_released();
    void init_syno24();
    void on_btn_Primming_released();
    void on_checkbx_sellect_all_toggled(bool checked);

    void on_btn_calib_step1_released();

    void on_btn_calib_step2_released();
    bool save_str_amidite_toJson(global_var_t* global_var, QString Json_Path);
    void on_btn_save_sequence_released();

    void on_btn_new_protocol_released();

    void on_btn_open_protocol_released();

    void on_btn_clear_data_step_released();

    void on_btn_Calib_released();

    void on_btn_fill_50ul_released();

    void on_btn_scanport_released();

    void scanAndSelectPort(const QString &OldPort);

    void on_btn_savecurrent_step_released();

    void on_btn_save_history_released();

    void on_btn_start_synthetic_released();

    void on_btn_Run2HomeStep_released();

    void on_btn_pause_synthetic_released();

    void on_spbox_number_sub_valueChanged(int arg1);

    void on_pushButton_5_released();

    void on_btn_stop_synthetic_released();

    void on_btn_RunStepper_released();

    void closeEvent (QCloseEvent *event);
    void Setstyle_groupsub(quint8 grbx);
    void checkSelected_STEP();
    void on_select_sub_1_toggled(bool checked);

    void on_select_sub_2_toggled(bool checked);

    void on_select_sub_3_toggled(bool checked);

    void on_select_sub_5_toggled(bool checked);

    void on_select_sub_4_toggled(bool checked);
    void checkSelectChemical();

    void setUI_FirstChemical(quint8 u8_step_cycle);

    quint8 get_FirstChemical(quint8 u8_chemical);
    void on_btn_ManualRun_released();

    void on_btn_backAtuoRun_released();

    void on_checkbx_manual_Allwell_toggled(bool checked);
    void Run_Manual_fill_Chemical();
    void on_btn_StartManual_released();

    void on_btn_StartManual_CtrlVacuum_released();
    void Run_Manual_CtrlVacuum();
    void Display_Protocol_to_user();
    void on_btn_save_protocol_released();
    void send_setting();
    bool ASK_VENDOR_ID();
    void get_sensor_humidity_tempareture();
    void wait_humidity();
    //void on_pushButton_released();
    void log(const QString& message);
    void readExcelSequence(QTableView *tableView, QString path);
    //void on_btn_ConnectPort_released();
    void on_btn_opeExcelSequence_released();
    void on_btn_sequence_released();
    void reload_table_sequence();
    void checkLineEditFormat(const QString &text);
    void onLineEditEditingFinished();
    void on_lineEdit_special_base_textEdited(const QString &arg1);
    void on_lineEdit_special_base_textChanged(const QString &arg1);
    void readSpecialBaseFromLineEdit(QLineEdit *lineEdit, uint16_t special_base[MAX_SEQUENCE_OF_WELL]);
    void printArray(const uint16_t special_base[MAX_SEQUENCE_OF_WELL]);
    bool isbaseSpecial(uint16_t number, const uint16_t special_base[MAX_SEQUENCE_OF_WELL]);
    void copy_sub_protocol_data(sub_protocol_t &dest, const sub_protocol_t &src);
    void calculator_volume_and_process_UI();
    void onButtonReleased_Add_A();
    void onButtonReleased_Add_T();
    void onButtonReleased_Add_G();
    void onButtonReleased_Add_C();
    void onButtonReleased_Add_F1();
    void onButtonReleased_Add_F2();
    void onButtonReleased_Add_ACT();
    void onButtonReleased_Add_TCA();
    void onButtonReleased_Add_WASH();
    void onButtonReleased_Add_OX();
    void onButtonReleased_Add_CAPA();
    void onButtonReleased_Add_CAPB();

    void onButtonReleased_Sub_A();
    void onButtonReleased_Sub_T();
    void onButtonReleased_Sub_G();
    void onButtonReleased_Sub_C();
    void onButtonReleased_Sub_I(); // TẠM THỜI ĐANG DÙNG I - hoặc F1
    void onButtonReleased_Sub_U(); // TẠM THỜI DÙNG U - hoặc F2
    void onButtonReleased_Sub_ACT();
    void onButtonReleased_Sub_TCA();
    void onButtonReleased_Sub_WASH();
    void onButtonReleased_Sub_OX();
    void onButtonReleased_Sub_CAPA();
    void onButtonReleased_Sub_CAPB();
    //void on_btn_calculator_volume_released();

    //void on_btn_ConnectPort_released();

    void on_btn_AutoHome_Manual_released();

    void on_btn_new_sequence_released();

    void on_lineEdit_special_base_trityl_textEdited(const QString &arg1);
    void showSplashScreen();
    void closeSplashScreen();
    //void on_checkbx_manual_clearAllWell_released();

    //void on_checkbx_manual_Allwell_released();

    //void on_pushButton_2_released();

    void on_btn_scanQR_sequence_released();

    void on_btn_savecurrent_dmt_released();

    void on_btn_clear_data_step_dmt_released();
    void LoadDMT();
    //void on_checkbx_manual_clearAllWell_toggled(bool checked);
    void CalEstimateTimeProtocol(quint32 *TimeEstimate);
    QString convertSecondsToHHMMSS(quint32 TimeEstimate);
    void updateMonitor();
    void setCellColor(QTableWidget* tableWidget, int row, int col, const QColor& color) ;
    void MonitorPlateUpdateUI(uint16_t baseFinishe);
    void on_spbox_num_step_sub_1_valueChanged(int arg1);
    void startCountdown(float interval_time);
    void updateCountdown();
    void stopCountdown();
    void writeSettings();
    void readSettings();
    void saveDMTtimeProcess(QSpinBox* DMTtime_process[], int count, const QString& iniFileName);
    void loadDMTtimeProcess(QSpinBox* DMTtime_process[], int count, const QString& iniFileName);


    void logError(const QString& message) {
        qDebug() << "[ERROR]" << message;
    }

    void logDebug(const QString& message) {
        qDebug() << "[DEBUG]" << message;
    }

    void logInfo(const QString& message) {
        qDebug() << "[INFO]" << message;
    }

    void logWarning(const QString& message) {
        qDebug() << "[WARNING]" << message;
    }

    void on_tabWidget_main_currentChanged(int index);

    //void on_select_sub_6_released();

    void on_select_sub_6_toggled(bool checked);

    void on_btnCopySub_released();

    void on_select_sub_7_toggled(bool checked);

    void on_select_sub_9_toggled(bool checked);

    void on_select_sub_10_toggled(bool checked);

    void on_select_sub_8_toggled(bool checked);

    void on_btn_addsub_released();

    void on_btn_delsub_released();

    // xử lý trityl collecttion
    void onTritylTimeout(); // Hàm xử lý khi hết thời gian chờ
    void showTritylMessageBox();
    void on_mgsbox_released();
    void on_btn_tabKillSequenceRun_released();

    void on_btn_AddStateMultiplier_released();

    void on_btn_DeleteMultiplier_released();
    int findMultiplierByPercentage(double percentage);
    void on_btn_InsertStep_released();

    void on_btnDeleteStep_released();
    void copyAndInsertStep(int sourceSubIndex, int sourceStepIndex, int destSubIndex, int insertionIndex);
    void deleteStep(int subIndex, int stepIndex);
    void on_btnDeleteSub_released();
    void ManualControlSystem(); // dùng để thực hiện lệnh đóng mở tay các solenoid các valve và đèn
    void GetFeatureVacuumBox();
    void on_lineEdit_special_base_VacuumBox_textEdited(const QString &arg1);

    void on_btnSaveReagentFillCoupling_released();
    void UpdateUISTTRun( uint8_t state);
    void onCountdownFinished();
    void on_chkbox_Allbase_toggled(bool checked);
    void loadValveDataToTable();
    void on_cbx_valve_selected_currentIndexChanged(int index);
    void saveTableDataToVolume();
    void on_btn_saveCalibTable_released();

    void on_btn_FanVacuumBox_released();

private:

    Ui::SYNO24 *ui;
    TableData m_tableDataMultiplier;
    QStandardItemModel *m_tableModel;
    void updateTableMultiplierView();
};
#endif // SYNO24_H
