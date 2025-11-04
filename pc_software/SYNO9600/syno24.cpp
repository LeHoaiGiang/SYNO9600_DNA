#include "syno24.h"
#include "ui_syno24.h"
#include "serial_custom.h"
#include "QMessageBox"
#include "struct.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QCoreApplication>
#include <QFileInfo>
#include <QtCore>
#include <QAxObject>
#include "state_machine.h"
#include <QRegularExpression>
#include <QMessageBox>
#include <QStringList>
#include "volume_manager.h"
#include "qr_scanner.h"
#include "dmt.h"
#include "circlewidget.h"
#include <QTableWidget>
#include <QColor>
#include <QBrush>
#include <QSettings>
#include "Loghistory.h"
#include "killsequence.h"
#include <QSound>
/***
 * 08-2023 Khoi tao du an SYNO9600 dựa trên resource SYNO24 GiangLH - CHUC THANH CONG************************************************
 * Khởi tạo phiên bản beta thử nghiệm Hardware board v1.0.0 - firmware 1.0.0.0 - software 1.0.0.0
 * 10 -2023 các tính năng cơ bản đã hoàn thiện primming - calib - protocol - limit Humidity - log system - Start STOP and Pause system
 * 01 -2024 tính năng tự động primming đối với base đầu tiên và chế động primming do người dùng cài đặt
 * 19-05-2024 Tính năng thu DMT và trityl do người dùng cài đặt
 * 13-06-2024 DMT off hoàn thiện tính năng
 * 01-12-2024  Mixed base tiêu chuẩn quốc tế IUPAC Nucleotide Code = done
 * 30-12-2024 tính năng Miscellaneous
 * 20-03-2025 Thêm tính năng cài đặt volume khi thu DMT Tính năng kill sequence
 * 08-04-2025 Cập nhật Mixed base
 * 20-04-2025 tính năng monitor Plate realtime
 * 26-04-2025 tính năng monitor status machine Run realtime, hiển thị trạng thái rõ ràng từng giây
 * 05-05-2025 phát triển tính năng fill cột rỗng khi kết thúc sequence ******** chưa hoàn thiện ưu tiên task quan trọng
 * 12-07-2025 cập nhật giao diện và backend do thay đổi cấu trúc sắp xếp Valve
 *
 *
*/
#define interval_timer_check_sensor     7000
#define MAIN_WINDOW_DEBUG
#define DEBUG_SOFTWARE
#define SCALE_LEVEL_WAITTIME 100
//#define MAIN_WINDOW_DEBUG_SYNC_OLIGO
QString widget_stylesheet_enable = "QWidget{ border: 3px solid; 	border-color: #00FF00;}";
QString widget_stylesheet_disable = "QWidget { background-color: #FFFFFF;}";
QStringList STEP_NAME = { "DEBLOCK", "WASHING", "COUPLING", "OXIDATION", "CAPPING", "MIXED"};
QStringList NAME_OPTION_PRESSURE = {"LP", "HP", "HV", "LV"};
QStringList NAME_MIX_FUNCTION = {"A", "T", "G", "C", "I","U", "ACTIVATOR", "TCA", "WASH", "OX", "CAP-B", "CAP-A", "AMIDITE"};
QString name_well_asign ="ABCDEFGH";
serial_custom STM32_COM;
QByteArray data_uart_received(LENGTH_COMMAND_RECEIVED, 0);
global_var_t global_var;
Calibration_t cablib_valve;
protocol_t protocol_oligo;
QTimer timer_update_humidity_tempareture;
Filemanager filemanager;
dmt m_DMT;
volume_manager volume;
using namespace QXlsx;
QSplashScreen* splash = nullptr;
// khai báo timer phục vụ tính toán thời gian chạy 28-11-2024
QTimer *timerEstimate;
int secondsRemaining;
QSpinBox* DMTtime_process[MAX_STEP_OF_SUB];
QSpinBox *spinbxNumberBaseOnSub;
QSpinBox *spinbxNumberStepOnBase;
QComboBox *cbx_option_coupling_sub;
QRadioButton *radioSubsellect[MAX_SUB_OF_PROTOCOL];

class ChemicalWells {
public:
    // 1. Tính tổng giếng chứa hóa chất ban đầu
    static int calculateTotalChemicalWells(const uint8_t u8_sequence[96]) {
        int totalWells = 0;
        for (int i = 0; i < 96; ++i) {
            if (u8_sequence[i] != 127) {
                totalWells++;
            }
        }
        return totalWells;
    }

    // 2. Hàm kiểm tra số giếng còn lại và tính tỉ lệ %
    static QPair<int, double> checkRemainingChemicalWells(const uint8_t current_sequence[96], int initialTotalWells) {
        int remainingWells = 0;
        int emptyWellsAtIndex = 0; // Đếm số phần tử có giá trị 127

        for (int i = 0; i < 96; ++i) {
            if (current_sequence[i] != 127) {
                remainingWells++;
            } else {
                emptyWellsAtIndex++;
            }
        }

        double percentage = 0.0;
        if (initialTotalWells > 0) {
            percentage = static_cast<double>(remainingWells) / initialTotalWells * 100.0;
        }

        return qMakePair(remainingWells, percentage);
    }
};

SYNO24::SYNO24(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::SYNO24)
{
    ui->setupUi(this);
    double remainingPercentage;
    /*
    uint8_t initial_sequence[96];
    for (int i = 0; i < 50; ++i) {
        initial_sequence[i] = 100; // Giả sử giá trị khác 127 là có hóa chất
    }
    for (int i = 50; i < 96; ++i) {
        initial_sequence[i] = 127;
    }

    int totalInitialWells = ChemicalWells::calculateTotalChemicalWells(initial_sequence);
    qDebug() << "Total well after Run (100%):" << totalInitialWells;

    uint8_t current_sequence[96];
    for (int i = 0; i < 30; ++i) {
        current_sequence[i] = 50; // Vẫn có hóa chất
    }
    for (int i = 30; i < 96; ++i) {
        current_sequence[i] = 127; // Trở thành giếng rỗng
    }
    QPair<int, double> result = ChemicalWells::checkRemainingChemicalWells(current_sequence, totalInitialWells);
    remainingPercentage = result.second;

    qDebug() << "Số giếng còn lại chứa hóa chất:" << result.first;
    qDebug() << "Tỉ lệ phần trăm giếng còn lại:" << QString::number(result.second, 'f', 2) + "%";

    */
    //showSplashScreen();
    serialPort = new QSerialPort(this);
    //    const auto infos = QSerialPortInfo::availablePorts();
    //    for (const QSerialPortInfo &info : infos)
    //    {
    //        ui->cbx_comport->addItem(info.portName());
    //    }
    //scanAndSelectPort(STM32_COM.currentPort);
    // setWindowIcon(QIcon(":/image/icon/SYNO24_256PPI.png"));
    spinbxNumberBaseOnSub = ui->spbox_numbase_sub_1;
    spinbxNumberStepOnBase = ui->spbox_num_step_sub_1;
    cbx_option_coupling_sub = ui->cbx_option_coupling_sub_1;
    radioSubsellect[0] = ui->select_sub_1;
    radioSubsellect[1] = ui->select_sub_2;
    radioSubsellect[2] = ui->select_sub_3;
    radioSubsellect[3] = ui->select_sub_4;
    radioSubsellect[4] = ui->select_sub_5;
    radioSubsellect[5] = ui->select_sub_6;
    radioSubsellect[6] = ui->select_sub_7;
    radioSubsellect[7] = ui->select_sub_8;
    radioSubsellect[8] = ui->select_sub_9;
    radioSubsellect[9] = ui->select_sub_10;
    STM32_COM.flag_connecttion = false;
    connect(ui->btn_ConnectPort, SIGNAL(released()), this, SLOT(fnc_openSerialPort())); // OPEN SERIAL
    connect(serialPort, SIGNAL(readyRead()), this, SLOT(serialReceived())); // ham xu ly doc du lieu UART
    QObject::connect(&timer_update_humidity_tempareture, SIGNAL(timeout()), this, SLOT(get_sensor_humidity_tempareture()));
    // Kết nối sự kiện editingFinished với hàm onLineEditEditingFinished
    //connect(ui->lineEdit_special_base, &QLineEdit::editingFinished, this, SLOT(onLineEditEditingFinished));
    timer_update_humidity_tempareture.start(interval_timer_check_sensor);
    // XỬ LÝ LỌC KÍ TỰ KHÔNG PHẢI SỐ VÀ DẤU PHẨY CỦA BASE ĐẶC BIỆT
    // Kiểm tra xem có ký tự nào không hợp lệ hay không
    // Khởi tạo validator với biểu thức lọc số và dấu phẩy
    // Khởi tạo validator với biểu thức lọc số và dấu phẩy
    QValidator *validator_trityl = new QRegExpValidator(QRegExp("[0-9,]+"), this);
    ui->lineEdit_special_base_trityl->setValidator(validator_trityl);
    ui->lineEdit_special_base_trityl->setPlaceholderText("Example: 1,2,8,9");
    // khởi tạo validator với biểu thức lọc số và dấu phẩy // quyết định base sẽ hút vacuum của box
    ui->lineEdit_special_base_VacuumBox->setValidator(validator_trityl);
    ui->lineEdit_special_base_VacuumBox->setPlaceholderText("Example: 1,2,8,9");
    // state machine process ==============================================================================================
    syno24_machine.setBtnStartAuto(ui->btn_start_synthetic);
    syno24_machine.setBtnStopAuto(ui->btn_stop_synthetic);
    syno24_machine.setBtnPauseAuto(ui->btn_pause_synthetic);
    ui->btn_pause_synthetic->setCheckable(true); // Bật chế độ toggle

    syno24_machine.setBtnStartFillChemicalManual(ui->btn_StartManual);
    syno24_machine.setBtnStartPushDownManual(ui->btn_StartManual_CtrlVacuum);
    syno24_machine.setBtnHomeManual(ui->btn_AutoHome_Manual);

    syno24_machine.state_machine_init();
    delay_ui.delay_ms(1000);
    ui->lineEdit_special_base_trityl->setCursorPosition(0);
    ui->lineEdit_special_base_VacuumBox->setCursorPosition(0);
    // m_trityl.cleardata();
    printArray(m_trityl.speacial_base);
    init_syno24();
    initUIVolumeMNG();
    // ================================== UPDATE MONITOR PLATE LÊN MÀN HÌNH
    //=============================================================================================
    MonitorPlateUpdateUI(0);
    // init timer tính toán thời gian chạy
    timerEstimate = new QTimer(this);
    float secondsRemainingEstimate;
    bool isPausedEstimate;
    connect(timerEstimate, SIGNAL(timeout()), this, SLOT(updateCountdown()));

    ui->tabWidget_main->setCurrentIndex(0);
    //================================= TRITYL COLLECTION==========================================
    //=============================================================================================
    // Khởi tạo QTimer *trong* phần thân của constructor:
    timermsgTrityl = new QTimer(this);

    //Kết nối signal timeout() của timer với slot onTritylTimeout()
    connect(timermsgTrityl, &QTimer::timeout, this, &SYNO24::onTritylTimeout);
    //====================================== MULTIPLIER============================================

    // Khởi tạo model cho QTableView
    m_tableModel = new QStandardItemModel(0, 3, this); // 0 rows, 3 columns
    m_tableModel->setHorizontalHeaderLabels({"From", "To", "Multiplier"});
    ui->tableView_2->setModel(m_tableModel);
    //================================== LOAD FILE INIT============================================
    readSettings();

    ui->btn_HighPushSV->setCheckable(true); // Đảm bảo nút có thể bật/tắt
    ui->btnLowPushSV->setCheckable(true); // Đảm bảo nút có thể bật/tắt
    ui->btnMediumPushSV->setCheckable(true); // Đảm bảo nút có thể bật/tắt
    ui->btnFAN->setCheckable(true); // Đảm bảo nút có thể bật/tắt
    ui->btnOpenAirNitor->setCheckable(true); // Đảm bảo nút có thể bật/tắt
    ui->btn_FanVacuumBox->setCheckable(true);
    connect(ui->btn_HighPushSV, &QPushButton::toggled, this, &SYNO24::ManualControlSystem);
    connect(ui->btn_FanVacuumBox,&QPushButton::toggled, this, &SYNO24::ManualControlSystem );
    connect(ui->btnLowPushSV, &QPushButton::toggled, this, &SYNO24::ManualControlSystem);
    connect(ui->btnMediumPushSV, &QPushButton::toggled, this, &SYNO24::ManualControlSystem);
    connect(ui->btnFAN, &QPushButton::toggled, this, &SYNO24::ManualControlSystem);
    connect(ui->btnOpenAirNitor, &QPushButton::toggled, this, &SYNO24::ManualControlSystem);

}

SYNO24::~SYNO24()
{
    delete ui;
}
void SYNO24:: initUIVolumeMNG()
{

    volume.tableView = ui->tableView_calculator_volume;
    volume.tableView_init();
    volume.spbxs_current[A] = ui->spbx_current_vl_A;
    volume.spbxs_current[T] = ui->spbx_current_vl_T;
    volume.spbxs_current[G] = ui->spbx_current_vl_G;
    volume.spbxs_current[C] = ui->spbx_current_vl_C;
    volume.spbxs_current[I] = ui->spbx_current_vl_F1;
    volume.spbxs_current[U] = ui->spbx_current_vl_F2;
    volume.spbxs_current[Activator] = ui->spbx_current_vl_ACT;
    volume.spbxs_current[TCA_in_DCM] = ui->spbx_current_vl_TCA;
    volume.spbxs_current[WASH_ACN_DCM] = ui->spbx_current_vl_WASH;
    volume.spbxs_current[CAPPING_CAPA] = ui->spbx_current_vl_CAPA;
    volume.spbxs_current[CAPPING_CAPB] = ui->spbx_current_vl_CAPB;
    volume.spbxs_current[OXIDATION_IODINE] = ui->spbx_current_vl_OX;
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    volume.spbxs_change[A] = ui->spbx_change_vl_A;
    volume.spbxs_change[T] = ui->spbx_change_vl_T;
    volume.spbxs_change[G] = ui->spbx_change_vl_G;
    volume.spbxs_change[C] = ui->spbx_change_vl_C;
    volume.spbxs_change[I] = ui->spbx_change_vl_F1;
    volume.spbxs_change[U] = ui->spbx_change_vl_F2;
    volume.spbxs_change[Activator] = ui->spbx_change_vl_ACT;
    volume.spbxs_change[TCA_in_DCM] = ui->spbx_change_vl_TCA;
    volume.spbxs_change[WASH_ACN_DCM] = ui->spbx_change_vl_WASH;
    volume.spbxs_change[CAPPING_CAPA] = ui->spbx_change_vl_CAPA;
    volume.spbxs_change[CAPPING_CAPB] = ui->spbx_change_VL_CAPB;
    volume.spbxs_change[OXIDATION_IODINE] = ui->spbx_change_vl_OX;
    calculator_volume_and_process_UI();
    connect(ui->btn_add_A,SIGNAL(released()),this,SLOT(onButtonReleased_Add_A())); //
    connect(ui->btn_add_T,SIGNAL(released()),this,SLOT(onButtonReleased_Add_T())); //
    connect(ui->btn_add_G,SIGNAL(released()),this,SLOT(onButtonReleased_Add_G())); //
    connect(ui->btn_add_C,SIGNAL(released()),this,SLOT(onButtonReleased_Add_C())); //
    connect(ui->btn_add_F1,SIGNAL(released()),this,SLOT(onButtonReleased_Add_F1())); //
    connect(ui->btn_add_F2,SIGNAL(released()),this,SLOT(onButtonReleased_Add_F2())); //
    connect(ui->btn_add_ACT,SIGNAL(released()),this,SLOT(onButtonReleased_Add_ACT())); //
    connect(ui->btn_add_TCA,SIGNAL(released()),this,SLOT(onButtonReleased_Add_TCA())); //
    connect(ui->btn_add_Wash,SIGNAL(released()),this,SLOT(onButtonReleased_Add_WASH())); //
    connect(ui->btn_add_Ox,SIGNAL(released()),this,SLOT(onButtonReleased_Add_OX())); //
    connect(ui->btn_add_CapA,SIGNAL(released()),this,SLOT(onButtonReleased_Add_CAPA())); //
    connect(ui->btn_add_CapB,SIGNAL(released()),this,SLOT(onButtonReleased_Add_CAPB())); //

    connect(ui->btn_sub_A,SIGNAL(released()),this,SLOT(onButtonReleased_Sub_A())); //
    connect(ui->btn_sub_T,SIGNAL(released()),this,SLOT(onButtonReleased_Sub_T())); //
    connect(ui->btn_sub_G,SIGNAL(released()),this,SLOT(onButtonReleased_Sub_G())); //
    connect(ui->btn_sub_C,SIGNAL(released()),this,SLOT(onButtonReleased_Sub_C())); //
    connect(ui->btn_sub_F1,SIGNAL(released()),this,SLOT(onButtonReleased_Sub_I())); //
    connect(ui->btn_sub_F2,SIGNAL(released()),this,SLOT(onButtonReleased_Sub_U())); //
    connect(ui->btn_sub_ACT,SIGNAL(released()),this,SLOT(onButtonReleased_Sub_ACT())); //
    connect(ui->btn_sub_TCA,SIGNAL(released()),this,SLOT(onButtonReleased_Sub_TCA())); //
    connect(ui->btn_sub_Wash,SIGNAL(released()),this,SLOT(onButtonReleased_Sub_WASH())); //
    connect(ui->btn_sub_Ox,SIGNAL(released()),this,SLOT(onButtonReleased_Sub_OX())); //
    connect(ui->btn_sub_CapA,SIGNAL(released()),this,SLOT(onButtonReleased_Sub_CAPA())); //
    connect(ui->btn_sub_CapB,SIGNAL(released()),this,SLOT(onButtonReleased_Sub_CAPB())); //
    //volume.read_parameter_valve(); // hiển thị số lượng hoá chất
    volume.ReloadUIVolumeMNG();
    // Cập nhật trạng thái cho một số chấm tròn
}


void SYNO24::closeEvent (QCloseEvent *event)
{
    //QSound::play(":/sounds/alert.wav");
    if (QMessageBox::question(this,
                              "Quit Application",
                              "Do you want to quit the application? This will stop all system operations.",
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
        serialPort->close();
        delete serialPort;
        volume.save_parameter_valve();
        filemanager.save();
        writeSettings();
        QCoreApplication::quit();
        QCoreApplication::exit();
    }
    else
    {
        event->ignore();
    }
}


void SYNO24:: init_syno24()
{
    global_var.status_and_sensor.flag_have_feedback_value = false;
    // ĐỊNH NGHĨA - KHÔNG có giá trị mặc định ở đây
    filemanager.initializeApplicationStorage();
    filemanager.load();
    m_DMT.setPath(filemanager.applicationDirpath + "/system/dmt.json");
    m_DMT.read_DMT(global_var);
    ui->lineEdit_special_base_trityl->setText(global_var.DMT_step.strspecialBase);
    m_trityl.cleardata();
    m_baseVacuumBox.cleardata();
    readSpecialBaseFromLineEdit(ui->lineEdit_special_base_trityl, m_trityl.speacial_base);
    fnc.read_protocol_fromJson(&protocol_oligo,filemanager.protocol_Path);
    volume.setPath(filemanager.valveSetting_Path);
    volume.read_parameter_valve();
    ui->lineEdit_path_sequence->setText(filemanager.amidite_sequence_Path);
    // Process TABLE AMIDITE
    ui->tableView->horizontalHeader()->setStretchLastSection(true);
    // Đặt độ rộng tối thiểu cho cột 1 và cột 2
    ui->tableView->horizontalHeader()->setMinimumSectionSize(50);
    ui->tableView->horizontalHeader()->resizeSection(0, 50);
    ui->tableView->horizontalHeader()->setMinimumSectionSize(150);
    ui->tableView->horizontalHeader()->resizeSection(1, 150);
    //ui->tableView->setColumnWidth(1,500);

    model_table_well = new QStandardItemModel(MAX_WELL_AMIDITE, 4, this);
    QStringList verticalHeader;
    ui->tableView->setModel(model_table_well);
    QStringList horizontalHeader = QStringList() << "Position" << "Name"<< "Sequence Length"<<"Sequence Oligo";
    model_table_well->setHorizontalHeaderLabels(horizontalHeader);
    model_table_well->setVerticalHeaderLabels(verticalHeader);
    fnc.read_str_amidite_fromJson(&global_var, filemanager.amidite_sequence_Path); // READING FILE SEQUENCE TO RUN
    fnc.getData2AmiditeProcess(&global_var);
    // Generate data to tableview
    reload_table_sequence();
    // reLoad DMT
    LoadDMT();
    //====================================================================================
    ui->lineEdit_path_protocol->setText(filemanager.protocol_Path);
    data_ui.u8_current_SUB_edit_ui = 0;
    data_ui.u8_current_STEP_edit_ui = 0;
    load_protocol_to_ui(0,0);
    // xu ly chon sub nao
    ui->selected_mix_fnc->hide();
    ui->select_sub_1->setChecked(true);
    Setstyle_groupsub(0);
    Display_Protocol_to_user();

    connect(ui->sub1_step_1, SIGNAL(clicked(bool)), this, SLOT(checkSelected_STEP()));
    connect(ui->sub1_step_2, SIGNAL(clicked(bool)), this, SLOT(checkSelected_STEP()));
    connect(ui->sub1_step_3, SIGNAL(clicked(bool)), this, SLOT(checkSelected_STEP()));
    connect(ui->sub1_step_4, SIGNAL(clicked(bool)), this, SLOT(checkSelected_STEP()));
    connect(ui->sub1_step_5, SIGNAL(clicked(bool)), this, SLOT(checkSelected_STEP()));
    connect(ui->sub1_step_6, SIGNAL(clicked(bool)), this, SLOT(checkSelected_STEP()));
    connect(ui->sub1_step_7, SIGNAL(clicked(bool)), this, SLOT(checkSelected_STEP()));
    connect(ui->sub1_step_8, SIGNAL(clicked(bool)), this, SLOT(checkSelected_STEP()));
    connect(ui->sub1_step_9, SIGNAL(clicked(bool)), this, SLOT(checkSelected_STEP()));
    connect(ui->sub1_step_10, SIGNAL(clicked(bool)), this, SLOT(checkSelected_STEP()));
    connect(ui->sub1_step_11, SIGNAL(clicked(bool)), this, SLOT(checkSelected_STEP()));
    connect(ui->sub1_step_12, SIGNAL(clicked(bool)), this, SLOT(checkSelected_STEP()));

    connect(ui->selected_deblock, SIGNAL(clicked(bool)), this, SLOT(checkSelectChemical()));
    connect(ui->selected_washing, SIGNAL(clicked(bool)), this, SLOT(checkSelectChemical()));
    connect(ui->selected_capping, SIGNAL(clicked(bool)), this, SLOT(checkSelectChemical()));
    connect(ui->selected_oxidation, SIGNAL(clicked(bool)), this, SLOT(checkSelectChemical()));
    connect(ui->selected_coupling, SIGNAL(clicked(bool)), this, SLOT(checkSelectChemical()));
    connect(ui->selected_mix_fnc,SIGNAL(clicked(bool)), this, SLOT(checkSelectChemical()));

    //========================================================================================
    checkbx_manual_well[0] = ui->well_1;
    checkbx_manual_well[1] = ui->well_2;
    checkbx_manual_well[2] = ui->well_3;
    checkbx_manual_well[3] = ui->well_4;
    checkbx_manual_well[4] = ui->well_5;
    checkbx_manual_well[5] = ui->well_6;
    checkbx_manual_well[6] = ui->well_7;
    checkbx_manual_well[7] = ui->well_8;
    checkbx_manual_well[8] = ui->well_9;
    checkbx_manual_well[9] = ui->well_10;
    checkbx_manual_well[10] = ui->well_11;
    checkbx_manual_well[11] = ui->well_12;
    checkbx_manual_well[12] = ui->well_13;
    checkbx_manual_well[13] = ui->well_14;
    checkbx_manual_well[14] = ui->well_15;
    checkbx_manual_well[15] = ui->well_16;
    checkbx_manual_well[16] = ui->well_17;
    checkbx_manual_well[17] = ui->well_18;
    checkbx_manual_well[18] = ui->well_19;
    checkbx_manual_well[19] = ui->well_20;
    checkbx_manual_well[20] = ui->well_21;
    checkbx_manual_well[21] = ui->well_22;
    checkbx_manual_well[22] = ui->well_23;
    checkbx_manual_well[23] = ui->well_24;
    checkbx_manual_well[24] = ui->well_25;
    checkbx_manual_well[25] = ui->well_26;
    checkbx_manual_well[26] = ui->well_27;
    checkbx_manual_well[27] = ui->well_28;
    checkbx_manual_well[28] = ui->well_29;
    checkbx_manual_well[29] = ui->well_30;
    checkbx_manual_well[30] = ui->well_31;
    checkbx_manual_well[31] = ui->well_32;
    checkbx_manual_well[32] = ui->well_33;
    checkbx_manual_well[33] = ui->well_34;
    checkbx_manual_well[34] = ui->well_35;
    checkbx_manual_well[35] = ui->well_36;
    checkbx_manual_well[36] = ui->well_37;
    checkbx_manual_well[37] = ui->well_38;
    checkbx_manual_well[38] = ui->well_39;
    checkbx_manual_well[39] = ui->well_40;
    checkbx_manual_well[40] = ui->well_41;
    checkbx_manual_well[41] = ui->well_42;
    checkbx_manual_well[42] = ui->well_43;
    checkbx_manual_well[43] = ui->well_44;
    checkbx_manual_well[44] = ui->well_45;
    checkbx_manual_well[45] = ui->well_46;
    checkbx_manual_well[46] = ui->well_47;
    checkbx_manual_well[47] = ui->well_48;
    checkbx_manual_well[48] = ui->well_49;
    checkbx_manual_well[49] = ui->well_50;
    checkbx_manual_well[50] = ui->well_51;
    checkbx_manual_well[51] = ui->well_52;
    checkbx_manual_well[52] = ui->well_53;
    checkbx_manual_well[53] = ui->well_54;
    checkbx_manual_well[54] = ui->well_55;
    checkbx_manual_well[55] = ui->well_56;
    checkbx_manual_well[56] = ui->well_57;
    checkbx_manual_well[57] = ui->well_58;
    checkbx_manual_well[58] = ui->well_59;
    checkbx_manual_well[59] = ui->well_60;
    checkbx_manual_well[60] = ui->well_61;
    checkbx_manual_well[61] = ui->well_62;
    checkbx_manual_well[62] = ui->well_63;
    checkbx_manual_well[63] = ui->well_64;
    checkbx_manual_well[64] = ui->well_65;
    checkbx_manual_well[65] = ui->well_66;
    checkbx_manual_well[66] = ui->well_67;
    checkbx_manual_well[67] = ui->well_68;
    checkbx_manual_well[68] = ui->well_69;
    checkbx_manual_well[69] = ui->well_70;
    checkbx_manual_well[70] = ui->well_71;
    checkbx_manual_well[71] = ui->well_72;
    checkbx_manual_well[72] = ui->well_73;
    checkbx_manual_well[73] = ui->well_74;
    checkbx_manual_well[74] = ui->well_75;
    checkbx_manual_well[75] = ui->well_76;
    checkbx_manual_well[76] = ui->well_77;
    checkbx_manual_well[77] = ui->well_78;
    checkbx_manual_well[78] = ui->well_79;
    checkbx_manual_well[79] = ui->well_80;
    checkbx_manual_well[80] = ui->well_81;
    checkbx_manual_well[81] = ui->well_82;
    checkbx_manual_well[82] = ui->well_83;
    checkbx_manual_well[83] = ui->well_84;
    checkbx_manual_well[84] = ui->well_85;
    checkbx_manual_well[85] = ui->well_86;
    checkbx_manual_well[86] = ui->well_87;
    checkbx_manual_well[87] = ui->well_88;
    checkbx_manual_well[88] = ui->well_89;
    checkbx_manual_well[89] = ui->well_90;
    checkbx_manual_well[90] = ui->well_91;
    checkbx_manual_well[91] = ui->well_92;
    checkbx_manual_well[92] = ui->well_93;
    checkbx_manual_well[93] = ui->well_94;
    checkbx_manual_well[94] = ui->well_95;
    checkbx_manual_well[95] = ui->well_96;
    //
    m_progressPush = new ProgressBarTimer(ui->prgBar_Push, this);
    m_progressWait = new ProgressBarTimer(ui->prgBar_Wait, this);
    connect(m_progressWait, &ProgressBarTimer::finished, this, &SYNO24::onCountdownFinished);
    // bảng hiển thị giá trị calib valve
    ui->tableWidget_valveinfo->setAlternatingRowColors(true);
    ui->cbx_valve_selected->setCurrentIndex(0);
    loadValveDataToTable();
}


void SYNO24:: serialReceived()
{
    if (serialPort->isOpen()) // PORT OPEN ???
    {
        data_uart_received = serialPort->readAll();
        STM32_COM.u16_length_command_fw_rx = data_uart_received.size();
        if(STM32_COM.u16_length_command_fw_rx == LENGTH_COMMAND_RECEIVED) // check length data
        {
            STM32_COM.flag_process_command = true;// flag nay dung de check command da dung length hay chua - kiem tra ki tu cuoi cung
#ifdef MAIN_WINDOW_DEBUG
            QByteArray ba_as_hex_string = data_uart_received.toHex();
            //qDebug() << "Headder command Rx Receive : "<<ba_as_hex_string[0]<<ba_as_hex_string[1];
            //qDebug() << "CMD TX" <<STM32_COM.header_commmand;
            //qDebug() << "CMD Rx Receive : "<<ba_as_hex_string[0]<<ba_as_hex_string[1];
#endif
            if(STM32_COM.header_commmand == data_uart_received[0])
            {
                STM32_COM.flag_waitResponse_from_FW = true; // firmware phan hoi
            }
        }
        else
        {
#ifdef MAIN_WINDOW_DEBUG
            qDebug() << "lENGTH DATA ERROR: "<<STM32_COM.u16_length_command_fw_rx;
            ui->textEdit_status_update_fw->append("lENGTH DATA ERROR: " + QString::number(STM32_COM.u16_length_command_fw_rx));
#endif
        }
    }
    else
    {
#ifdef MAIN_WINDOW_DEBUG
        qDebug() << "OPEN ERROR: ";
#endif
    } // END PORT OPEN
    if(STM32_COM.flag_process_command) // CHECK FLAG PROCESS COMMAND
    {
        switch (data_uart_received[0])
        {
        case CMD_ASK_VENDOR_ID:
        {
#ifdef MAIN_WINDOW_DEBUG
            qDebug() << "FW Command CMD_ASK_VENDOR_ID";
#endif
            break;
        }
        case CMD_RECIVED_SETTING:
        {
            break;
        }
        case CMD_PRESURE_TESTING:
        {
            break;
        }
        case CMD_CONNECT_LINK:
        {
#ifdef MAIN_WINDOW_DEBUG
            qDebug() << "FW Command 0x01 CMD_CONNECT_LINK";
#endif
            break;
        }
        case CMD_START_OLIGO_STEP:
        {
#ifdef MAIN_WINDOW_DEBUG
            qDebug() << "FW Command 0x05 CMD_START_OLIGO_STEP";
#endif
            break;
        }
        case 0x03:
        {
#ifdef MAIN_WINDOW_DEBUG
            qDebug() << "FW Command 0x03 ";
#endif

            break;
        }
        case CMD_DATA_OLIGO:
        {
#ifdef MAIN_WINDOW_DEBUG
            qDebug() << "FW Command CMD_DATA_OLIGO 0x06 ";
#endif

            break;
        }
        case CMD_PRIMMING:
        {
#ifdef MAIN_WINDOW_DEBUG
            qDebug() << "FW  Command CMD_PRIMMING ";
#endif
            break;
        }
        case CMD_CALIBRATION_VALVE:
        {
#ifdef MAIN_WINDOW_DEBUG
            qDebug() << "FW  Command 0x64 - CMD_CALIBRATION_VALVE";
#endif
            break;
        }
        case CMD_MANUAL_RUN:
        {
#ifdef MAIN_WINDOW_DEBUG
            qDebug() << "FW  Command 0x66 - CMD_MANUAL_RUN";
#endif
            break;
        }
        case CMD_EXHAUSTED_CHEMICAL:
        {
            qDebug() << "FW  Command 0x66 - CMD_EXHAUSTED_CHEMICAL";
            break;
        }
        case CMD_RUN2HOME:
        {
            qDebug() << "FW  Command - CMD_RUN2HOME";
            break;
        }
        case CMD_TRITYL_START:
        {
            qDebug() << "FW  Command 0x20 - CMD_TRITYL_START";
            break;
        }
        case CMD_TRITYL_STOP:
        {
            qDebug() << "FW  Command 0x21 - CMD_TRITYL_STOP";
            break;
        }
        case CMD_FIRMWARE_UPDATE_STATUS_AND_SENSOR:
        {
            global_var.status_and_sensor.u16_temperature.Byte[0] = data_uart_received[1];
            global_var.status_and_sensor.u16_temperature.Byte[1] = data_uart_received[2];
            global_var.status_and_sensor.u16_humidity.Byte[0] = data_uart_received[3];
            global_var.status_and_sensor.u16_humidity.Byte[1] = data_uart_received[4];
            global_var.status_and_sensor.f_humidity = global_var.status_and_sensor.u16_humidity.Data ;
            global_var.status_and_sensor.f_temperature = global_var.status_and_sensor.u16_temperature.Data;
            global_var.status_and_sensor.str_humidity.setNum(global_var.status_and_sensor.f_humidity / 100);
            global_var.status_and_sensor.str_temperature.setNum(global_var.status_and_sensor.f_temperature / 100);

            global_var.status_and_sensor.Pos_X.Byte[0] = data_uart_received[5];
            global_var.status_and_sensor.Pos_X.Byte[1] = data_uart_received[6];
            global_var.status_and_sensor.Pos_Y.Byte[0] = data_uart_received[7];
            global_var.status_and_sensor.Pos_Y.Byte[1] = data_uart_received[8];
            global_var.status_and_sensor.Pos_Z1.Byte[0] = data_uart_received[9];
            global_var.status_and_sensor.Pos_Z1.Byte[1] = data_uart_received[10];

            global_var.status_and_sensor.fPosX = (float)global_var.status_and_sensor.Pos_X.Data / 10;
            global_var.status_and_sensor.fPosY = (float)global_var.status_and_sensor.Pos_Y.Data / 10;
            global_var.status_and_sensor.fPosZ1 = (float)global_var.status_and_sensor.Pos_Z1.Data / 10;

            ui->spbx_x_stepper_work->setValue(global_var.status_and_sensor.fPosX);
            ui->spbx_y_stepper_work->setValue(global_var.status_and_sensor.fPosY);
            ui->spbx_z1_stepper_work->setValue(global_var.status_and_sensor.fPosZ1);


            ui->lbl_humidity->setText(global_var.status_and_sensor.str_humidity);
            ui->lbl_temperature->setText(global_var.status_and_sensor.str_temperature);
            global_var.status_and_sensor.flag_have_feedback_value = true;
            //qDebug() << " f_humidity" <<global_var.status_and_sensor.f_humidity;
            break;
        }
        case CMD_FEEDBACK_STATUS_RUN:
        {
            global_var.updateSTTRun2UI.u8_function_count = data_uart_received[1];
            global_var.updateSTTRun2UI.u8_subfunction =  data_uart_received[2];
            global_var.updateSTTRun2UI.u16tb_procs_time.Byte[0] = data_uart_received[3];
            global_var.updateSTTRun2UI.u16tb_procs_time.Byte[1] = data_uart_received[4];
            global_var.updateSTTRun2UI.u16tb_waitting_after_time.Byte[0] = data_uart_received[5];
            global_var.updateSTTRun2UI.u16tb_waitting_after_time.Byte[1] = data_uart_received[6];
            qDebug() << "CMD_FEEDBACK_STATUS_RUN" << "functionProgress" << global_var.updateSTTRun2UI.u8_function_count <<
                        "functionProgress sub" <<  global_var.updateSTTRun2UI.u8_subfunction <<
                        global_var.updateSTTRun2UI.u16tb_procs_time.Data<< " : "<< global_var.updateSTTRun2UI.u16tb_waitting_after_time.Data ;
            if( global_var.updateSTTRun2UI.u8_subfunction == WAIT_AFTERFILL)
            {
                UpdateUISTTRun(global_var.updateSTTRun2UI.u8_function_count);
            }
            else
            {
                UpdateUISTTRun(global_var.updateSTTRun2UI.u8_function_count + 1);
                global_var.updateSTTRun2UI.log_control_pressure = NAME_OPTION_PRESSURE[protocol_oligo.sub[global_var.updateSTTRun2UI.currentSub].step[global_var.updateSTTRun2UI.currentStep].control_pressure.u8_option_pressure[global_var.updateSTTRun2UI.u8_function_count]] +  " - "+
                        QString::number(protocol_oligo.sub[global_var.updateSTTRun2UI.currentSub].step[global_var.updateSTTRun2UI.currentStep].control_pressure.u16tb_procs_time[global_var.updateSTTRun2UI.u8_function_count].Data) + " | "
                        + QString::number(protocol_oligo.sub[global_var.updateSTTRun2UI.currentSub].step[global_var.updateSTTRun2UI.currentStep].control_pressure.u16tb_waitting_after_time[global_var.updateSTTRun2UI.u8_function_count].Data);
                // khác so với SYNO24X không nhân cho 100 để tính thời gian
                ui->lbl_stt_fw_fb->setText(global_var.updateSTTRun2UI.log_control_pressure);
            }

            if(global_var.updateSTTRun2UI.u8_subfunction == PUSHDOWN_FNC) // progress
            {
                m_progressPush->startCountdown(global_var.updateSTTRun2UI.u16tb_procs_time.Data);
            }
            else
            {
                if(global_var.updateSTTRun2UI.u8_subfunction == WAIT_FNC) // waitting proress
                {
                    m_progressWait->startCountdown(global_var.updateSTTRun2UI.u16tb_waitting_after_time.Data * SCALE_LEVEL_WAITTIME);
                }
                else // waiting after fill
                {
                    m_progressWait->startCountdown(global_var.updateSTTRun2UI.u16tb_waitting_after_time.Data);
                }
            }
            break;
        }
        case CMD_CONTROL_AIR_START:
        {
            qDebug() << "CMD_CONTROL_AIR_START";
            break;
        }
        case CMD_STOP_SYSTHETIC_OLIGO:
        {
            //CMD_STOP_SYSTHETIC_OLIGO
            qDebug() << "STOP OK";
            break;
        }
        case CMD_SEUQENCE_AND_KILL:
        {
            qDebug() << "CMD_SEUQENCE_AND_KILL OK";
            break;
        }
        case CMD_CONTROL_SYNCHRONIZE_IO:
        {
            qDebug() << "CMD_CONTROL_SYNCHRONIZE_IO OK";
            break;
        }
        default:
        {
            qDebug() << "FW  Command not find on table";
            ui->textEdit_status_update_fw->append("ERROR COMMAND NOT FIND");
            break;
        }
        }
        STM32_COM.flag_process_command = false;
    }
    //data_uart_received.clear();
}


/*
 *Scan Serial Port
*/
void SYNO24::on_btn_scanport_released()
{
    ui->cbx_comport->clear();
    const auto infos = QSerialPortInfo::availablePorts();
    // Scan serial Port
    // Add Name combobox COMPORT
    for (const QSerialPortInfo &info : infos)
    {
        ui->cbx_comport->addItem(info.portName());
        //        qDebug() << "Port Name: " << info.portName();
        //        qDebug() << "Description: " << info.description();
        //        qDebug() << "Manufacturer: " << info.manufacturer();
        //        qDebug() << "Serial Number: " << info.serialNumber();
        //        qDebug() << "Vendor Identifier: " << info.vendorIdentifier();
        //        qDebug() << "Product Identifier: " << info.productIdentifier();
    }
}


void SYNO24:: scanAndSelectPort(const QString &OldPort) {
    ui->cbx_comport->clear(); // Xóa các mục trong combobox
    const auto infos = QSerialPortInfo::availablePorts(); // Lấy danh sách các cổng COM
    int matchingIndex = -1; // Khởi tạo chỉ số cho cổng khớp với OldPort
    int currentIndex = 0;   // Biến đếm chỉ số cho combobox

    for (const QSerialPortInfo &info : infos) {
        QString portName = info.portName();
        ui->cbx_comport->addItem(portName); // Thêm cổng vào combobox
        // Debug thông tin cổng COM
        //        qDebug() << "Port Name: " << info.portName();
        //        qDebug() << "Description: " << info.description();
        //        qDebug() << "Manufacturer: " << info.manufacturer();
        //        qDebug() << "Serial Number: " << info.serialNumber();
        //        qDebug() << "Vendor Identifier: " << info.vendorIdentifier();
        //        qDebug() << "Product Identifier: " << info.productIdentifier();
        // Kiểm tra nếu port khớp với OldPort
        if (portName == OldPort) {
            matchingIndex = currentIndex; // Lưu lại chỉ số của port khớp
        }
        currentIndex++; // Tăng chỉ số
    }

    // Nếu tìm thấy cổng khớp, chọn nó trong combobox
    if (matchingIndex != -1) {
        ui->cbx_comport->setCurrentIndex(matchingIndex);
        qDebug() << "[INFO] OldPort found: " << OldPort << " at index: " << matchingIndex;
    } else {
        qDebug() << "[WARNING] OldPort not found: " << OldPort;
    }
}
/*
 * Function Open Serial port
*/
void SYNO24:: fnc_openSerialPort()
{
    //QByteArray Command_send(LENGTH_COMMAND_SEND,0);
    //Command_send[0] = CMD_ASK_VENDOR_ID; // FILL to well
    if(ui->cbx_comport->currentText().isEmpty())
    {
        QMessageBox::warning(this,"Warning", "Don't have ComPort, Check your connection!");
    }
    else
    {
        serialPort->close();
        serialPort->setPortName(ui->cbx_comport->currentText());
        serialPort->setBaudRate(BAUDRATE_UART);
        serialPort->setDataBits(QSerialPort::Data8);
        serialPort->setParity(QSerialPort::NoParity);
        serialPort->setStopBits(QSerialPort::OneStop);
        serialPort->setFlowControl(QSerialPort::NoFlowControl);

        if(STM32_COM.flag_connecttion == false)
        {
            serialPort->setReadBufferSize(LENGTH_COMMAND_RECEIVED);
            if (serialPort->open(QIODevice::ReadWrite))
            {
                ui->btn_ConnectPort->setText("Connected");
                ui->btn_ConnectPort->setCheckable(true);
                ui->btn_ConnectPort->setChecked(true);
                //ui->btn_ConnectPort->setStyleSheet("background-color: rgb(255, 0, 0)");
                if(ASK_VENDOR_ID())
                {
                    STM32_COM.flag_connecttion = true;
                    send_setting();
                    STM32_COM.currentPort = ui->cbx_comport->currentText();
                    delay_ui.delay_ms(1000);
                    // autoHome when connection to device
                    //                    QByteArray Command_send(LENGTH_COMMAND_SEND,0);
                    //                    Command_send[0] = CMD_RUN2HOME; // RUN STEPPER
                    //                    if(STM32_COM.serial_send_command_Firmware(serialPort, Command_send, 40000))
                    //                    {
                    //                        //log("CMD SEND SETTING = OK");
                    //                    }
                    //                    else
                    //                    {
                    //                        //log("CMD SEND SETTING = ERROR");
                    //                        QMessageBox::critical(this, tr("Error"), "NO DEVICE CONNECT, STARTUP AUTOHOME ERROR");
                    //                        //QMessageBox::critical(this, tr("Error: System not response, PLEASE RESTART SYSTEM! "), serialPort->errorString());
                    //                    }
#ifdef MAIN_WINDOW_DEBUG
                    qDebug()<< "Serial Connected";
#endif
                }
                else
                {
                    // 29-07-2023 thêm dòng này bắt đầu giao tiếp với firmware
                    // fimmrware gửi lệnh thành công thì mới tiếp tục chạy được
                    serialPort->close();
                    ui->btn_ConnectPort->setCheckable(false);
                    ui->btn_ConnectPort->setChecked(false);
                    QMessageBox::critical(this, tr("Error"), "NO DEVICE CONNECT");
                }

            }
            else
            {
                ui->btn_ConnectPort->setCheckable(false);
                ui->btn_ConnectPort->setChecked(false);
                QMessageBox::critical(this, tr("Error"), serialPort->errorString());
            }
        }
        else
        {
            serialPort->close();
            ui->btn_ConnectPort->setText("Disconnect");
            ui->btn_ConnectPort->setCheckable(false);
            ui->btn_ConnectPort->setChecked(false);
            STM32_COM.flag_connecttion = false;
        }
    }
}

void SYNO24:: on_btn_start_update_fw_released()
{

}

/*
 * Primming control START
*/
void SYNO24::on_btn_Primming_released()
{
    // Start Primming control
    double db_time_primming = ui->db_spbox_time_primming->value();
    global_var.primming_control.b_custom_position = ui->rdbtn_isprimming_thisPos->isChecked();
#ifdef MAIN_WINDOW_DEBUG
    qDebug()<< "b_autoPosition "  << global_var.primming_control.b_custom_position;
#endif
    QByteArray Command_send(LENGTH_COMMAND_SEND,0);
    Command_send[0] = CMD_PRIMMING; // FILL to well
    global_var.primming_control.valve[0] = ui->checkbx_valve_A1->isChecked();
    global_var.primming_control.valve[1] = ui->checkbx_valve_A2->isChecked();
    global_var.primming_control.valve[2] = ui->checkbx_valve_A3->isChecked();
    global_var.primming_control.valve[3] = ui->checkbx_valve_A4->isChecked();

    global_var.primming_control.valve[4] = ui->checkbx_valve_T1->isChecked();
    global_var.primming_control.valve[5] = ui->checkbx_valve_T2->isChecked();
    global_var.primming_control.valve[6] = ui->checkbx_valve_T3->isChecked();
    global_var.primming_control.valve[7] = ui->checkbx_valve_T4->isChecked();

    global_var.primming_control.valve[8] = ui->checkbx_valve_G1->isChecked();
    global_var.primming_control.valve[9] = ui->checkbx_valve_G2->isChecked();
    global_var.primming_control.valve[10] = ui->checkbx_valve_G3->isChecked();
    global_var.primming_control.valve[11] = ui->checkbx_valve_G4->isChecked();

    global_var.primming_control.valve[12] = ui->checkbx_valve_C1->isChecked();
    global_var.primming_control.valve[13] = ui->checkbx_valve_C2->isChecked();
    global_var.primming_control.valve[14] = ui->checkbx_valve_C3->isChecked();
    global_var.primming_control.valve[15] = ui->checkbx_valve_C4->isChecked();

    global_var.primming_control.valve[16] = ui->checkbx_valve_5->isChecked(); // I
    global_var.primming_control.valve[17] = ui->checkbx_valve_6->isChecked(); // U

    //=================================== ACTIVATOR =============================
    global_var.primming_control.valve[18] = ui->checkbx_valve_11->isChecked();
    global_var.primming_control.valve[19] = ui->checkbx_valve_12->isChecked();
    global_var.primming_control.valve[20] = ui->checkbx_valve_13->isChecked();
    global_var.primming_control.valve[21] = ui->checkbx_valve_14->isChecked();
    //==================================== TCA_in_DCM_
    global_var.primming_control.valve[22] = ui->checkbx_valve_7->isChecked();
    global_var.primming_control.valve[23] = ui->checkbx_valve_8->isChecked();
    global_var.primming_control.valve[24] = ui->checkbx_valve_9->isChecked();
    global_var.primming_control.valve[25] = ui->checkbx_valve_10->isChecked();
    //==================================== WASH_ACN_DCM_
    global_var.primming_control.valve[26] = ui->checkbx_valve_15->isChecked();
    global_var.primming_control.valve[27] = ui->checkbx_valve_16->isChecked();
    global_var.primming_control.valve[28] = ui->checkbx_valve_17->isChecked();
    global_var.primming_control.valve[29] = ui->checkbx_valve_18->isChecked();
    //==================================== OXIDATION_IODINE_
    global_var.primming_control.valve[30] = ui->checkbx_valve_19->isChecked();
    global_var.primming_control.valve[31] = ui->checkbx_valve_20->isChecked();
    global_var.primming_control.valve[32] = ui->checkbx_valve_21->isChecked();
    global_var.primming_control.valve[33] = ui->checkbx_valve_22->isChecked();
    //==================================== CAPPING_CAPA_
    global_var.primming_control.valve[34] = ui->checkbx_valve_23->isChecked();
    global_var.primming_control.valve[35] = ui->checkbx_valve_24->isChecked();
    global_var.primming_control.valve[36] = ui->checkbx_valve_25->isChecked();
    global_var.primming_control.valve[37] = ui->checkbx_valve_26->isChecked();
    //==================================== CAPPING_CAPB_
    global_var.primming_control.valve[38] = ui->checkbx_valve_27->isChecked();
    global_var.primming_control.valve[39] = ui->checkbx_valve_28->isChecked();
    global_var.primming_control.valve[40] = ui->checkbx_valve_29->isChecked();
    global_var.primming_control.valve[41] = ui->checkbx_valve_30->isChecked();
    double db_volume2sub = 0;
    uint16_t u16_volume2sub;

    for(uint8_t u8_idx = 0; u8_idx < MAX_NUMBER_VALVE; u8_idx++)
    {
        if(global_var.primming_control.valve[u8_idx] == true)
        {
            db_volume2sub = (double)(db_time_primming * 10000 - volume.valve[u8_idx].b) /  volume.valve[u8_idx].a;
            u16_volume2sub = static_cast<uint16_t>(db_volume2sub);
            volume.sub_volume(u8_idx, u16_volume2sub);
#ifdef MAIN_WINDOW_DEBUG
            qDebug()<< "primming valve " << u8_idx << "volume :"<< u16_volume2sub;
#endif
        }
    }
    global_var.primming_control.u8_time_primming_control = db_time_primming * 10;
    uint16_t idx_valve = 0;
    for(idx_valve = 0; idx_valve < MAX_NUMBER_VALVE; idx_valve++)
    {
        Command_send[idx_valve + 1] = global_var.primming_control.valve[idx_valve];
#ifdef MAIN_WINDOW_DEBUG
        qDebug()<< "primming_control" << idx_valve << Command_send[idx_valve + 1];
#endif
    }

    idx_valve++;
    qDebug()<< "idx_valve u8_time_primming_control : " << idx_valve;
    Command_send[idx_valve] = global_var.primming_control.u8_time_primming_control;
    idx_valve++;
    qDebug()<< "idx_valve b_custom_position : " << idx_valve;
    Command_send[idx_valve] = global_var.primming_control.b_custom_position;

    uint16_t timewait = global_var.primming_control.u8_time_primming_control*100 + 10000;// thoi gian bom + 10s thoi gian di chuyen
    STM32_COM.serial_send_command_Firmware(serialPort, Command_send, 5000);
}

/*
 * check all valve for primming
*/

void SYNO24::on_checkbx_sellect_all_toggled(bool checked)
{
    ui->checkbx_valve_A1->setChecked(checked);
    ui->checkbx_valve_A2->setChecked(checked);
    ui->checkbx_valve_A3->setChecked(checked);
    ui->checkbx_valve_A4->setChecked(checked);

    ui->checkbx_valve_T1->setChecked(checked);
    ui->checkbx_valve_T2->setChecked(checked);
    ui->checkbx_valve_T3->setChecked(checked);
    ui->checkbx_valve_T4->setChecked(checked);

    ui->checkbx_valve_G1->setChecked(checked);
    ui->checkbx_valve_G2->setChecked(checked);
    ui->checkbx_valve_G3->setChecked(checked);
    ui->checkbx_valve_G4->setChecked(checked);

    ui->checkbx_valve_C1->setChecked(checked);
    ui->checkbx_valve_C2->setChecked(checked);
    ui->checkbx_valve_C3->setChecked(checked);
    ui->checkbx_valve_C4->setChecked(checked);

    ui->checkbx_valve_5->setChecked(checked);
    ui->checkbx_valve_6->setChecked(checked);
    ui->checkbx_valve_7->setChecked(checked);
    ui->checkbx_valve_8->setChecked(checked);
    ui->checkbx_valve_9->setChecked(checked);
    ui->checkbx_valve_10->setChecked(checked);
    ui->checkbx_valve_11->setChecked(checked);
    ui->checkbx_valve_12->setChecked(checked);
    ui->checkbx_valve_13->setChecked(checked);
    ui->checkbx_valve_14->setChecked(checked);
    ui->checkbx_valve_15->setChecked(checked);
    ui->checkbx_valve_16->setChecked(checked);
    ui->checkbx_valve_17->setChecked(checked);
    ui->checkbx_valve_18->setChecked(checked);
    ui->checkbx_valve_19->setChecked(checked);
    ui->checkbx_valve_20->setChecked(checked);
    ui->checkbx_valve_21->setChecked(checked);
    ui->checkbx_valve_22->setChecked(checked);
    ui->checkbx_valve_23->setChecked(checked);
    ui->checkbx_valve_24->setChecked(checked);
    ui->checkbx_valve_25->setChecked(checked);
    ui->checkbx_valve_26->setChecked(checked);
    ui->checkbx_valve_27->setChecked(checked);
    ui->checkbx_valve_28->setChecked(checked);
    ui->checkbx_valve_29->setChecked(checked);
    ui->checkbx_valve_30->setChecked(checked);
}


void SYNO24::on_btn_calib_step1_released()
{
    QByteArray Command_send(LENGTH_COMMAND_SEND,0);
    cablib_valve.u32fb_time_primming_calib.Data = ui->db_Spbox_calib_time_1->value() * 1000;
    Command_send[0] = CMD_CALIBRATION_VALVE; // FILL to well
    Command_send[1] = ui->cbx_valve_selected->currentIndex(); // CHON VALVE
    Command_send[2] = cablib_valve.u32fb_time_primming_calib.Byte[0];
    Command_send[3] = cablib_valve.u32fb_time_primming_calib.Byte[1];
    Command_send[4] = cablib_valve.u32fb_time_primming_calib.Byte[2];
    Command_send[5] = cablib_valve.u32fb_time_primming_calib.Byte[3];
    STM32_COM.serial_send_command_Firmware(serialPort, Command_send, 5000);
}


void SYNO24::on_btn_calib_step2_released()
{
    QByteArray Command_send(LENGTH_COMMAND_SEND,0);
    cablib_valve.u32fb_time_primming_calib.Data = ui->db_Spbox_calib_time_2->value()* 1000;
    Command_send[0] = CMD_CALIBRATION_VALVE; // FILL to well
    Command_send[1] = ui->cbx_valve_selected->currentIndex();// CHON VALVE
    Command_send[2] = cablib_valve.u32fb_time_primming_calib.Byte[0];
    Command_send[3] = cablib_valve.u32fb_time_primming_calib.Byte[1];
    Command_send[4] = cablib_valve.u32fb_time_primming_calib.Byte[2];
    Command_send[5] = cablib_valve.u32fb_time_primming_calib.Byte[3];
    STM32_COM.serial_send_command_Firmware(serialPort, Command_send, 5000);
}
void SYNO24::on_btn_Calib_released()
{
    int16_t a1 = 0, b1 = 0 , c1 = 0, a2 = 0, b2 = 0, c2 = 0;
    quint8 u8_current_valve_selected = 0;
    c1 = ui->db_Spbox_calib_time_1->value() * 1000; // lay data tu ui
    b1 = 1;
    a1 = ui->spbox_calib_V1->value();
    c2 = ui->db_Spbox_calib_time_2->value()* 1000; // lay data tu ui
    b2 = 1;
    a2 = ui->spbox_calib_V2->value();
    float Delta, Delta_t, Delta_v;
    Delta = a1 * b2 - a2 * b1;
    Delta_t = c1 * b2 - c2 * b1;
    Delta_v = a1 * c2 - a2 * c1;
    if(Delta == 0)
    {
        QMessageBox::warning(this,"Error Calibration", "Please check your valve!");
#ifdef MAIN_WINDOW_DEBUG
        qDebug()<< "Error Calibration";
#endif
    }
    else
    {
        u8_current_valve_selected = ui->cbx_valve_selected->currentIndex();
        //global_var.valve_setting[u8_current_valve_selected].a = Delta_t / Delta;
        //global_var.valve_setting[u8_current_valve_selected].b = Delta_v / Delta;
        volume.valve[u8_current_valve_selected].a = Delta_t / Delta;
        volume.valve[u8_current_valve_selected].b = Delta_v / Delta;

        volume.valve[u8_current_valve_selected].t1= ui->db_Spbox_calib_time_1->value();
        volume.valve[u8_current_valve_selected].vol1 = a1;
        volume.valve[u8_current_valve_selected].t2 = ui->db_Spbox_calib_time_2->value();
        volume.valve[u8_current_valve_selected].vol2 = a2;
        qDebug()<< "current index" << u8_current_valve_selected;
        qDebug()<< "a" << volume.valve[u8_current_valve_selected].a;
        qDebug()<< "b" << volume.valve[u8_current_valve_selected].b;
#ifdef MAIN_WINDOW_DEBUG
        qDebug()<< "current index" << u8_current_valve_selected;
        qDebug()<< "a" << volume.valve[u8_current_valve_selected].a;
        qDebug()<< "b" << volume.valve[u8_current_valve_selected].b;
#endif
        //fnc.save_parameter_valve_calib(&global_var, filemanager.valveSetting_Path);
        volume.save_parameter_valve();
        loadValveDataToTable();
        send_setting();
    }
}


void SYNO24::on_btn_fill_50ul_released()
{
    QByteArray Command_send(LENGTH_COMMAND_SEND,0);
    quint8 u8_current_valve_selected = ui->cbx_valve_selected->currentIndex();
    cablib_valve.u32fb_time_primming_calib.Data = VOLUME_FILL_50UL * volume.valve[u8_current_valve_selected].a + volume.valve[u8_current_valve_selected].b;
    qDebug()<< "Time fill" << cablib_valve.u32fb_time_primming_calib.Data;
    Command_send[0] = CMD_CALIBRATION_VALVE; // FILL
    Command_send[1] = ui->cbx_valve_selected->currentIndex();
    Command_send[2] = cablib_valve.u32fb_time_primming_calib.Byte[0];
    Command_send[3] = cablib_valve.u32fb_time_primming_calib.Byte[1];
    Command_send[4] = cablib_valve.u32fb_time_primming_calib.Byte[2];
    Command_send[5] = cablib_valve.u32fb_time_primming_calib.Byte[3];
    //fnc.save_parameter_valve_calib(&global_var, filemanager.valveSetting_Path);
    volume.save_parameter_valve();
    STM32_COM.serial_send_command_Firmware(serialPort, Command_send, 5000);
}
//  ==========================================================================================================================================================
bool SYNO24::save_str_amidite_toJson(global_var_t* global_var, QString Json_Path)
{
    QFile file(Json_Path); // path file
    if (!(file.open(QIODevice::WriteOnly)))
    {
#ifdef PRINT_DEBUG
        qDebug()<<"Error open file"<< file.errorString();
#endif
    }
    QTextStream config_file(&file);
    QModelIndex index ;
    for(int i = 0; i < MAX_WELL_AMIDITE; i++)
    {
        index = model_table_well->index(i, 3, QModelIndex());
        //global_var->amidite_well[i].string_sequence = ui->tableView->model()->data(index).toString();
        QString sequence = ui->tableView->model()->data(index).toString();

        // Xóa khoảng trắng trong chuỗi
        sequence.replace(" ", ""); // Loại bỏ tất cả khoảng trắng

        // Cập nhật lại chuỗi đã loại bỏ khoảng trắng vào biến global
        global_var->amidite_well[i].string_sequence = sequence;

        // Set lại chuỗi đã loại bỏ khoảng trắng vào tableView
        model_table_well->setData(index, sequence, Qt::EditRole);
        // kết thúc xóa kí tự khoảng trắng
        // ================================================================


        index = model_table_well->index(i, 1, QModelIndex());
        global_var->amidite_well[i].string_name =  ui->tableView->model()->data(index).toString();


        //std::reverse(global_var->data_amidite.strcode_well[i].begin(), global_var->data_amidite.strcode_well[i].end());
#ifdef MAIN_WINDOW_DEBUG
        qDebug()<< global_var->amidite_well[i].string_sequence;
        qDebug()<< "Length Data" << global_var->amidite_well[i].string_sequence.length();
#endif
    }
    QJsonObject jsonOb_well[MAX_WELL_AMIDITE];
    // Valve
    for(int element = 0; element < MAX_WELL_AMIDITE ; element++)
    {
        jsonOb_well[element].insert("Sequence",global_var->amidite_well[element].string_sequence );
        jsonOb_well[element].insert("Name",  global_var->amidite_well[element].string_name);
    }
    QJsonObject content;
    QString  str2  = "well_";
    for(int i = 0; i < MAX_WELL_AMIDITE ; i++)
    {
        QString  str_name_obj = str2 +  QString::number(i);
        content.insert( str_name_obj, jsonOb_well[i]);
    }
    QJsonDocument document;
    document.setObject( content );
    QByteArray bytes = document.toJson( QJsonDocument::Indented );
    QFile File_Save(Json_Path); // path file
    if( File_Save.open( QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate ) )
    {
        QTextStream iStream( &File_Save );
        iStream.setCodec( "utf-8" );
        iStream << bytes;
        File_Save.close();
    }
    else
    {
#ifdef MAIN_WINDOW_DEBUG
        qDebug()<< " open file save data well";
#endif
    }
    return true;
}

void SYNO24::on_btn_save_sequence_released()
{
    save_str_amidite_toJson(&global_var, filemanager.amidite_sequence_Path);
    fnc.getData2AmiditeProcess(&global_var);
    // UPDATE LENGTH TO TABLE
    for(int row = 0; row < MAX_WELL_AMIDITE; row++)
    {
        QModelIndex index = model_table_well->index(row, 2,QModelIndex());
        model_table_well->setData(index,global_var.amidite_well[row].string_sequence.length());
    }
    calculator_volume_and_process_UI();
    Display_Protocol_to_user();
    // updateMonitor();
    MonitorPlateUpdateUI(0);
}


void SYNO24::on_btn_new_protocol_released()
{
    QString saveFileName = QFileDialog::getSaveFileName(this,
                                                        tr("Save Json File"),
                                                        filemanager.protocol_Dir,
                                                        tr("JSON (*.json)"));
    QFile File_Save(saveFileName);
    if( File_Save.open( QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate ) )
    {
        File_Save.close();
        filemanager.protocol_Path = saveFileName;
        ui->lineEdit_path_protocol->setText(saveFileName);
    }
    else
    {
        QMessageBox::warning(this, "Warning", " Can't Create or Read this file, Please try again!!!");
    }
}


void SYNO24::on_btn_open_protocol_released()
{
    filemanager.protocol_Path = QFileDialog::getOpenFileName(this, "Open file", filemanager.protocol_Dir,"json(*.json)");
    if(filemanager.protocol_Dir.isEmpty())
    {
        QMessageBox::warning(this, "Warning", " File Path protocol Empty, Please try choise again!!!");
    }
    else
    {
        ui->lineEdit_path_protocol->setText(filemanager.protocol_Path);
        // 14/03/2023 mở comment này để chạy load giá trị lên ui
        fnc.read_protocol_fromJson(&protocol_oligo, filemanager.protocol_Path);
        //fnc.loadProtocol(filemanager.protocol_Path, &protocol_oligo);
        fnc.getData2AmiditeProcess(&global_var);
        load_protocol_to_ui(0, 0); // load ui khi thay co thay doi
        ui->select_sub_1->setChecked(true);
        Setstyle_groupsub(0);
    }
    Display_Protocol_to_user();
    calculator_volume_and_process_UI();
}


void SYNO24::on_btn_clear_data_step_released()
{
    //-------------------------------------------------------------------------------------------
    //ui->spbox_loop_page->setValue()
    ui->spbox_volume->setValue(0);
    ui->selected_deblock->setChecked(true);
    ui->spbox_wait_after_fill->setValue(0);
    //ui->spbx_time_fast_vacuum->setValue(0);
    // STATE 2 START
    ui->cbx_option_pressure_state_2->setCurrentIndex(0);
    ui->spbx_time_process_state_2->setValue( 0);
    ui->spbox_wait_state_2->setValue(0);

    ui->cbx_option_pressure_state_3->setCurrentIndex(0);
    ui->spbx_time_process_state_3->setValue(0);
    ui->spbox_wait_state_3->setValue(0);

    ui->cbx_option_pressure_state_4->setCurrentIndex(0);
    ui->spbx_time_process_state_4->setValue( 0);
    ui->spbox_wait_state_4->setValue(0);

    ui->cbx_option_pressure_state_5->setCurrentIndex(0);
    ui->spbx_time_process_state_5->setValue( 0);
    ui->spbox_wait_state_5->setValue(0);

    ui->cbx_option_pressure_state_6->setCurrentIndex(0);
    ui->spbx_time_process_state_6->setValue(0);
    ui->spbox_wait_state_6->setValue(0);

    ui->cbx_option_pressure_state_7->setCurrentIndex(0);
    ui->spbx_time_process_state_7->setValue(0);
    ui->spbox_wait_state_7->setValue(0);

    ui->cbx_option_pressure_state_8->setCurrentIndex(0);
    ui->spbx_time_process_state_8->setValue(0);
    ui->spbox_wait_state_8->setValue(0);

    ui->cbx_option_pressure_state_9->setCurrentIndex(0);
    ui->spbx_time_process_state_9->setValue(0);
    ui->spbox_wait_state_9->setValue(0);

    ui->cbx_option_pressure_state_10->setCurrentIndex(0);

    ui->spbx_time_process_state_10->setValue(0);
    ui->spbox_wait_state_10->setValue(0);

    ui->cbx_option_pressure_state_11->setCurrentIndex(0);
    ui->spbx_time_process_state_11->setValue(0);
    ui->spbox_wait_state_11->setValue(0);
}




void SYNO24::on_btn_savecurrent_step_released()
{
    quint8 u8_current_sub = data_ui.u8_current_SUB_edit_ui;
    quint8 u8_current_step = data_ui.u8_current_STEP_edit_ui;

    protocol_oligo.sub[u8_current_sub].u8_number_base_on_sub = spinbxNumberBaseOnSub->value();
    protocol_oligo.sub[u8_current_sub].u8_number_step_on_base = spinbxNumberStepOnBase->value();


    // function 1 bom hoa chat
    checkSelectChemical();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].fill_chemical.u8_first_type_chemical = protocol_oligo.u8_step_cycle;
    protocol_oligo.sub[u8_current_sub].douple_coupling_option = cbx_option_coupling_sub->currentIndex();
    // ==========================================================================================
    //protocol_oligo.u16_scale_volume.Data = ui->spbx_scale_volume->value();
    //protocol_oligo.u16_scale_time.Data = ui->spbx_scale_time->value();
    qDebug()<< " u8_current_step" << u8_current_step;
    qDebug()<< " chemical" << protocol_oligo.u8_step_cycle;
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].fill_chemical.u16tb_Volume.Data = ui->spbox_volume->value();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].fill_chemical.u16tb_wait_after_fill.Data = ui->spbox_wait_after_fill->value();
    // funtion mix chemical
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].fill_chemical.mix_funtion.u8_type_chemical[0] = ui->cbx_type_chemical_mix_1->currentIndex();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].fill_chemical.mix_funtion.u8_type_chemical[1] = ui->cbx_type_chemical_mix_2->currentIndex();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].fill_chemical.mix_funtion.u8_type_chemical[2] = ui->cbx_type_chemical_mix_3->currentIndex();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].fill_chemical.mix_funtion.u16tb_Volume[0].Data = ui->spbx_volume_mix_1->value();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].fill_chemical.mix_funtion.u16tb_Volume[1].Data = ui->spbx_volume_mix_2->value();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].fill_chemical.mix_funtion.u16tb_Volume[2].Data = ui->spbx_volume_mix_3->value();
    // Funtion 2
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u8_option_pressure[0] = ui->cbx_option_pressure_state_2->currentIndex();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u8_option_pressure[1] = ui->cbx_option_pressure_state_3->currentIndex();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u8_option_pressure[2] = ui->cbx_option_pressure_state_4->currentIndex();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u8_option_pressure[3] = ui->cbx_option_pressure_state_5->currentIndex();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u8_option_pressure[4] = ui->cbx_option_pressure_state_6->currentIndex();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u8_option_pressure[5] = ui->cbx_option_pressure_state_7->currentIndex();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u8_option_pressure[6] = ui->cbx_option_pressure_state_8->currentIndex();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u8_option_pressure[7] = ui->cbx_option_pressure_state_9->currentIndex();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u8_option_pressure[8] = ui->cbx_option_pressure_state_10->currentIndex();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u8_option_pressure[9] = ui->cbx_option_pressure_state_11->currentIndex();

    protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_waitting_after_time[0].Data = ui->spbox_wait_state_2->value();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_waitting_after_time[1].Data = ui->spbox_wait_state_3->value();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_waitting_after_time[2].Data  = ui->spbox_wait_state_4->value();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_waitting_after_time[3].Data  = ui->spbox_wait_state_5->value();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_waitting_after_time[4].Data  = ui->spbox_wait_state_6->value();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_waitting_after_time[5].Data = ui->spbox_wait_state_7->value();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_waitting_after_time[6].Data  = ui->spbox_wait_state_8->value();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_waitting_after_time[7].Data  = ui->spbox_wait_state_9->value();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_waitting_after_time[8].Data = ui->spbox_wait_state_10->value();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_waitting_after_time[9].Data  = ui->spbox_wait_state_11->value();

    protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_procs_time[0].Data = ui->spbx_time_process_state_2->value();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_procs_time[1].Data= ui->spbx_time_process_state_3->value();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_procs_time[2].Data = ui->spbx_time_process_state_4->value();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_procs_time[3].Data = ui->spbx_time_process_state_5->value();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_procs_time[4].Data = ui->spbx_time_process_state_6->value();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_procs_time[5].Data = ui->spbx_time_process_state_7->value();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_procs_time[6].Data = ui->spbx_time_process_state_8->value();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_procs_time[7].Data= ui->spbx_time_process_state_9->value();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_procs_time[8].Data = ui->spbx_time_process_state_10->value();
    protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_procs_time[9].Data = ui->spbx_time_process_state_11->value();
    //fnc.save_protocol_toJson(&protocol_oligo, path_file_manager.protocol_path);
    Display_Protocol_to_user();
    calculator_volume_and_process_UI();

}



void SYNO24::on_btn_save_history_released()
{
    // Khởi tạo một đối tượng Word
    QAxObject word("Word.Application");

    // Tạo một tài liệu mới
    QAxObject *documents = word.querySubObject("Documents");
    QAxObject *document = documents->querySubObject("Add()");

    // Lấy văn bản từ QLineEdit
    QString text = ui->textEdit_oligo_history_log->toPlainText();

    // Chèn văn bản vào tài liệu Word
    QAxObject *selection = word.querySubObject("Selection");
    selection->dynamicCall("TypeText(QString)", text);

    // Mở hộp thoại lưu tệp tin
    QString fileName = QFileDialog::getSaveFileName(nullptr, "Save to Word", "", "*.docx");

    // Lưu tài liệu vào tệp tin
    document->dynamicCall("SaveAs(const QString&)", fileName);

    // Đóng tài liệu và Word
    document->dynamicCall("Close()");
    word.dynamicCall("Quit()");

    // Hiển thị thông báo lưu thành công
    QMessageBox::information(nullptr, "Save to Word", "Save successful.");
}

void SYNO24::load_protocol_to_ui(quint8 u8_current_sub, quint8 u8_current_step)
{
    //-------------------------------------------------------------------------------------------
    //  ui->spbox_loop_page->setValue()
    spinbxNumberBaseOnSub->setValue(protocol_oligo.sub[u8_current_sub].u8_number_base_on_sub);
    spinbxNumberStepOnBase->setValue(protocol_oligo.sub[u8_current_sub].u8_number_step_on_base);
    cbx_option_coupling_sub->setCurrentIndex(protocol_oligo.sub[u8_current_sub].douple_coupling_option);
    ui->spbox_number_sub->setValue(protocol_oligo.u8_number_sub);
    ui->spbox_volume->setValue(protocol_oligo.sub[u8_current_sub].step[u8_current_step].fill_chemical.u16tb_Volume.Data);
    setUI_FirstChemical(protocol_oligo.sub[u8_current_sub].step[u8_current_step].fill_chemical.u8_first_type_chemical);
    //ui->cbx_type_sulphite->setCurrentIndex(protocol_oligo.sub[u8_current_sub].step[u8_current_step].fill_chemical.u8_first_type_chemical);
    //on_cbx_type_sulphite_currentIndexChanged(protocol_oligo.sub[u8_current_sub].step[u8_current_step].fill_chemical.u8_first_type_chemical);

    ui->spbox_wait_after_fill->setValue(protocol_oligo.sub[u8_current_sub].step[u8_current_step].fill_chemical.u16tb_wait_after_fill.Data);

    // mix function
    ui->spbx_volume_mix_1->setValue(protocol_oligo.sub[u8_current_sub].step[u8_current_step].fill_chemical.mix_funtion.u16tb_Volume[0].Data);
    ui->spbx_volume_mix_2->setValue(protocol_oligo.sub[u8_current_sub].step[u8_current_step].fill_chemical.mix_funtion.u16tb_Volume[1].Data);
    ui->spbx_volume_mix_3->setValue(protocol_oligo.sub[u8_current_sub].step[u8_current_step].fill_chemical.mix_funtion.u16tb_Volume[2].Data);
    ui->cbx_type_chemical_mix_1->setCurrentIndex(protocol_oligo.sub[u8_current_sub].step[u8_current_step].fill_chemical.mix_funtion.u8_type_chemical[0]);
    ui->cbx_type_chemical_mix_2->setCurrentIndex(protocol_oligo.sub[u8_current_sub].step[u8_current_step].fill_chemical.mix_funtion.u8_type_chemical[1]);
    ui->cbx_type_chemical_mix_3->setCurrentIndex(protocol_oligo.sub[u8_current_sub].step[u8_current_step].fill_chemical.mix_funtion.u8_type_chemical[2]);
    // CONTROL PRESSURE
    ui->cbx_option_pressure_state_2->setCurrentIndex(protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u8_option_pressure[0]);
    ui->spbx_time_process_state_2->setValue( protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_procs_time[0].Data);
    ui->spbox_wait_state_2->setValue(protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_waitting_after_time[0].Data);

    ui->cbx_option_pressure_state_3->setCurrentIndex(protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u8_option_pressure[1]);
    ui->spbx_time_process_state_3->setValue(protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_procs_time[1].Data);
    ui->spbox_wait_state_3->setValue(protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_waitting_after_time[1].Data);

    ui->cbx_option_pressure_state_4->setCurrentIndex(protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u8_option_pressure[2]);
    ui->spbx_time_process_state_4->setValue( protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_procs_time[2].Data);
    ui->spbox_wait_state_4->setValue(protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_waitting_after_time[2].Data);

    ui->cbx_option_pressure_state_5->setCurrentIndex(protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u8_option_pressure[3]);
    ui->spbx_time_process_state_5->setValue( protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_procs_time[3].Data);
    ui->spbox_wait_state_5->setValue(protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_waitting_after_time[3].Data);

    ui->cbx_option_pressure_state_6->setCurrentIndex(protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u8_option_pressure[4]);
    ui->spbx_time_process_state_6->setValue(protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_procs_time[4].Data);
    ui->spbox_wait_state_6->setValue(protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_waitting_after_time[4].Data);

    ui->cbx_option_pressure_state_7->setCurrentIndex(protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u8_option_pressure[5]);
    ui->spbx_time_process_state_7->setValue(protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_procs_time[5].Data);
    ui->spbox_wait_state_7->setValue(protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_waitting_after_time[5].Data);

    ui->cbx_option_pressure_state_8->setCurrentIndex(protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u8_option_pressure[6]);
    ui->spbx_time_process_state_8->setValue(protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_procs_time[6].Data);
    ui->spbox_wait_state_8->setValue(protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_waitting_after_time[6].Data);

    ui->cbx_option_pressure_state_9->setCurrentIndex(protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u8_option_pressure[7]);
    ui->spbx_time_process_state_9->setValue(protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_procs_time[7].Data);
    ui->spbox_wait_state_9->setValue(protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_waitting_after_time[7].Data);

    ui->cbx_option_pressure_state_10->setCurrentIndex(protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u8_option_pressure[8]);
    ui->spbx_time_process_state_10->setValue(protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_procs_time[8].Data);
    ui->spbox_wait_state_10->setValue(protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_waitting_after_time[8].Data);

    ui->cbx_option_pressure_state_11->setCurrentIndex(protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u8_option_pressure[9]);
    ui->spbx_time_process_state_11->setValue(protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_procs_time[9].Data);
    ui->spbox_wait_state_11->setValue(protocol_oligo.sub[u8_current_sub].step[u8_current_step].control_pressure.u16tb_waitting_after_time[9].Data);
}

void SYNO24::on_btn_start_synthetic_released()
{
    //printArray(m_trityl.speacial_base);
    QFileInfo fileInfo(filemanager.protocol_Path);
    // Lấy tên file không bao gồm phần mở rộng
    QString baseName = fileInfo.baseName();
    qDebug() << "Log history Run Path:" << baseName;
    LogHistory LogHistoryRun(baseName);
    timer_update_humidity_tempareture.stop();
    QByteArray Command_send(LENGTH_COMMAND_SEND,0);
    Command_send[0] = CMD_RUN2HOME; // RUN STEPPER
    int multiplier;
    int totalInitialWells;
    double remainingPercentage = 0;
    STM32_COM.serial_send_command_Firmware(serialPort, Command_send, 20000);
    if( syno24_machine.getAutoState() == State::STOPED)
    {
        ui->textEdit_oligo_history_log->clear();
        //.signal_status_oligo.u8_FLAG_RUN_OLIGO =  START_PROCESS_SYNTHETIC_OLIGO; // bat dau start chu trình oligo
        syno24_machine.setAutoState(State::RUNNING);
        global_var.status_and_sensor.u16tb_humidity_Preset.Data = ui->spinbx_humidity_value->value();
        float hummmidity = global_var.status_and_sensor.f_humidity / 100;
        global_var.status_and_sensor.flag_enable_auto_control_air_Nito = ui->checkbox_wait_humidity->isChecked();
        get_sensor_humidity_tempareture();
        if((global_var.status_and_sensor.flag_enable_auto_control_air_Nito == true) && (global_var.status_and_sensor.flag_have_feedback_value == true))
        {
            ui->textEdit_oligo_history_log->append("Waitting for humidity to decrease");
            wait_humidity();
            while(hummmidity >= global_var.status_and_sensor.u16tb_humidity_Preset.Data)
            {
                hummmidity = global_var.status_and_sensor.f_humidity / 100;
                delay_ui.delay_ms(500);
                get_sensor_humidity_tempareture();
                qDebug()<< "hummmidity" << hummmidity;
                qDebug()<<"preset_value_hummmidity"<< global_var.status_and_sensor.u16tb_humidity_Preset.Data;
                if(syno24_machine.getAutoState() == State::STOPED)
                    break;
                // xu ly pause
                while(syno24_machine.getAutoState() == State::PAUSE)
                {
                    delay_ui.delay_ms(100);
                    if(syno24_machine.getAutoState() == State::STOPED)
                        break;
                }
            }
            ui->textEdit_oligo_history_log->append("Finish Wait for humidity");
        }
    }
    if( syno24_machine.getAutoState() == State::RUNNING && (STM32_COM.flag_connecttion == true))
    {
        // 05-04-2025 tính năng tính toán phần trăm hoàn thành của toàn bộ RUN


        //        uint8_t current_sequence[96];
        //        for (int i = 0; i < 30; ++i) {
        //            current_sequence[i] = 50; // Vẫn có hóa chất
        //        }
        //        for (int i = 30; i < 96; ++i) {
        //            current_sequence[i] = 127; // Trở thành giếng rỗng
        //        }

        LogHistoryRun.appendToLog(ui->textEdit_list_task_protocol->toPlainText());
        quint32 timeEstimate = 0;
        CalEstimateTimeProtocol(&timeEstimate);
        startCountdown(timeEstimate);
        QTime currentTime = QTime::currentTime();
        QString formattedTime = currentTime.toString("hh:mm:ss");
        //ui->textEdit_oligo_history_log->append("current Time : " + formattedTime);
        //global_var.signal_status_oligo.u8_FLAG_RUN_OLIGO =  START_PROCESS_SYNTHETIC_OLIGO; // bat dau start chu trình oligo
        QByteArray Command_send(LENGTH_COMMAND_SEND,0);
        // CHỜ CẢM BIÊN ĐẠT ĐỦ ĐỘ ẨM
        ui->textEdit_oligo_history_log->clear();
        ui->textEdit_oligo_history_log->append("current Time : " + formattedTime);
        // auto primming amidite và activator
        // GiangLH 15-11
        global_var.advanced_setting.flag_auto_primming_chemical = ui->checkbox_autoPrim_amidite->isChecked();
        qDebug()<< "Auto to primming : " << global_var.advanced_setting.flag_auto_primming_chemical;
        Command_send[0] = CMD_CONTROL_AIR_START;
        Command_send[1] = 1;
        STM32_COM.serial_send_command_Firmware(serialPort, Command_send, 10000);
        ui->btn_start_synthetic->setDisabled(true);
        quint8 u8_number_step_run =  0;
        quint8 u8_number_base_run =  0;
        quint32 u32_time_oligo_process_step = 0;
        quint8 u8_number_sub_run = ui->spbox_number_sub->value(); // lấy số sub cần Run
        //qDebug()<< "number sub run"<< u8_number_sub_run;
        quint8 u8_lastest_sub = 0;
        quint16 u16_max_base_setting_protocol = 0;
        // send các setting cần thiết 31/07 tạm thời chỗ này chỉ gửi giá trị calib valve sau này phát sinh thêm sẽ dùng command này
        send_setting();
        uint8_t initial_sequence[96];
        for (int i = 0; i < 96; ++i) {
            initial_sequence[i] = global_var.amidite_well[i].u8_sequence[0]; // Giả sử giá trị khác 127 là có hóa chất
        }
        totalInitialWells  = ChemicalWells::calculateTotalChemicalWells(initial_sequence);
        qDebug() << "Total well after Run (100%):" << totalInitialWells;
        for(uint8_t ctn_sub = 0; ctn_sub < protocol_oligo.u8_number_sub; ctn_sub++)
        {
            if(syno24_machine.getAutoState() == State::STOPED)
                break;
            u16_max_base_setting_protocol = u16_max_base_setting_protocol + protocol_oligo.sub[ctn_sub].u8_number_base_on_sub;
            if(u16_max_base_setting_protocol < global_var.signal_status_oligo.u16_max_sequence_amidite_setting)
            {
                if(protocol_oligo.u8_number_sub == 1) // truong hop chi cai dat 1 sub thi sub cuoi cung chinh là sub 0
                {
                    u8_lastest_sub = 0;
                }
                else // trường hợp có nhiều sub phía sau thì cứ lấy sub tiếp theo mà chạy
                {
                    if(ctn_sub < (protocol_oligo.u8_number_sub -1))
                    {
                        u8_lastest_sub = ctn_sub + 1;
                    }
                    else
                    {
                        u8_lastest_sub = ctn_sub;
                    }
                }
            }
        }
        // kiểm tra xem sequence có dài hơn là protocol không
        int int_remain_base = global_var.signal_status_oligo.u16_max_sequence_amidite_setting - u16_max_base_setting_protocol;

        ui->textEdit_oligo_history_log->append("Start Synthesis Oligo");
        ui->textEdit_oligo_history_log->append("Protocol Included : " + QString::number(u8_number_sub_run) + " sub-protocol");
        ui->textEdit_oligo_history_log->append("Protocol setting total: " + QString::number(u16_max_base_setting_protocol) + " base");
        ui->textEdit_oligo_history_log->append("Table Sequence setting max: " + QString::number(global_var.signal_status_oligo.u16_max_sequence_amidite_setting) + " base");
        if(int_remain_base > 0) // sequence dài hơn protocol rồi // tự động tăng page cho sub
        {
            protocol_oligo.sub[u8_lastest_sub].u8_number_base_on_sub += int_remain_base;
            ui->textEdit_oligo_history_log->append("System auto continuous run "+ QString::number(int_remain_base)+ " last sequence with sub" +  QString::number(u8_lastest_sub+1));
            qDebug()<< "sub cuoi cung" << u8_lastest_sub;
            qDebug()<< "so base cua sub cuoi cung" << protocol_oligo.sub[u8_lastest_sub].u8_number_base_on_sub;
        }
        ui->lbl_status_system->setText("System Running Synthetic");
        TwoByte_to_u16 u16tb_Timefill_Volume_first_type;
        TwoByte_to_u16 u16tb_Timefill_Volume_function_mix[3];
        global_var.signal_status_oligo.u16_counter_base_finished = 0;
        ui->lbl_base_finished->setText(QString::number(global_var.signal_status_oligo.u16_counter_base_finished));
        const quint8 idx_start_sequence_amidite = 250;
        // Chạy từng sub
        ui->textEdit_status_update_fw->clear();
        currentTime = QTime::currentTime();
        formattedTime = currentTime.toString("hh:mm:ss");
        ui->textEdit_oligo_history_log->append("current Time : " + formattedTime);
        for(uint8_t u8_counter_sub_run = 0; u8_counter_sub_run < u8_number_sub_run; u8_counter_sub_run++)
        {
            // lấy số sub của protocol
            // lấy số sub của protocol
            global_var.updateSTTRun2UI.currentSub = u8_counter_sub_run; // for update UI
            u8_number_base_run = protocol_oligo.sub[u8_counter_sub_run].u8_number_base_on_sub;
            ui->textEdit_oligo_history_log->append( "Running sub : "+ QString::number(u8_counter_sub_run + 1));
            LogHistoryRun.appendToLog("Finish Run Protocol");
            for(quint8 u8_counter_base_on_sub = 0; u8_counter_base_on_sub < u8_number_base_run; u8_counter_base_on_sub++)
            {
                // 05-04-2025 thêm function tính toán
                uint8_t current_sequence[96];
                for (int i = 0; i < 96; ++i) {
                    current_sequence[i] = global_var.amidite_well[i].u8_sequence[global_var.signal_status_oligo.u16_counter_base_finished]; // Vẫn có hóa chất
                }
                QPair<int, double> result = ChemicalWells::checkRemainingChemicalWells(current_sequence, totalInitialWells);
                remainingPercentage = result.second;
                ui->progressBar_percentRun->setValue(100.0 - remainingPercentage);
                qDebug() << "Số giếng còn lại chứa hóa chất:" << result.first;
                qDebug() << "Tỉ lệ phần trăm giếng còn lại:" << QString::number(result.second, 'f', 2) + "%";
                double DonePercentage = 100.0 - remainingPercentage;
                multiplier = findMultiplierByPercentage(DonePercentage);
                if (multiplier != 100) {
                    qDebug() << "Tỉ lệ phần trăm giếng còn lại là:" << remainingPercentage << "%";
                    qDebug() << "Hệ số nhân tương ứng là:" << multiplier;
                    // Thực hiện các hành động khác với hệ số nhân này
                } else {
                    qDebug() << "Không tìm thấy hệ số nhân phù hợp cho tỉ lệ:" << remainingPercentage << "%";
                }
                ui->lbl_multipler->setText("Multiplier : " + QString::number(multiplier) + " %" );

                global_var.signal_status_oligo.u16_counter_current_base = global_var.signal_status_oligo.u16_counter_base_finished + 1;
                // check xem có double coupling
                ui->textEdit_oligo_history_log->append("    - Base : " +  QString::number(global_var.signal_status_oligo.u16_counter_base_finished + 1));
                LogHistoryRun.appendToLog("    - Base : " +  QString::number(global_var.signal_status_oligo.u16_counter_base_finished + 1));
                // start get data
                u8_number_step_run = protocol_oligo.sub[u8_counter_sub_run].u8_number_step_on_base;
                for(quint8 u8_idx_well = 0; u8_idx_well < MAX_WELL_AMIDITE; u8_idx_well++)
                {
                    Command_send[idx_start_sequence_amidite + u8_idx_well] = global_var.amidite_well[u8_idx_well].u8_sequence[global_var.signal_status_oligo.u16_counter_base_finished];
                }
                for(quint8 u8_counter_step = 0; u8_counter_step < u8_number_step_run; u8_counter_step++)
                {
                    // cap nhật 09-07-2025 tính năng hiển thị ui
                    global_var.updateSTTRun2UI.currentStep = u8_counter_step;
                    UpdateUISTTRun(0);
                    // 05-06-2025 Tính năng hút khí trong hộp hóa chất
                    // get signal enable tính năng này
                    GetFeatureVacuumBox();
                    //volume.save_parameter_valve();
                    volume.ReloadUIVolumeMNG();
                    delay_ui.delay_ms(60);
                    ui->textEdit_oligo_history_log->append("        * step: " +  QString::number(u8_counter_step + 1)+" " + STEP_NAME[protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.u8_first_type_chemical]);
                    //()<< "run sub "<< u8_counter_sub_run;
                    //qDebug()<< "    number base" << u8_counter_base_on_sub<< " run step"<< u8_counter_step;
                    quint8 u8_first_chemical_temp = 127;
                    u8_first_chemical_temp = get_FirstChemical(protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.u8_first_type_chemical);

                    if(u8_first_chemical_temp == TCA_in_DCM && global_var.signal_status_oligo.u16_counter_base_finished != 0)
                    {
                        // lấy u16_counter_base_finished vì base này đếm từ 0 lên, người dùng nhập thu DMT tại base 1 thì sang base 2 mới dừng
                        // người dùng nhập là lấy DMT số base đã tổng hợp xong nên dùng biến này
                        //qDebug()<< global_var.signal_status_oligo.u16_counter_base_finished;
                        if(isbaseSpecial(global_var.signal_status_oligo.u16_counter_base_finished, m_trityl.speacial_base))
                        {

                            quint16 volume_trityl = ui->spbx_volume_trityl->value();

                            Command_send[0] = CMD_TRITYL_START;
                            // sub command 1 là hệ thống đến vị trí save state đè chặt trong hộp|| 2 là chạy đến vị trí đưa plate vào || 3 hornON
                            Command_send[1] = 3;
                            Command_send[2] = 0; // step DEBLOCK, giá trị = 1 đánh dấu dể sử dụng volume trityl người dùng
                            // volume dac biet dung cho trityl add them 12-02-2025 chua test
                            Command_send[3] =  0; // byte thap
                            Command_send[4] =  0; // byte cao// Volume cài đặt trityl
                            if(STM32_COM.serial_send_command_Firmware(serialPort, Command_send, 10000))
                            {
                                ui->textEdit_oligo_history_log->append("SAVE STATE POSITION FW OK");
                                qDebug()<<" CMD_TRITYL_START FW OK ";
                            }
                            else
                            {
                                ui->textEdit_oligo_history_log->append("SAVE STATE POSITION FW ERROR");
                                qDebug()<<" CMD_TRITYL_START FW ERROR ";
                                syno24_machine.setAutoState(State::PAUSE);// 20-01 sua thanh statemachine
                                ui->textEdit_oligo_history_log->moveCursor(QTextCursor::End);
                                ui->textEdit_oligo_history_log->insertPlainText (" = error");
                                ui->textEdit_oligo_history_log->moveCursor(QTextCursor::End);
                                ui->textEdit_oligo_history_log->append("SYSTEM AUTO PAUSED");
                                ui->textEdit_oligo_history_log->append("FIRMWARE NOT RESPONSE ************");
                            }

                            // Hoi nguoi dung co thu DMT khong
                            //                            if(QMessageBox::question(this, "TRITYL SELLECTION", "Insert collection plate Specify\n Choise Yes will move to position Trityl \n Choise No will Continuos Synthesis ") == QMessageBox::Yes)
                            //                            {
                            //                            if(QMessageBox::question(this, "TRITYL SELLECTION", "Insert collection plate Specify\n Choise Yes will move to position Trityl \n Choise No will Continuos Synthesis ") == QMessageBox::Yes)
                            //                            {
                            showTritylMessageBox();
                            qDebug()<< "tritylSelectionResult" << tritylSelectionResult;
                            if(tritylSelectionResult){
                                // Gianglh 14-02-2025 không cần đưa đến vị trí cửa nữa hãy giữ nó
                                m_trityl.bTritylProcessEnable = true;

                                Command_send[0] = CMD_TRITYL_START;
                                //  sub command = 1 là hệ thống đến vị trí save state đè chặt trong hộp|| 2 là chạy đến vị trí đưa plate vào
                                Command_send[1] = 1; // SAVE State
                                // sub command 2 tín hiệu cho biết đây là
                                Command_send[2] = 1; // step DEBLOCK, giá trị 1 đánh dấu dể sử dụng volume trityl người dùng
                                // volume dac biet dung cho trityl add them 12-02-2025 chua test
                                Command_send[3] =  (volume_trityl& 0xFF); // byte thap
                                Command_send[4] =  (volume_trityl>> 8)&0xFF; // byte cao// Volume cài đặt trityl

                                if(STM32_COM.serial_send_command_Firmware(serialPort, Command_send, 10000))
                                {
                                    ui->textEdit_oligo_history_log->append("TRITYL START FW OK");
                                    qDebug()<<" CMD_TRITYL_START FW OK ";
                                }
                                else
                                {
                                    ui->textEdit_oligo_history_log->append("TRITYL START FW ERROR");
                                    qDebug()<<" CMD_TRITYL_START FW ERROR ";
                                    syno24_machine.setAutoState(State::PAUSE);// 20-01 sua thanh statemachine
                                    ui->textEdit_oligo_history_log->moveCursor(QTextCursor::End);
                                    ui->textEdit_oligo_history_log->insertPlainText (" = error");
                                    ui->textEdit_oligo_history_log->moveCursor(QTextCursor::End);
                                    ui->textEdit_oligo_history_log->append("SYSTEM AUTO PAUSED");
                                    ui->textEdit_oligo_history_log->append("FIRMWARE NOT RESPONSE ==========================================");
                                }

                                qDebug()<<global_var.signal_status_oligo.u16_counter_base_finished;
                                printArray(m_trityl.speacial_base);
#ifdef MAIN_WINDOW_DEBUG
                                qDebug()<<" TRITYL SELLECTION";
#endif
                                // người dùng insert plate xong thì hỏi tiếp để xem coi có chạy thu DMT thật không

                                if(QMessageBox::question(this, "COMPLETE TRITYL SELLECTION?", "Insert collection plate Specify\n Choise Yes will process Trityl \n Choise No will Continuos Synthesis ") == QMessageBox::Yes)
                                {
                                    m_trityl.bTritylProcessEnable = true;
                                }
                                else
                                {
                                    m_trityl.bTritylProcessEnable = false;
                                }
                            }
                            else
                            {
                                m_trityl.bTritylProcessEnable = false;
                                Command_send[0] = CMD_TRITYL_START;
                                // sub command 1 là hệ thống đến vị trí save state đè chặt trong hộp|| 2 là chạy đến vị trí đưa plate vào || 3 hornON || hornOFF
                                Command_send[1] = 4;
                                if(STM32_COM.serial_send_command_Firmware(serialPort, Command_send, 10000))
                                {
                                    ui->textEdit_oligo_history_log->append("SAVE STATE POSITION FW OK");
                                    qDebug()<<" CMD_TRITYL_START FW OK ";
                                }
                                else
                                {
                                    ui->textEdit_oligo_history_log->append("SAVE STATE POSITION FW ERROR");
                                    qDebug()<<" CMD_TRITYL_START FW ERROR ";
                                    syno24_machine.setAutoState(State::PAUSE);// 20-01 sua thanh statemachine
                                    ui->textEdit_oligo_history_log->moveCursor(QTextCursor::End);
                                    ui->textEdit_oligo_history_log->insertPlainText (" = error");
                                    ui->textEdit_oligo_history_log->moveCursor(QTextCursor::End);
                                    ui->textEdit_oligo_history_log->append("SYSTEM AUTO PAUSED");
                                    ui->textEdit_oligo_history_log->append("FIRMWARE NOT RESPONSE ==========================================");
                                }
#ifdef MAIN_WINDOW_DEBUG
                                qDebug()<<" DON'T TRITYL SELLECTION";
#endif
                            }
                        }
                    }
                    else
                    {
                        m_trityl.bTritylProcessEnable = false;
                    }
                    ui->textEdit_status_update_fw->append("STEP RUN : " + QString::number(u8_counter_step));
                    //qDebug()<< " u8_first_type_chemical" << u8_first_chemical_temp;
                    if((u8_first_chemical_temp == COUPLING) || u8_first_chemical_temp == CAPPING) // neu la coupling amidite
                    {
                        for(quint8 idx_mix_fnc = 0; idx_mix_fnc < 3; idx_mix_fnc++)
                        {
                            u16tb_Timefill_Volume_first_type.Data = 0; // neu la function coupling thi khong bom lan 1
                            // 29/07/2023 lấy về giá trị gốc chưa qua convert firmware tự convert đối với function amidite
                            // 05-03-2024 chỉ cần gửi volume của function không cần nhiều data như vậy chỉ cần gửi 2 byte thay vì gửi 2*maxwell byte
                            global_var.amidite_well[0].u16_timefill_well[idx_mix_fnc].Data  = protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data;
                            if(protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u8_type_chemical[idx_mix_fnc] == AMIDITE)
                            {
                                for(quint8 u8_idx_well = 0; u8_idx_well < MAX_WELL_AMIDITE; u8_idx_well++)
                                {
                                    Command_send[idx_start_sequence_amidite + u8_idx_well] = global_var.amidite_well[u8_idx_well].u8_sequence[global_var.signal_status_oligo.u16_counter_base_finished];
#ifdef MAIN_WINDOW_DEBUG
                                    //qDebug()<< "u8_sequence"<<global_var.amidite_well[u8_idx_well].u8_sequence[u8_counter_base_on_sub];
#endif


                                    //                                    global_var.amidite_well[u8_idx_well].u16_timefill_well[idx_mix_fnc].Data  = fnc.valve_calculator_timefill(&global_var, global_var.amidite_well[u8_idx_well].u8_sequence[u8_counter_base_on_sub],
                                    //                                                                                                                                              protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data);
                                    //qDebug()<< "well = "+ QString::number(u8_idx_well)<< " Sequence = "+ QString::number(global_var.amidite_well[u8_idx_well].u8_sequence[u8_counter_base_on_sub])<< " volume = "<<  global_var.amidite_well[u8_idx_well].u16_timefill_well[idx_mix_fnc].Data;
                                    //                                    ui->textEdit_status_update_fw->append("AMIDITE");
                                    //                                    ui->textEdit_status_update_fw->append("volume " + QString::number(protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data)
                                    //                                                                          + " time " + QString::number(global_var.amidite_well[u8_idx_well].u16_timefill_well[idx_mix_fnc].Data));
                                    //volume.sub_volume_amidite();
                                    // TRỪ ĐẾM HOÁ CHẤT ================================================================================================================
                                    volume.sub_volume_amidite(global_var.amidite_well[u8_idx_well].u8_sequence[global_var.signal_status_oligo.u16_counter_base_finished],
                                            protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data);

                                }
                            }
                            else // KHONG PHAI COUPLING AMIDITE
                            {
                                // 13-04-2024 sua lai gui volume draw cho fw tu tinh thoi gian bom
                                //                                u16tb_Timefill_Volume_function_mix[idx_mix_fnc].Data = volume.valve_calculator_timefill(protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u8_type_chemical[idx_mix_fnc],
                                //                                                                                                                        protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data);
                                u16tb_Timefill_Volume_function_mix[idx_mix_fnc].Data = protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data;
                                //qDebug()<< "no amidite "<< u16tb_Timefill_Volume_function_mix[idx_mix_fnc].Data;
                                //                                ui->textEdit_status_update_fw->append("chemical");
                                //                                ui->textEdit_status_update_fw->append("volume  " + QString::number( protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data)
                                //                                                                      + " time  " + QString::number(u16tb_Timefill_Volume_function_mix[idx_mix_fnc].Data));
                                for(quint8 u8_idx_well = 0; u8_idx_well < MAX_WELL_AMIDITE; u8_idx_well++)
                                {
                                    if(global_var.amidite_well[u8_idx_well].u8_sequence[global_var.signal_status_oligo.u16_counter_base_finished] != CHEMICAL_SUBTANCE_EMPTY)
                                    {
                                        volume.sub_volume(protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u8_type_chemical[idx_mix_fnc],protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data);
                                    }
                                }
                            }
                        } // end 3 mix function
                    } // end CAPPING || COUPLING
                    else // khong phai CAPPING || COUPLING
                    {
                        // 13-04-2024 sua lai gui volume draw cho fw tu tinh thoi gian bom
                        //                        u16tb_Timefill_Volume_first_type.Data = volume.valve_calculator_timefill(u8_first_chemical_temp,
                        //                                                                                                 protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.u16tb_Volume.Data);
                        u16tb_Timefill_Volume_first_type.Data = protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.u16tb_Volume.Data;
                        //                        ui->textEdit_status_update_fw->append("chemical");
                        //                        ui->textEdit_status_update_fw->append("volume " + QString::number( protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.u16tb_Volume.Data)
                        //                                                              + " time " + QString::number(u16tb_Timefill_Volume_first_type.Data));
#ifdef MAIN_WINDOW_DEBUG
                        //qDebug()<< "NO amidite "<< "chemical" << protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.u8_first_type_chemical << " timefill :" << u16tb_Timefill_Volume_first_type.Data;
#endif
                        for(quint8 u8_idx_well = 0; u8_idx_well < MAX_WELL_AMIDITE; u8_idx_well++)
                        {
                            if(global_var.amidite_well[u8_idx_well].u8_sequence[global_var.signal_status_oligo.u16_counter_base_finished] != CHEMICAL_SUBTANCE_EMPTY)
                            {
                                volume.sub_volume(u8_first_chemical_temp,protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.u16tb_Volume.Data);
                            }
                        }
                    }

                    // GET AUTO PRIMMING AMIDITE AND ACTIVATOR -GIANGLH 16-11-2024
                    global_var.advanced_setting.u16tb_autoPrim_volume_amidite.Data = ui->spinbx_volume_prim_amidite->value();
                    global_var.advanced_setting.u16tb_autoPrim_volume_Activator.Data = ui->spinbx_volume_prim_activator->value();
                    // 17-02-2024 and command sequence and kill for function kill seuquence
                    Command_send[0] = CMD_SEUQENCE_AND_KILL;
                    int idx_sequence =1;
                    int idx_killsequence = 97;
                    for(int idx = 0; idx < MAX_WELL_AMIDITE; idx++)
                    {
                        Command_send[idx_sequence] =global_var.amidite_well[idx].u8_sequence[global_var.signal_status_oligo.u16_counter_base_finished];
                        idx_sequence++;
                        Command_send[idx_killsequence] = global_var.signal_kill.well_index[idx];
                        idx_killsequence++;
                    }
                    qDebug()<< "sequence amidite idx : "<< idx_sequence;
                    qDebug()<< "kill sequence idx : "<< idx_killsequence;
                    if(STM32_COM.serial_send_command_Firmware(serialPort, Command_send, 1500))
                    {

                    }
                    else
                    {

                    }
                    // command data GET AMIDITE old version dùng cái này cho dòng 1559 -> dời xuống đây để command CMD_SEUQENCE_AND_KILL
                    // nếu không chạy thì debug cái này
                    for(quint8 u8_idx_well = 0; u8_idx_well < MAX_WELL_AMIDITE; u8_idx_well++)
                    {
                        Command_send[idx_start_sequence_amidite + u8_idx_well] = global_var.amidite_well[u8_idx_well].u8_sequence[global_var.signal_status_oligo.u16_counter_base_finished];
                    }
                    Command_send[0] = CMD_DATA_OLIGO;
                    Command_send[1] = u8_first_chemical_temp;
                    Command_send[2] = u16tb_Timefill_Volume_first_type.Byte[0];
                    Command_send[3] = u16tb_Timefill_Volume_first_type.Byte[1];
                    Command_send[4] = protocol_oligo.sub[u8_counter_sub_run].douple_coupling_option;

                    Command_send[5] = u16tb_Timefill_Volume_function_mix[0].Byte[0];
                    Command_send[6] = u16tb_Timefill_Volume_function_mix[0].Byte[1];
                    Command_send[7] = u16tb_Timefill_Volume_function_mix[1].Byte[0];
                    Command_send[8] = u16tb_Timefill_Volume_function_mix[1].Byte[1];
                    Command_send[9] = u16tb_Timefill_Volume_function_mix[2].Byte[0];
                    Command_send[10] = u16tb_Timefill_Volume_function_mix[2].Byte[1];

                    Command_send[11] = protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u8_type_chemical[0];
                    Command_send[12] = protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u8_type_chemical[1];
                    Command_send[13] = protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u8_type_chemical[2];
                    Command_send[14] = protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.u16tb_wait_after_fill.Byte[0];
                    Command_send[15] = protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.u16tb_wait_after_fill.Byte[1];
                    // FLAG control enable auto control NITO
                    Command_send[16] = global_var.status_and_sensor.flag_enable_auto_control_air_Nito;
                    // ========= KIEM TRA BASE HIEN TAI CO PHAI LA BASE DAC BIET KHÔNG
                    if(isbaseSpecial(global_var.signal_status_oligo.u16_counter_current_base, protocol_oligo.speacial_base))
                    {
                        Command_send[17] = true;
                        Command_send[18] = protocol_oligo.u16_scale_volume.Byte[0]; // nhan ty le volume
                        Command_send[19] = protocol_oligo.u16_scale_volume.Byte[1];
                        Command_send[20] = protocol_oligo.u16_scale_time.Byte[0]; // ty le thoi gian
                        Command_send[21] = protocol_oligo.u16_scale_time.Byte[1];
                    }
                    else
                    {
                        Command_send[17] = false;
                    }
                    Command_send[22] =  (global_var.signal_status_oligo.u16_counter_base_finished)&0xFF;
                    Command_send[23] =  (global_var.signal_status_oligo.u16_counter_base_finished << 8)&0xFF;

                    Command_send[24] =  (global_var.signal_status_oligo.u16_counter_base_finished)&0xFF;
                    Command_send[25] =  (global_var.signal_status_oligo.u16_counter_base_finished << 8)&0xFF;
                    Command_send[26] =  u8_counter_step;
                    Command_send[27] = global_var.advanced_setting.flag_auto_primming_chemical;
                    Command_send[28] = global_var.advanced_setting.u16tb_autoPrim_volume_amidite.Byte[0];
                    Command_send[29] = global_var.advanced_setting.u16tb_autoPrim_volume_amidite.Byte[1];
                    Command_send[30] = global_var.advanced_setting.u16tb_autoPrim_volume_Activator.Byte[0];
                    Command_send[31] = global_var.advanced_setting.u16tb_autoPrim_volume_Activator.Byte[1];
                    /*
                        global_var.advanced_setting.FillChemistryDone.EnableFillWellDone = ui->chkbxEnableReagentFillDone;
                        global_var.advanced_setting.FillChemistryDone.En_WASH = ui->chkbxDeblock->isChecked();
                        global_var.advanced_setting.FillChemistryDone.En_Deblock = ui->chkbxDeBlock->isChecked();
                        global_var.advanced_setting.FillChemistryDone.En_Coupling = ui->chkbxCoupling->isChecked();
                        global_var.advanced_setting.FillChemistryDone.En_Cap = ui->chkbxCapping->isChecked();
                        global_var.advanced_setting.FillChemistryDone.En_Ox = ui->chkbxVacuumBoxOx->isChecked();
                        global_var.advanced_setting.FillChemistryDone.typeReagent =   ui->cbx_type_reagentDelivery->currentIndex();
                        global_var.advanced_setting.FillChemistryDone.volumeWASH.Data = ui->spbx_volume_deliveryWash->value();
                        global_var.advanced_setting.FillChemistryDone.volumeDeblock.Data = ui->spbx_volume_deliveryDeblock->value();
                        global_var.advanced_setting.FillChemistryDone.volumeCoupling.Data = ui->spbx_volume_deliveryOxidation->value();
                        global_var.advanced_setting.FillChemistryDone.volumeCap.Data = ui->spbx_volume_Coupling->value();
                        global_var.advanced_setting.FillChemistryDone.volumeOx.Data = ui->spbx_volume_deliveryCapping->value();
                     */
                    Command_send[32] = global_var.advanced_setting.FillChemistryDone.EnableFillWellDone;
                    Command_send[33] = global_var.advanced_setting.FillChemistryDone.En_WASH;
                    Command_send[34] = global_var.advanced_setting.FillChemistryDone.En_Deblock;
                    Command_send[35] = global_var.advanced_setting.FillChemistryDone.En_Coupling;
                    Command_send[36] = global_var.advanced_setting.FillChemistryDone.En_Deblock;
                    Command_send[37] = global_var.advanced_setting.FillChemistryDone.En_Coupling;
                    Command_send[38] = global_var.advanced_setting.FillChemistryDone.typeReagent;
                    Command_send[39] = global_var.advanced_setting.FillChemistryDone.volumeWASH.Byte[0];
                    Command_send[40] = global_var.advanced_setting.FillChemistryDone.volumeWASH.Byte[1];
                    Command_send[41] = global_var.advanced_setting.FillChemistryDone.volumeDeblock.Byte[0];
                    Command_send[43] = global_var.advanced_setting.FillChemistryDone.volumeDeblock.Byte[1];
                    Command_send[44] = global_var.advanced_setting.FillChemistryDone.volumeCoupling.Byte[0];
                    Command_send[45] = global_var.advanced_setting.FillChemistryDone.volumeCoupling.Byte[1];
                    Command_send[46] = global_var.advanced_setting.FillChemistryDone.volumeCap.Byte[0];
                    Command_send[47] = global_var.advanced_setting.FillChemistryDone.volumeCap.Byte[1];
                    Command_send[48] = global_var.advanced_setting.FillChemistryDone.volumeOx.Byte[0];
                    Command_send[49] = global_var.advanced_setting.FillChemistryDone.volumeOx.Byte[1];

                    const quint8 idx_start_opt_vaccum = 50;// 10byte tu 50 den 49
                    const quint8 idx_start_time_process = 60;// 20byte tuu 60 den 79
                    const quint8 idx_start_time_wait = 80; // 20byte tuu 80 den 99
                    const quint8 idx_volume_func_mixed = 100; // 20byte tu 90 den 109
                    const quint16 idx_VacuumBox = 350; // 20byte tu 90 den 109
                    u32_time_oligo_process_step = 0;

                    // multiplier tính năng cập nhật 05-04-2025
                    TwoByte_to_u16 u16tb_procs_time[10];

                    for(uint8_t idx_process = 0; idx_process < 10; idx_process++)
                    {
                        // 01-08-2025 cập nhật thành cho phép chọn lựa nhân hay không nhân tỉ lệ
                        bool LowpushMul = ui->chkbx_LowPushMul->isChecked();
                        bool HighpushMul = ui->chkbx_HighPushMul->isChecked();

                        uint8_t option = protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].control_pressure.u8_option_pressure[idx_process];
                        uint16_t original_time = protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].control_pressure.u16tb_procs_time[idx_process].Data;

                        if(option == LOW_PUSH && LowpushMul)
                        {
                            u16tb_procs_time[idx_process].Data = original_time * multiplier / 100;
                            qDebug() << "LOW_PUSH multiplier applied:" << u16tb_procs_time[idx_process].Data;
                        }
                        else if(option == HIGH_PUSH && HighpushMul)
                        {
                            u16tb_procs_time[idx_process].Data = original_time * multiplier / 100;
                            qDebug() << "HIGH_PUSH multiplier applied:" << u16tb_procs_time[idx_process].Data;
                        }
                        else
                        {
                            // Mặc định giữ nguyên giá trị
                            u16tb_procs_time[idx_process].Data = original_time;
                            qDebug() << "No multiplier applied for option:" << option;
                        }
                        /*
                        if(protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].control_pressure.u8_option_pressure[idx_process] == LOW_PUSH && (LowpushMul == true))
                        {
                            u16tb_procs_time[idx_process].Data = protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].control_pressure.u16tb_procs_time[idx_process].Data * multiplier / 100;
                            qDebug()<<"LOW_PUSH multiplier " << u16tb_procs_time[idx_process].Data;
                        }
                        if(protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].control_pressure.u8_option_pressure[idx_process] == HIGH_PUSH && (HighpushMul == true))
                        {
                            u16tb_procs_time[idx_process].Data = protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].control_pressure.u16tb_procs_time[idx_process].Data * multiplier / 100;
                            qDebug()<<" HIGH_PUSHmultiplier " << u16tb_procs_time[idx_process].Data;
                        }
                        if(LowpushMul == false  && HighpushMul == false )
                        {
                            u16tb_procs_time[idx_process].Data = protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].control_pressure.u16tb_procs_time[idx_process].Data;
                        }
                        */
                    }

                    for(uint8_t idx_process = 0; idx_process < 10; idx_process++)
                    {
                        Command_send[idx_start_opt_vaccum + idx_process]            = protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].control_pressure.u8_option_pressure[idx_process];
                        Command_send[idx_start_time_process + idx_process*2]        = u16tb_procs_time[idx_process].Byte[0];
                        Command_send[idx_start_time_process + idx_process*2 + 1]    = u16tb_procs_time[idx_process].Byte[1];
                        Command_send[idx_start_time_wait + idx_process*2]           = protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].control_pressure.u16tb_waitting_after_time[idx_process].Byte[0];
                        Command_send[idx_start_time_wait + idx_process*2 +1]        = protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].control_pressure.u16tb_waitting_after_time[idx_process].Byte[1];
                        u32_time_oligo_process_step +=  protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].control_pressure.u16tb_procs_time[idx_process].Data;
                        u32_time_oligo_process_step +=  protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].control_pressure.u16tb_waitting_after_time[idx_process].Data;
                    }
                    u32_time_oligo_process_step += 360000; // thoi gian doi command
#ifdef MAIN_WINDOW_DEBUG
                    //qDebug()<<"u32_time_oligo_process_step" << u32_time_oligo_process_step ;
#endif
                    // SYNO96 thay đổi chỉ cần gửi volume amidte của function 1 2 3 là được lấy dữ liệu tại giếng số 0
                    // đối với SYNO24 software phải gửi tổng cộng volume của 24well và 3 funtion
                    // thay doi đối với syno96 17-04-24 thay doi so voi syno24 gui volume firmware tu tinh thoi gian bom
                    Command_send[idx_volume_func_mixed ]       = global_var.amidite_well[0].u16_timefill_well[0].Byte[0];
                    Command_send[idx_volume_func_mixed +1]    = global_var.amidite_well[0].u16_timefill_well[0].Byte[1];
                    Command_send[idx_volume_func_mixed + 2]       = global_var.amidite_well[0].u16_timefill_well[1].Byte[0];
                    Command_send[idx_volume_func_mixed + 3]    = global_var.amidite_well[0].u16_timefill_well[1].Byte[1];
                    Command_send[idx_volume_func_mixed + 4]       = global_var.amidite_well[0].u16_timefill_well[2].Byte[0];
                    Command_send[idx_volume_func_mixed + 5]    = global_var.amidite_well[0].u16_timefill_well[2].Byte[1];
#ifdef MAIN_WINDOW_DEBUG
                    qDebug()<<"Function Mix ";
                    qDebug()<<"fnc 1: "<< global_var.amidite_well[0].u16_timefill_well[0].Data;
                    qDebug()<<"fnc 2: "<< global_var.amidite_well[0].u16_timefill_well[1].Data;
                    qDebug()<<"fnc 3: "<< global_var.amidite_well[0].u16_timefill_well[2].Data;
                    qDebug()<<"Send command data";
#endif
                    if(isbaseSpecial((global_var.signal_status_oligo.u16_counter_base_finished +1), m_baseVacuumBox.speacial_base))
                    {
                        int currentIdx = idx_VacuumBox; // Biến theo dõi chỉ số hiện tại
                        Command_send[currentIdx]= global_var.advanced_setting.VacuumBox.Enablefeature;
                        currentIdx++;
                        Command_send[currentIdx]= global_var.advanced_setting.VacuumBox.En_WASH;
                        currentIdx++;
                        Command_send[currentIdx]= global_var.advanced_setting.VacuumBox.En_Deblock;
                        currentIdx++;
                        Command_send[currentIdx]= global_var.advanced_setting.VacuumBox.En_Coupling;
                        currentIdx++;
                        Command_send[currentIdx]= global_var.advanced_setting.VacuumBox.En_Ox;
                        currentIdx++;
                        Command_send[currentIdx]= global_var.advanced_setting.VacuumBox.En_Cap;
                        currentIdx++;
                        Command_send[currentIdx]= global_var.advanced_setting.VacuumBox.time.Byte[0];
                        currentIdx++;
                        Command_send[currentIdx]= global_var.advanced_setting.VacuumBox.time.Byte[1];
                        //currentIdx++;
                    }else
                    {
                        int currentIdx = idx_VacuumBox; // Biến theo dõi chỉ số hiện tại
                        Command_send[currentIdx]= false;
                    }


                    if(STM32_COM.serial_send_command_Firmware(serialPort, Command_send, 10000))
                    {
#ifdef MAIN_WINDOW_DEBUG
                        qDebug()<<QTime::currentTime();
                        qDebug()<<"FW receive successful";
                        qDebug()<<"START OLIGO";
#endif
                        Command_send[0] = CMD_START_OLIGO_STEP;
                        //global_var.signal_status_oligo.u8_FLAG_FINISHED_PROCESS = false;
                        if(STM32_COM.serial_send_command_Firmware(serialPort, Command_send, u32_time_oligo_process_step))
                        {
                            //global_var.signal_status_oligo.u8_FLAG_FINISHED_PROCESS = true; // da finished roi
                            ui->textEdit_oligo_history_log->moveCursor(QTextCursor::End);
                            ui->textEdit_oligo_history_log->insertPlainText (" >> Ok");
                            ui->textEdit_oligo_history_log->moveCursor(QTextCursor::End);
                            // Double coupling process
                            delay_ui.delay_ms(60);
                            if(u8_first_chemical_temp == COUPLING)
                            {
                                switch (protocol_oligo.sub[u8_counter_sub_run].douple_coupling_option)
                                {
                                case DOUBLE_COUPLING_FIRSTBASE:
                                {
                                    if(u8_counter_base_on_sub == 0)
                                    {
                                        Command_send[0] = CMD_START_OLIGO_STEP;
                                        ui->textEdit_oligo_history_log->append("Run Double Coupling ");
                                        // global_var.signal_status_oligo.u8_FLAG_FINISHED_PROCESS = false;
                                        if(STM32_COM.serial_send_command_Firmware(serialPort, Command_send, u32_time_oligo_process_step))
                                        {
                                            //global_var.signal_status_oligo.u8_FLAG_FINISHED_PROCESS = true; // da finished roi
                                            ui->textEdit_oligo_history_log->moveCursor(QTextCursor::End);
                                            ui->textEdit_oligo_history_log->insertPlainText (" = ok");
                                            ui->textEdit_oligo_history_log->moveCursor(QTextCursor::End);
                                        }
                                        else
                                        {
                                            //global_var.signal_status_oligo.u8_FLAG_FINISHED_PROCESS = true; // da finished roi
                                            // 29-07-2023 THÊM TÍNH NĂNG TỰ ĐỘNG NGỪNG KHI GIAO TIẾP VỚI FIRMWARE KHÔNG THÀNH CÔNG
                                            //global_var.signal_status_oligo.b_FLAG_PAUSE_OLIGO = true;
                                            syno24_machine.setAutoState(State::PAUSE);// 20-01 sua thanh statemachine
                                            ui->textEdit_oligo_history_log->moveCursor(QTextCursor::End);
                                            ui->textEdit_oligo_history_log->insertPlainText (" = error");
                                            ui->textEdit_oligo_history_log->moveCursor(QTextCursor::End);
                                            ui->textEdit_oligo_history_log->append("SYSTEM AUTO PAUSED");
                                            ui->textEdit_oligo_history_log->append("FIRMWARE NOT RESPONSE ==========================================");
                                        }
                                        // TÍNH TOÁN TRỪ HOÁ CHẤT tại bước DOUBLE COUPLING
                                        for(quint8 idx_mix_fnc = 0; idx_mix_fnc < 3; idx_mix_fnc++)
                                        {
                                            if(protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u8_type_chemical[idx_mix_fnc] == AMIDITE)
                                            {
                                                for(quint8 u8_idx_well = 0; u8_idx_well < MAX_WELL_AMIDITE; u8_idx_well++)
                                                {
                                                    // 29/07/2023 lấy về giá trị gốc chưa qua convert firmware tự convert đối với function amidite
                                                    //global_var.amidite_well[u8_idx_well].u16_timefill_well[idx_mix_fnc].Data  = protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data;
                                                    volume.sub_volume(global_var.amidite_well[u8_idx_well].u8_sequence[global_var.signal_status_oligo.u16_counter_base_finished],
                                                            protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data);
                                                }

                                            }
                                            else // KHONG PHAI COUPLING AMIDITE
                                            {

                                                //                            u16tb_Timefill_Volume_function_mix[idx_mix_fnc].Data = fnc.valve_calculator_timefill(&global_var, protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u8_type_chemical[idx_mix_fnc],
                                                //                                                                                                                 protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data);
                                                qDebug()<< "KHONG PHAI COUPLING AMIDITE";
                                                for(quint8 u8_idx_well = 0; u8_idx_well < MAX_WELL_AMIDITE; u8_idx_well++)
                                                {
                                                    if(global_var.amidite_well[u8_idx_well].u8_sequence[global_var.signal_status_oligo.u16_counter_base_finished] != CHEMICAL_SUBTANCE_EMPTY)
                                                    {
                                                        volume.sub_volume_amidite(protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u8_type_chemical[idx_mix_fnc],protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data);
                                                    }
                                                }
                                            }
                                        } // end 3 mix function
                                        // HẾT BƯỚC TRỪ HOÁ CHẤT
                                    }
                                    break;
                                }
                                case DOUBLE_COUPLING_FIRST_SECOND_BASE:
                                {
                                    // double 2 base
                                    if(u8_counter_base_on_sub == 0 || u8_counter_base_on_sub == 1)
                                    {
                                        Command_send[0] = CMD_START_OLIGO_STEP;
                                        ui->textEdit_oligo_history_log->append("Run Double Coupling ");
                                        //global_var.signal_status_oligo.u8_FLAG_FINISHED_PROCESS = false; // chua finished
                                        if(STM32_COM.serial_send_command_Firmware(serialPort, Command_send, u32_time_oligo_process_step))
                                        {
                                            //global_var.signal_status_oligo.u8_FLAG_FINISHED_PROCESS = true; // finished

                                            ui->textEdit_oligo_history_log->moveCursor(QTextCursor::End);
                                            ui->textEdit_oligo_history_log->insertPlainText (" >>> Completed");
                                            ui->textEdit_oligo_history_log->moveCursor(QTextCursor::End);
                                        }
                                        else
                                        {
                                            // 29-07-2023 THÊM TÍNH NĂNG TỰ ĐỘNG NGỪNG KHI GIAO TIẾP VỚI FIRMWARE KHÔNG THÀNH CÔNG
                                            syno24_machine.setAutoState(State::PAUSE);// 20-01 sua thanh statemachine
                                            ui->textEdit_oligo_history_log->append("SYSTEM AUTO PAUSED");
                                            ui->textEdit_oligo_history_log->append("FIRMWARE NOT RESPONSE ==============");
                                        }
                                        // TÍNH TOÁN TRỪ HOÁ CHẤT tại bước DOUBLE COUPLING
                                        for(quint8 idx_mix_fnc = 0; idx_mix_fnc < 3; idx_mix_fnc++)
                                        {
                                            if(protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u8_type_chemical[idx_mix_fnc] == AMIDITE)
                                            {
                                                for(quint8 u8_idx_well = 0; u8_idx_well < MAX_WELL_AMIDITE; u8_idx_well++)
                                                {
                                                    // 29/07/2023 lấy về giá trị gốc chưa qua convert firmware tự convert đối với function amidite
                                                    //global_var.amidite_well[u8_idx_well].u16_timefill_well[idx_mix_fnc].Data  = protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data;
                                                    volume.sub_volume(global_var.amidite_well[u8_idx_well].u8_sequence[global_var.signal_status_oligo.u16_counter_base_finished],
                                                            protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data);
                                                }

                                            }
                                            else // KHONG PHAI COUPLING AMIDITE
                                            {

                                                //                            u16tb_Timefill_Volume_function_mix[idx_mix_fnc].Data = fnc.valve_calculator_timefill(&global_var, protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u8_type_chemical[idx_mix_fnc],
                                                //                                                                                                                 protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data);
                                                qDebug()<< "KHONG PHAI COUPLING AMIDITE";
                                                for(quint8 u8_idx_well = 0; u8_idx_well < MAX_WELL_AMIDITE; u8_idx_well++)
                                                {
                                                    if(global_var.amidite_well[u8_idx_well].u8_sequence[global_var.signal_status_oligo.u16_counter_base_finished] != CHEMICAL_SUBTANCE_EMPTY)
                                                    {
                                                        volume.sub_volume_amidite(protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u8_type_chemical[idx_mix_fnc],protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data);
                                                    }
                                                }
                                            }
                                        } // end 3 mix function
                                        // HẾT BƯỚC TRỪ HOÁ CHẤT
                                    }
                                    break;
                                }
                                case DOUBLE_COUPLING_ALL_BASE:
                                {
                                    Command_send[0] = CMD_START_OLIGO_STEP;
                                    //global_var.signal_status_oligo.u8_FLAG_FINISHED_PROCESS = false; // chua finished
                                    ui->textEdit_oligo_history_log->append("Run Double Coupling ");
                                    if(STM32_COM.serial_send_command_Firmware(serialPort, Command_send, u32_time_oligo_process_step))
                                    {
                                        //global_var.signal_status_oligo.u8_FLAG_FINISHED_PROCESS = true; // da finished roi
                                        ui->textEdit_oligo_history_log->moveCursor(QTextCursor::End);
                                        ui->textEdit_oligo_history_log->insertPlainText (" >>> Completed");
                                        ui->textEdit_oligo_history_log->moveCursor(QTextCursor::End);
                                    }
                                    else
                                    {
                                        // 29-07-2023 THÊM TÍNH NĂNG TỰ ĐỘNG NGỪNG KHI GIAO TIẾP VỚI FIRMWARE KHÔNG THÀNH CÔNG
                                        //global_var.signal_status_oligo.b_FLAG_PAUSE_OLIGO = true;
                                        ui->textEdit_oligo_history_log->append("SYSTEM AUTO PAUSED");
                                        //global_var.signal_status_oligo.u8_FLAG_FINISHED_PROCESS = true; // da finished roi
                                        syno24_machine.setAutoState(State::PAUSE); // 20-01 sua thanh statemachine
                                        ui->textEdit_oligo_history_log->append("FIRMWARE NOT RESPONSE ==============");
                                    }
                                    // TÍNH TOÁN TRỪ HOÁ CHẤT tại bước DOUBLE COUPLING
                                    for(quint8 idx_mix_fnc = 0; idx_mix_fnc < 3; idx_mix_fnc++)
                                    {
                                        if(protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u8_type_chemical[idx_mix_fnc] == AMIDITE)
                                        {
                                            for(quint8 u8_idx_well = 0; u8_idx_well < MAX_WELL_AMIDITE; u8_idx_well++)
                                            {
                                                // 29/07/2023 lấy về giá trị gốc chưa qua convert firmware tự convert đối với function amidite
                                                //global_var.amidite_well[u8_idx_well].u16_timefill_well[idx_mix_fnc].Data  = protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data;
                                                volume.sub_volume(global_var.amidite_well[u8_idx_well].u8_sequence[global_var.signal_status_oligo.u16_counter_base_finished],
                                                        protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data);
                                            }
                                        }
                                        else // KHONG PHAI COUPLING AMIDITE
                                        {

                                            //                            u16tb_Timefill_Volume_function_mix[idx_mix_fnc].Data = fnc.valve_calculator_timefill(&global_var, protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u8_type_chemical[idx_mix_fnc],
                                            //                                                                                                                 protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data);
                                            qDebug()<< "KHONG PHAI COUPLING AMIDITE";
                                            for(quint8 u8_idx_well = 0; u8_idx_well < MAX_WELL_AMIDITE; u8_idx_well++)
                                            {
                                                if(global_var.amidite_well[u8_idx_well].u8_sequence[global_var.signal_status_oligo.u16_counter_base_finished] != CHEMICAL_SUBTANCE_EMPTY)
                                                {
                                                    volume.sub_volume_amidite(protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u8_type_chemical[idx_mix_fnc],protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data);
                                                }
                                            }
                                        }
                                    } // end 3 mix function
                                    // HẾT BƯỚC TRỪ HOÁ CHẤT
                                    break;
                                }
                                default:
                                {
                                    break;
                                }
                                } // end switch
                            } // END IF double coupling
                        }
                        else // start oligo khong thanh cong = PAUSE va bao loi
                        {
                            //global_var.signal_status_oligo.u8_FLAG_FINISHED_PROCESS = true; // da finished roi
                            // 29-07-2023 THÊM TÍNH NĂNG TỰ ĐỘNG NGỪNG KHI GIAO TIẾP VỚI FIRMWARE KHÔNG THÀNH CÔNG
                            //global_var.signal_status_oligo.b_FLAG_PAUSE_OLIGO = true;
                            // 20-01
                            // 20-01 sua thanh statemachine
                            syno24_machine.setAutoState(State::PAUSE); // 20-01 sua thanh statemachine
                            ui->textEdit_oligo_history_log->append("SYSTEM AUTO PAUSED");
                            ui->textEdit_oligo_history_log->append("FIRMWARE NOT RESPONSE ==============");
                            break;
                        }
                    }
                    else // 20-01-24 gui data oligo khong thanh cong
                    {
                        // 29-07-2023 THÊM TÍNH NĂNG TỰ ĐỘNG NGỪNG KHI GIAO TIẾP VỚI FIRMWARE KHÔNG THÀNH CÔNG
                        //global_var.signal_status_oligo.b_FLAG_PAUSE_OLIGO = true;
                        syno24_machine.setAutoState(State::PAUSE); // 20-01 sua thanh statemachine
                        ui->textEdit_oligo_history_log->append("SYSTEM AUTO PAUSED");
                        ui->textEdit_oligo_history_log->append("FIRMWARE feedback Error in this step - Please Restart SYSTEM");

#ifdef MAIN_WINDOW_DEBUG
                        qDebug()<<"FW feadback error";
                        break;
#endif
                    }
                    // TRIYTL
                    // XỬ lý thu Trityl Continous or STOP
                    if(m_trityl.bTritylProcessEnable == true)
                    {
                        m_trityl.bTritylProcessEnable = false;
                        // lenh fw di chuyen den vi tri lay plate ra
                        // sau khi lay plate ra se doi người dùng quyết định có tiếp tục tổng hợp không
                        Command_send[0] = CMD_TRITYL_STOP;
                        if(STM32_COM.serial_send_command_Firmware(serialPort, Command_send, 10000))
                        {
                            ui->textEdit_oligo_history_log->append("TRITYL STOP FW OK");
                            qDebug()<<" CMD_TRITYL_STOP FW OK ";
                        }
                        else
                        {
                            ui->textEdit_oligo_history_log->append("TRITYL STOP FW ERROR");
                            qDebug()<<" CMD_TRITYL_STOP FW ERROR ";
                            syno24_machine.setAutoState(State::PAUSE);// 20-01 sua thanh statemachine
                            ui->textEdit_oligo_history_log->moveCursor(QTextCursor::End);
                            ui->textEdit_oligo_history_log->insertPlainText (" = error");
                            ui->textEdit_oligo_history_log->moveCursor(QTextCursor::End);
                            ui->textEdit_oligo_history_log->append("SYSTEM AUTO PAUSED");
                            ui->textEdit_oligo_history_log->append("FIRMWARE NOT RESPONSE == CMD_TRITYL_STOP FW");
                        }
                        // hộp thoại đợi người dùng tiếp tục
                        if(QMessageBox::question(this, "TRITYL SELLECTION COMPLETED", "Select Yes to continue synthesizing the oligo, or No to stop the synthesis") == QMessageBox::Yes)
                        {
                            // tiếp tục tổng hợp
                            //m_trityl.bTritylProcessEnable = true;
#ifdef MAIN_WINDOW_DEBUG
                            qDebug()<<" Continous after Trityl collection";
#endif
                        }
                        else // không tiếp tục tổng hợp
                        {
                            syno24_machine.setAutoState(State::STOPED);
#ifdef MAIN_WINDOW_DEBUG

                            qDebug()<<" STOP MACHINE AFTER TRITYL SELLECTION";
#endif
                        }
                    }
                    if(syno24_machine.getAutoState() == State::STOPED)
                        break;

                }// for run all step of base
                global_var.signal_status_oligo.u16_counter_base_finished++;
                // monitor plate lên màn hình
                MonitorPlateUpdateUI(global_var.signal_status_oligo.u16_counter_base_finished); //
                ui->lbl_base_finished->setText(QString::number(global_var.signal_status_oligo.u16_counter_base_finished));
                //xu ly truong hop hết sequence
                /// GET GIA TRI NHIET DO - DO AM FORM FIRMWARE
                delay_ui.delay_ms(5);
                if((global_var.signal_status_oligo.u16_counter_base_finished % 1) == 0)
                {
                    //                    //ui->textEdit_oligo_history_log->append(">>>>EXHAUSTED_CHEMICAL");
                    //                    Command_send[0] = CMD_FIRMWARE_UPDATE_STATUS_AND_SENSOR;
                    //                    if(STM32_COM.serial_send_command_Firmware(serialPort, Command_send, 10000))
                    //                    {
                    //                        //                        ui->textEdit_oligo_history_log->moveCursor(QTextCursor::End);
                    //                        //                        ui->textEdit_oligo_history_log->insertPlainText (" >>> Completed");
                    //                        //                        ui->textEdit_oligo_history_log->moveCursor(QTextCursor::End);
                    //                    }
                    get_sensor_humidity_tempareture();
                    ui->textEdit_oligo_history_log->append("Humidity: " + QString::number(global_var.status_and_sensor.f_humidity / 100));
                    currentTime = QTime::currentTime();
                    formattedTime = currentTime.toString("hh:mm:ss");
                    ui->textEdit_oligo_history_log->append("current Time : " + formattedTime);
                }
                if(syno24_machine.getAutoState() == State::PAUSE)
                {
                    ui->textEdit_oligo_history_log->append("SYSTEM AUTO PAUSE");
                }
                // xu ly pause
                while(syno24_machine.getAutoState() == State::PAUSE)
                {
                    delay_ui.delay_ms(100);
                    if(syno24_machine.getAutoState() == State::STOPED)
                        break;
                }
                if(syno24_machine.getAutoState() == State::STOPED)
                    break;
                // nếu số base dài hơn sequence
                if(global_var.signal_status_oligo.u16_counter_base_finished >= global_var.signal_status_oligo.u16_max_sequence_amidite_setting)
                {
                    ui->textEdit_oligo_history_log->append("SYSTEM AUTO STOPED, SEQUENCE OLIGO IS OVER");
                    syno24_machine.setAutoState(State::STOPED);
                    //global_var.signal_status_oligo.u8_FLAG_RUN_OLIGO = STOP_PROCESS_SYNTHETIC_OLIGO;
                }
                if(syno24_machine.getAutoState() == State::STOPED)
                    break;
            }// vòng lặp chạy các base

            if(syno24_machine.getAutoState() == State::STOPED)
                break;
            delay_ui.delay_ms(20);
        }// vòng lặp chạy hết các sub
        // 05-04-2025 ============================== TINH TOAN % hien thi len ui
        // 05-04-2025 thêm function tính toán
        uint8_t current_sequence[96];
        for (int i = 0; i < 96; ++i) {
            current_sequence[i] = global_var.amidite_well[i].u8_sequence[global_var.signal_status_oligo.u16_counter_base_finished]; // Vẫn có hóa chất
        }
        QPair<int, double> result = ChemicalWells::checkRemainingChemicalWells(current_sequence, totalInitialWells);
        remainingPercentage = result.second;
        ui->progressBar_percentRun->setValue(100.0 - remainingPercentage);
        qDebug() << "Số giếng còn lại chứa hóa chất:" << result.first;
        qDebug() << "Tỉ lệ phần trăm giếng còn lại:" << QString::number(result.second, 'f', 2) + "%";

        // DMT ON-OFF == FUNCTION DMT OFF 11-06-2024
        // kiem tra DMT off
        if(global_var.DMT_step.bSingnal_DMTOff == true)
        {
            ui->textEdit_oligo_history_log->append("DMT OFF enable");
            ui->textEdit_oligo_history_log->append("Start process DMT OFF");
            if(QMessageBox::question(this, "TRITYL SELLECTION", "Insert the collection plate and make your choice:\n Select Yes to process Trityl \n Select No to end the synthesis ") == QMessageBox::Yes)
            {
                ui->textEdit_oligo_history_log->append("Trityl sellection in progress");
                // m_trityl.bTritylProcessEnable = true;
#ifdef MAIN_WINDOW_DEBUG
                qDebug()<<" TRITYL SELLECTION";
#endif
                // lay sequence 0 de bom DEBLOCK vao cac cot can cat DMT
                for(quint8 u8_idx_well = 0; u8_idx_well < MAX_WELL_AMIDITE; u8_idx_well++)
                {
                    Command_send[idx_start_sequence_amidite + u8_idx_well] = global_var.amidite_well[u8_idx_well].u8_sequence[0];
#ifdef MAIN_WINDOW_DEBUG
                    //qDebug()<< "u8_sequence"<<global_var.amidite_well[u8_idx_well].u8_sequence[u8_counter_base_on_sub];
#endif
                }
                Command_send[0] = CMD_DATA_OLIGO;
                Command_send[1] = global_var.DMT_step.fill_chemical.u8_first_type_chemical; // DMT
                Command_send[2] = 0; // KHONG CO VOLUME
                Command_send[3] = 0;// KHONG CO VOLUME
                Command_send[4] = 0;//douple_coupling_option;

                Command_send[5] = global_var.DMT_step.fill_chemical.mix_funtion.u16tb_Volume[0].Byte[0] ;
                Command_send[6] = global_var.DMT_step.fill_chemical.mix_funtion.u16tb_Volume[0].Byte[1];
                Command_send[7] = global_var.DMT_step.fill_chemical.mix_funtion.u16tb_Volume[1].Byte[0] ;
                Command_send[8] = global_var.DMT_step.fill_chemical.mix_funtion.u16tb_Volume[1].Byte[1];
                Command_send[9] = global_var.DMT_step.fill_chemical.mix_funtion.u16tb_Volume[2].Byte[0] ;
                Command_send[10] = global_var.DMT_step.fill_chemical.mix_funtion.u16tb_Volume[2].Byte[1];

                Command_send[11] = global_var.DMT_step.fill_chemical.mix_funtion.u8_type_chemical[0];
                Command_send[12] = global_var.DMT_step.fill_chemical.mix_funtion.u8_type_chemical[1];
                Command_send[13] = global_var.DMT_step.fill_chemical.mix_funtion.u8_type_chemical[2];
                Command_send[14] = global_var.DMT_step.fill_chemical.u16tb_wait_after_fill.Byte[0];
                Command_send[15] = global_var.DMT_step.fill_chemical.u16tb_wait_after_fill.Byte[1];
                // FLAG control enable auto control NITO
                Command_send[16] = global_var.status_and_sensor.flag_enable_auto_control_air_Nito;
                // ========= KIEM TRA BASE HIEN TAI CO PHAI LA BASE DAC BIET KHÔNG
                Command_send[17] = false;
                Command_send[18] = 0;
                Command_send[19] = 0;
                Command_send[20] = 0;
                Command_send[21] = 0;
                const quint8 idx_start_opt_vaccum = 50;// 10byte tu 50 den 49
                const quint8 idx_start_time_process = 60;// 20byte tuu 60 den 79
                const quint8 idx_start_time_wait = 80; // 20byte tuu 80 den 99
                const quint8 idx_volume_func_mixed = 100; // 20byte tu 90 den 109
                u32_time_oligo_process_step = 0;
                for(uint8_t idx_process = 0; idx_process < 10; idx_process++)
                {
                    Command_send[idx_start_opt_vaccum + idx_process]            = global_var.DMT_step.control_pressure.u8_option_pressure[idx_process];
                    Command_send[idx_start_time_process + idx_process*2]        = global_var.DMT_step.control_pressure.u16tb_procs_time[idx_process].Byte[0];
                    Command_send[idx_start_time_process + idx_process*2 + 1]    = global_var.DMT_step.control_pressure.u16tb_procs_time[idx_process].Byte[1];
                    Command_send[idx_start_time_wait + idx_process*2]           = global_var.DMT_step.control_pressure.u16tb_waitting_after_time[idx_process].Byte[0];
                    Command_send[idx_start_time_wait + idx_process*2 +1]        = global_var.DMT_step.control_pressure.u16tb_waitting_after_time[idx_process].Byte[1];
                    u32_time_oligo_process_step +=  global_var.DMT_step.control_pressure.u16tb_procs_time[0].Data;
                    u32_time_oligo_process_step +=  global_var.DMT_step.control_pressure.u16tb_waitting_after_time[0].Data;
                }
                u32_time_oligo_process_step += 360000; // thoi gian doi command
#ifdef MAIN_WINDOW_DEBUG
                //qDebug()<<"u32_time_oligo_process_step" <<u32_time_oligo_process_step ;
#endif
                // SYNO96 thay đổi chỉ cần gửi volume amidte của function 1 2 3 là được lấy dữ liệu tại giếng số 0
                // đối với SYNO24 software phải gửi tổng cộng volume của 24well và 3 funtion
                // thay doi đối với syno96 17-04-24 thay doi so voi syno24 gui volume firmware tu tinh thoi gian bom
                // DMT khong can cai nay
                Command_send[idx_volume_func_mixed ]       = global_var.amidite_well[0].u16_timefill_well[0].Byte[0];
                Command_send[idx_volume_func_mixed +1]    = global_var.amidite_well[0].u16_timefill_well[0].Byte[1];
                Command_send[idx_volume_func_mixed + 2]       = global_var.amidite_well[0].u16_timefill_well[1].Byte[0];
                Command_send[idx_volume_func_mixed + 3]    = global_var.amidite_well[0].u16_timefill_well[1].Byte[1];
                Command_send[idx_volume_func_mixed + 4]       = global_var.amidite_well[0].u16_timefill_well[2].Byte[0];
                Command_send[idx_volume_func_mixed + 5]    = global_var.amidite_well[0].u16_timefill_well[2].Byte[1];

                qDebug()<<"Function Mix DMT";
                qDebug()<<"fnc 1: "<< global_var.amidite_well[0].u16_timefill_well[0].Data;
                qDebug()<<"fnc 2: "<< global_var.amidite_well[0].u16_timefill_well[1].Data;
                qDebug()<<"fnc 3: "<< global_var.amidite_well[0].u16_timefill_well[2].Data;
#ifdef MAIN_WINDOW_DEBUG
                qDebug()<<"Send command data FUNCTION DMT";

#endif
                if(STM32_COM.serial_send_command_Firmware(serialPort, Command_send, 10000))
                {
#ifdef MAIN_WINDOW_DEBUG
                    qDebug()<<"Send data DMT";
#endif
                    Command_send[0] = CMD_START_OLIGO_STEP;
                    if(STM32_COM.serial_send_command_Firmware(serialPort, Command_send, u32_time_oligo_process_step))
                    {
#ifdef MAIN_WINDOW_DEBUG
                        qDebug()<<"START DMT";
#endif
                        ui->textEdit_oligo_history_log->append("<<< Ok");
                    }
                    else // 20-01-24 gui data oligo khong thanh cong
                    {
                        syno24_machine.setAutoState(State::PAUSE); // 20-01 sua thanh statemachine
                        ui->textEdit_oligo_history_log->append("SYSTEM AUTO PAUSED");
                        ui->textEdit_oligo_history_log->append("FIRMWARE feedback Error in this step - Please Restart SYSTEM");

#ifdef MAIN_WINDOW_DEBUG
                        qDebug()<<"FW feadback error";
#endif
                    }
                }
                else // 20-01-24 gui data oligo khong thanh cong
                {
                    syno24_machine.setAutoState(State::PAUSE); // 20-01 sua thanh statemachine
                    ui->textEdit_oligo_history_log->append("SYSTEM AUTO PAUSED");
                    ui->textEdit_oligo_history_log->append("FIRMWARE feedback Error in this step - Please Restart SYSTEM");

#ifdef MAIN_WINDOW_DEBUG
                    qDebug()<<"FW feadback error";
#endif
                }
            }
            else
            {
                // m_trityl.bTritylProcessEnable = false;
#ifdef MAIN_WINDOW_DEBUG
                qDebug()<<" DON'T TRITYL SELLECTION";
#endif
            }
#ifdef MAIN_WINDOW_DEBUG
            qDebug()<<"CUT DMT";
#endif

        } // END IF CUT DMT
        else
        {
#ifdef MAIN_WINDOW_DEBUG
            qDebug()<<"KHONG CAT DMT";
#endif
        }
        //global_var.signal_status_oligo.u8_FLAG_RUN_OLIGO = STOP_PROCESS_SYNTHETIC_OLIGO;
        syno24_machine.setAutoState(State::STOPED);
        // chay ve home cho nguoi dung lay phit ra khoi khay
        //on_btn_Run2HomeStep_released();
        //global_var.signal_status_oligo.u8_FLAG_FINISHED_PROCESS = true;
        ui->btn_start_synthetic->setDisabled(false);
        ui->textEdit_oligo_history_log->append("Finish Synthesis Oligo");
        volume.save_parameter_valve(); // 14-12
        volume.ReloadUIVolumeMNG();
        syno24_machine.procsess_ui();
    }// end IF FLAG == STOP == neu flag == stop thi moi START
    else // neu flag la START thi canh bao nguoi dung
    {
        if(syno24_machine.getAutoState() != State::STOPED)
        {
            QMessageBox::warning(this,"Warning", "System Oligo Running, Please Waitting or STOP and Restart");
        }
        if(STM32_COM.flag_connecttion == false)
        {
            QMessageBox::warning(this,"Warning", "System Don't connect to Device, Please Connect Device to Run");
        }
    }
    syno24_machine.procsess_ui();
    on_btn_Run2HomeStep_released();
    /// 05-03-2024 send STOP to FW disable all system
    // 09-12 Giang Them flag và thời gian exhaustFan
    global_var.advanced_setting.flag_exhaustFan = ui->checkbox_exhaustFan->isChecked();
    global_var.advanced_setting.u16tb_timeExhaustFan.Data = ui->spinbx_exhaustFan->value();
    //QByteArray Command_send(LENGTH_COMMAND_SEND,0);
    Command_send[0] = CMD_STOP_SYSTHETIC_OLIGO;
    Command_send[1] = global_var.advanced_setting.flag_exhaustFan;
    Command_send[2] = global_var.advanced_setting.u16tb_timeExhaustFan.Byte[0];
    Command_send[3] = global_var.advanced_setting.u16tb_timeExhaustFan.Byte[1];
    //global_var.signal_status_oligo.u8_FLAG_FINISHED_PROCESS = false;
    if(STM32_COM.serial_send_command_Firmware(serialPort, Command_send, 200000))
    {
        log("STOP OK");
        qDebug()<<"STOP OK";
    }
    startCountdown(0);
    stopCountdown();
    syno24_machine.setAutoState(State::STOPED);
    timer_update_humidity_tempareture.start(interval_timer_check_sensor);
    LogHistoryRun.appendToLog(ui->textEdit_oligo_history_log->toPlainText());
    LogHistoryRun.setLogFileReadOnly();
}


void SYNO24::on_btn_pause_synthetic_released()
{
    if(QMessageBox::question(this, "Pause at the end of the cycle process", "DO YOU WANT PAUSE PROCESS") == QMessageBox::Yes)
    {
        if(syno24_machine.getAutoState() == State::PAUSE)
        {
            ui->btn_pause_synthetic->setText("PAUSE");
            ui->lbl_status_system->setText("System Running");
            syno24_machine.setAutoState(State::RUNNING);

        }
        else
        {
            syno24_machine.setAutoState(State::PAUSE);
            ui->lbl_status_system->setText("System Paused");
            ui->btn_pause_synthetic->setText("RESUME");


        }
    }
    else
    {
        qDebug()<<" DON'T STOP ALL PROCESS";
    }

}


void SYNO24::on_spbox_number_sub_valueChanged(int arg1)
{
    //    protocol_oligo.u8_number_sub = arg1;
    //    switch (arg1)
    //    {
    //    case 1:
    //    {
    //        ui->widget_sub_1->show();
    //        ui->widget_sub_2->hide();
    //        ui->widget_sub_3->hide();
    //        ui->widget_sub_4->hide();
    //        ui->widget_sub_5->hide();
    //        ui->select_sub_1->show();
    //        ui->select_sub_2->hide();
    //        ui->select_sub_3->hide();
    //        ui->select_sub_4->hide();
    //        ui->select_sub_5->hide();
    //        break;
    //    }
    //    case 2:
    //    {
    //        ui->widget_sub_1->show();
    //        ui->widget_sub_2->show();
    //        ui->widget_sub_3->hide();
    //        ui->widget_sub_4->hide();
    //        ui->widget_sub_5->hide();
    //        ui->select_sub_1->show();
    //        ui->select_sub_2->show();
    //        ui->select_sub_3->hide();
    //        ui->select_sub_4->hide();
    //        ui->select_sub_5->hide();
    //        copy_sub_protocol_data(protocol_oligo.sub[1], protocol_oligo.sub[0]);
    //        break;
    //    }
    //    case 3:
    //    {
    //        ui->widget_sub_1->show();
    //        ui->widget_sub_2->show();
    //        ui->widget_sub_3->show();
    //        ui->widget_sub_4->hide();
    //        ui->widget_sub_5->hide();
    //        ui->select_sub_1->show();
    //        ui->select_sub_2->show();
    //        ui->select_sub_3->show();
    //        ui->select_sub_4->hide();
    //        ui->select_sub_5->hide();
    //        copy_sub_protocol_data(protocol_oligo.sub[2], protocol_oligo.sub[1]);
    //        break;
    //    }
    //    case 4:
    //    {
    //        ui->widget_sub_1->show();
    //        ui->widget_sub_2->show();
    //        ui->widget_sub_3->show();
    //        ui->widget_sub_4->show();
    //        ui->widget_sub_5->hide();
    //        ui->select_sub_1->show();
    //        ui->select_sub_2->show();
    //        ui->select_sub_3->show();
    //        ui->select_sub_4->show();
    //        ui->select_sub_5->hide();
    //        copy_sub_protocol_data(protocol_oligo.sub[3], protocol_oligo.sub[2]);
    //        break;
    //    }
    //    case 5:
    //    {
    //        ui->widget_sub_1->show();
    //        ui->widget_sub_2->show();
    //        ui->widget_sub_3->show();
    //        ui->widget_sub_4->show();
    //        ui->widget_sub_5->show();
    //        ui->select_sub_1->show();
    //        ui->select_sub_2->show();
    //        ui->select_sub_3->show();
    //        ui->select_sub_4->show();
    //        ui->select_sub_5->show();
    //        copy_sub_protocol_data(protocol_oligo.sub[4], protocol_oligo.sub[3]);
    //        break;
    //    }
    //    default:
    //    {
    //        break;
    //    }
    //    }
}


void SYNO24::on_pushButton_5_released() // button save as protocol
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"), filemanager.protocol_Path, tr("JSON Files (*.json);;All Files (*)"));
    //QFileDialog::getOpenFileName(this, "Open file", path_file_manager.applicationDirpath,"json(*.json)");
    if (fileName.isEmpty())
    {
        return;
    }
    fnc.save_protocol_toJson(&protocol_oligo, fileName);// save dữ liệu vào path mới
}


void SYNO24::on_btn_stop_synthetic_released()
{
    if(QMessageBox::question(this, "STOP ALL PROCESS", "DO YOU WANT STOP PROCESS") == QMessageBox::Yes)
    {
        if(syno24_machine.getAutoState() == State::RUNNING || syno24_machine.getAutoState() == State::PAUSE)
        {
            //global_var.signal_status_oligo.u8_FLAG_RUN_OLIGO = STOP_PROCESS_SYNTHETIC_OLIGO;
            syno24_machine.setAutoState(State::STOPED);
            ui->btn_pause_synthetic->setText("PAUSE");
            ui->textEdit_oligo_history_log->append("Please Waitting for finish current Step!");
            ui->lbl_status_system->setText("System Stoped Synthetic");
#ifdef MAIN_WINDOW_DEBUG
            syno24_machine.printModeAndState();
#endif
        }
    }
    else
    {
        ui->lbl_status_system->setText("System Running Synthetic");
#ifdef MAIN_WINDOW_DEBUG
        qDebug()<<" DON'T STOP ALL PROCESS";
#endif
    }
}


void SYNO24::on_btn_RunStepper_released()
{
    QByteArray Command_send(LENGTH_COMMAND_SEND,0);
    global_var.control_stepper.u16tb_X_Distance.Data = ui->spbx_x_stepper->value();
    global_var.control_stepper.u16tb_Y_Distance.Data = ui->spbx_y_stepper->value();
    global_var.control_stepper.u16tb_Z1_Distance.Data = ui->spbx_z1_stepper->value();
    Command_send[0] = CMD_RUNSTEPPER; // RUN STEPPER
    //-------------------------------------------------------------------------
    Command_send[1] = global_var.control_stepper.u16tb_X_Distance.Byte[0];
    Command_send[2] = global_var.control_stepper.u16tb_X_Distance.Byte[1];
    Command_send[3] = global_var.control_stepper.u16tb_Y_Distance.Byte[0];
    Command_send[4] = global_var.control_stepper.u16tb_Y_Distance.Byte[1];
    Command_send[5] = global_var.control_stepper.u16tb_Z1_Distance.Byte[0];
    Command_send[6] = global_var.control_stepper.u16tb_Z1_Distance.Byte[1];
    STM32_COM.serial_send_command_Firmware(serialPort, Command_send, 5000);
}

void SYNO24::on_btn_Run2HomeStep_released()
{
    QByteArray Command_send(LENGTH_COMMAND_SEND,0);
    Command_send[0] = CMD_RUN2HOME; // RUN STEPPER
    //Command_send[1] = (ui->db_spbox_time_primming->value() *10);
    STM32_COM.serial_send_command_Firmware(serialPort, Command_send, 20000);
}

// Process UI/ UX từ chỗ này bắt đầu chỉ xử lý các lệnh software UI/UX
void SYNO24::Setstyle_groupsub(quint8 grbx)
{
    data_ui.u8_current_STEP_edit_ui = 0;
    data_ui.u8_current_SUB_edit_ui = grbx;
    ui->sub1_step_1->setChecked(true);
    qDebug()<< " new ui select_sub_ "<< data_ui.u8_current_SUB_edit_ui;
    //    switch(grbx)
    //    {
    //    case 0:
    //    {
    //        // select_sub_1 đang được chọn
    //        data_ui.u8_current_SUB_edit_ui = 0;

    //        //qDebug()<< " new ui select_sub_ "<< data_ui.u8_current_SUB_edit_ui;
    //        ui->sub1_step_1->setChecked(true);
    //        ui->widget_sub_1->setDisabled(false);
    //        ui->widget_sub_2->setDisabled(true);
    //        ui->widget_sub_3->setDisabled(true);
    //        ui->widget_sub_4->setDisabled(true);
    //        ui->widget_sub_5->setDisabled(true);

    //        ui->widget_sub_1->setStyleSheet(widget_stylesheet_enable);
    //        ui->widget_sub_2->setStyleSheet(widget_stylesheet_disable);
    //        ui->widget_sub_3->setStyleSheet(widget_stylesheet_disable);
    //        ui->widget_sub_4->setStyleSheet(widget_stylesheet_disable);
    //        ui->widget_sub_5->setStyleSheet(widget_stylesheet_disable);
    //        break;
    //    }
    //    case 1:
    //    {
    //        data_ui.u8_current_SUB_edit_ui = 1;
    //        // select_sub_2 đang được chọn
    //        ui->sub2_step_1->setChecked(true);
    //        ui->widget_sub_1->setDisabled(true);
    //        ui->widget_sub_2->setDisabled(false);
    //        ui->widget_sub_3->setDisabled(true);
    //        ui->widget_sub_4->setDisabled(true);
    //        ui->widget_sub_5->setDisabled(true);
    //        ui->widget_sub_1->setStyleSheet(widget_stylesheet_disable);
    //        ui->widget_sub_2->setStyleSheet(widget_stylesheet_enable);
    //        ui->widget_sub_3->setStyleSheet(widget_stylesheet_disable);
    //        ui->widget_sub_4->setStyleSheet(widget_stylesheet_disable);
    //        ui->widget_sub_5->setStyleSheet(widget_stylesheet_disable);
    //        //qDebug()<< "new ui  select_sub_ "<< data_ui.u8_current_SUB_edit_ui;
    //        break;
    //    }
    //    case 2:
    //    {
    //        data_ui.u8_current_SUB_edit_ui = 2;
    //        // select_sub_3 đang được chọn
    //        ui->sub3_step_1->setChecked(true);
    //        ui->widget_sub_1->setDisabled(true);
    //        ui->widget_sub_2->setDisabled(true);
    //        ui->widget_sub_3->setDisabled(false);
    //        ui->widget_sub_4->setDisabled(true);
    //        ui->widget_sub_5->setDisabled(true);
    //        ui->widget_sub_1->setStyleSheet(widget_stylesheet_disable);
    //        ui->widget_sub_2->setStyleSheet(widget_stylesheet_disable);
    //        ui->widget_sub_3->setStyleSheet(widget_stylesheet_enable);
    //        ui->widget_sub_4->setStyleSheet(widget_stylesheet_disable);
    //        ui->widget_sub_5->setStyleSheet(widget_stylesheet_disable);
    //        //qDebug()<< "new ui  select_sub_ "<<data_ui.u8_current_SUB_edit_ui;
    //        break;
    //    }
    //    case 3:
    //    {
    //        data_ui.u8_current_SUB_edit_ui = 3;
    //        ui->sub4_step_1->setChecked(true);
    //        ui->widget_sub_1->setDisabled(true);
    //        ui->widget_sub_2->setDisabled(true);
    //        ui->widget_sub_3->setDisabled(true);
    //        ui->widget_sub_4->setDisabled(false);
    //        ui->widget_sub_5->setDisabled(true);
    //        ui->widget_sub_1->setStyleSheet(widget_stylesheet_disable);
    //        ui->widget_sub_2->setStyleSheet(widget_stylesheet_disable);
    //        ui->widget_sub_3->setStyleSheet(widget_stylesheet_disable);
    //        ui->widget_sub_4->setStyleSheet(widget_stylesheet_enable);
    //        ui->widget_sub_5->setStyleSheet(widget_stylesheet_disable);
    //        // Không có select box nào được chọn
    //        //qDebug()<< "new ui select_sub_ " << data_ui.u8_current_SUB_edit_ui;
    //        break;
    //    }
    //    case 4:
    //    {
    //        data_ui.u8_current_SUB_edit_ui = 4;
    //        ui->sub5_step_1->setChecked(true);
    //        ui->widget_sub_1->setDisabled(true);
    //        ui->widget_sub_2->setDisabled(true);
    //        ui->widget_sub_3->setDisabled(true);
    //        ui->widget_sub_4->setDisabled(true);
    //        ui->widget_sub_5->setDisabled(false);
    //        ui->widget_sub_1->setStyleSheet(widget_stylesheet_disable);
    //        ui->widget_sub_2->setStyleSheet(widget_stylesheet_disable);
    //        ui->widget_sub_3->setStyleSheet(widget_stylesheet_disable);
    //        ui->widget_sub_4->setStyleSheet(widget_stylesheet_disable);
    //        ui->widget_sub_5->setStyleSheet(widget_stylesheet_enable);
    //        //qDebug()<< "new ui select_sub_ "<< data_ui.u8_current_SUB_edit_ui;
    //        break;
    //    }
    //    default:
    //    {
    //        break;
    //    }
    //    }
    load_protocol_to_ui(data_ui.u8_current_SUB_edit_ui, 0);
}

void SYNO24::checkSelected_STEP()
{
    static QRadioButton *sub_step[MAX_STEP_OF_SUB] = {
        ui->sub1_step_1,
        ui->sub1_step_2,
        ui->sub1_step_3,
        ui->sub1_step_4,
        ui->sub1_step_5,
        ui->sub1_step_6,
        ui->sub1_step_7,
        ui->sub1_step_8,
        ui->sub1_step_9,
        ui->sub1_step_10,
        ui->sub1_step_11,
        ui->sub1_step_12
    };
    for(int i = 0; i < MAX_STEP_OF_SUB; i++)
    {
        if(sub_step[i]->isChecked())
        {
            data_ui.u8_current_STEP_edit_ui = i;
            qDebug()<< "select_sub "<< data_ui.u8_current_SUB_edit_ui<< " >>step_ "<< data_ui.u8_current_STEP_edit_ui;
            break;
        }
    }
    /*
    switch(data_ui.u8_current_SUB_edit_ui)
    {
    case 0:
    {
        static QRadioButton *sub1_step[MAX_STEP_OF_SUB] = {
            ui->sub1_step_1,
            ui->sub1_step_2,
            ui->sub1_step_3,
            ui->sub1_step_4,
            ui->sub1_step_5,
            ui->sub1_step_6,
            ui->sub1_step_7,
            ui->sub1_step_8,
            ui->sub1_step_9,
            ui->sub1_step_10,
            ui->sub1_step_11,
            ui->sub1_step_12
        };
        for(int i = 0; i < MAX_STEP_OF_SUB; i++)
        {
            if(sub1_step[i]->isChecked())
            {
                data_ui.u8_current_STEP_edit_ui = i;
                qDebug()<< "select_sub "<< data_ui.u8_current_SUB_edit_ui<< " >>step_ "<< data_ui.u8_current_STEP_edit_ui;
                break;
            }
        }
        break;
    }
    case 1:
    {
        static QRadioButton *sub2_step[MAX_STEP_OF_SUB] = {
            ui->sub2_step_1,
            ui->sub2_step_2,
            ui->sub2_step_3,
            ui->sub2_step_4,
            ui->sub2_step_5,
            ui->sub2_step_6,
            ui->sub2_step_7,
            ui->sub2_step_8,
            ui->sub2_step_9,
            ui->sub2_step_10,
            ui->sub2_step_11,
            ui->sub2_step_12
        };
        for(int i = 0; i < MAX_STEP_OF_SUB; i++)
        {
            if(sub2_step[i]->isChecked())
            {
                data_ui.u8_current_STEP_edit_ui = i;
                qDebug()<< "select_sub "<< data_ui.u8_current_SUB_edit_ui<< " >>step_ "<< data_ui.u8_current_STEP_edit_ui;
                break;
            }
        }
        break;
    }
    case 2:
    {
        static QRadioButton *sub3_step[MAX_STEP_OF_SUB] = {
            ui->sub3_step_1,
            ui->sub3_step_2,
            ui->sub3_step_3,
            ui->sub3_step_4,
            ui->sub3_step_5,
            ui->sub3_step_6,
            ui->sub3_step_7,
            ui->sub3_step_8,
            ui->sub3_step_9,
            ui->sub3_step_10,
            ui->sub3_step_11,
            ui->sub3_step_12
        };
        for(int i = 0; i < MAX_STEP_OF_SUB; i++)
        {
            if(sub3_step[i]->isChecked())
            {
                data_ui.u8_current_STEP_edit_ui = i;
                qDebug()<< "select_sub "<< data_ui.u8_current_SUB_edit_ui<< " >>step_ "<< data_ui.u8_current_STEP_edit_ui;
                break;
            }
        }
        break;
    }
    case 3:
    {
        static QRadioButton *sub4_step[MAX_STEP_OF_SUB] = {
            ui->sub4_step_1,
            ui->sub4_step_2,
            ui->sub4_step_3,
            ui->sub4_step_4,
            ui->sub4_step_5,
            ui->sub4_step_6,
            ui->sub4_step_7,
            ui->sub4_step_8,
            ui->sub4_step_9,
            ui->sub4_step_10,
            ui->sub4_step_11,
            ui->sub4_step_12
        };
        for(int i = 0; i < MAX_STEP_OF_SUB; i++)
        {
            if(sub4_step[i]->isChecked())
            {
                data_ui.u8_current_STEP_edit_ui = i;
                qDebug()<< "select_sub "<< data_ui.u8_current_SUB_edit_ui<< " >>step_ "<< data_ui.u8_current_STEP_edit_ui;
                break;
            }
        }
        break;
    }
    case 4:
    {
        static QRadioButton *sub5_step[MAX_STEP_OF_SUB] = {
            ui->sub5_step_1,
            ui->sub5_step_2,
            ui->sub5_step_3,
            ui->sub5_step_4,
            ui->sub5_step_5,
            ui->sub5_step_6,
            ui->sub5_step_7,
            ui->sub5_step_8,
            ui->sub5_step_9,
            ui->sub5_step_10,
            ui->sub5_step_11,
            ui->sub5_step_12
        };
        for(int i = 0; i < MAX_STEP_OF_SUB; i++)
        {
            if(sub5_step[i]->isChecked())
            {
                data_ui.u8_current_STEP_edit_ui = i;
                qDebug()<< "select_sub "<< data_ui.u8_current_SUB_edit_ui<< " >>step_ "<< data_ui.u8_current_STEP_edit_ui;
                break;
            }
        }
        break;
    }
    default:
    {
        break;
    }
    }
    */
    load_protocol_to_ui(data_ui.u8_current_SUB_edit_ui,data_ui.u8_current_STEP_edit_ui);
}

void SYNO24::on_select_sub_1_toggled(bool checked)
{
    if(checked)
    {
        Setstyle_groupsub(0);
    }
}


void SYNO24::on_select_sub_2_toggled(bool checked)
{
    if(checked)
    {
        Setstyle_groupsub(1);
    }
}


void SYNO24::on_select_sub_3_toggled(bool checked)
{
    if(checked)
    {
        Setstyle_groupsub(2);
    }

}

void SYNO24::on_select_sub_4_toggled(bool checked)
{
    if(checked)
    {
        Setstyle_groupsub(3);
    }
}


void SYNO24::on_select_sub_5_toggled(bool checked)
{
    if(checked)
    {
        Setstyle_groupsub(4);
    }
}

void SYNO24::on_select_sub_6_toggled(bool checked)
{
    if(checked)
    {
        Setstyle_groupsub(5);
    }
}


void SYNO24::on_select_sub_7_toggled(bool checked)
{
    if(checked)
    {
        Setstyle_groupsub(6);
    }
}

void SYNO24::on_select_sub_8_toggled(bool checked)
{
    if(checked)
    {
        Setstyle_groupsub(7);
    }
}

void SYNO24::on_select_sub_9_toggled(bool checked)
{
    if(checked)
    {
        Setstyle_groupsub(8);
    }
}



void SYNO24::on_select_sub_10_toggled(bool checked)
{
    if(checked)
    {
        Setstyle_groupsub(9);
    }
}




void SYNO24::checkSelectChemical()
{
    static quint8 u8_current_option_chemical;
    static quint8 u8_chemical_odinal;
    static QRadioButton *optionChemical_step[6] =
    {
        ui->selected_deblock,
        ui->selected_washing,
        ui->selected_coupling,
        ui->selected_oxidation,
        ui->selected_capping,
        ui->selected_mix_fnc
    };
    u8_current_option_chemical = -1;
    for(int i = 0; i < 6; i++)
    {
        if(optionChemical_step[i]->isChecked())
        {
            u8_current_option_chemical = i;
            qDebug()<< "option "<< u8_current_option_chemical<< STEP_NAME[u8_current_option_chemical];
            // break;
        }
    }

    switch(u8_current_option_chemical)
    {
    case DEBLOCK_FNC:
    {
        u8_chemical_odinal = TCA_in_DCM;
        ui->stackedWidget->setCurrentIndex(0);
        ui->lbl_step_name->setText(STEP_NAME[u8_current_option_chemical]);
        break;
    }
    case WASH_FNC:
    {
        ui->stackedWidget->setCurrentIndex(0);
        u8_chemical_odinal = WASH_ACN_DCM;
        ui->lbl_step_name->setText(STEP_NAME[u8_current_option_chemical]);
        break;
    }
    case COUPLING_FNC:
    {
        ui->stackedWidget->setCurrentIndex(1);
        u8_chemical_odinal = COUPLING;
        ui->lbl_step_name->setText(STEP_NAME[u8_current_option_chemical]);
        break;
    }
    case OXICATION_FNC:
    {
        ui->stackedWidget->setCurrentIndex(0);
        u8_chemical_odinal = OXIDATION_IODINE;
        ui->lbl_step_name->setText(STEP_NAME[u8_current_option_chemical]);
        break;
    }
    case CAP_FNC:
    {
        ui->stackedWidget->setCurrentIndex(1);
        u8_chemical_odinal = CAPPING;
        ui->lbl_step_name->setText(STEP_NAME[u8_current_option_chemical]);
        break;
    }
    case MIXED_FNC:
    {
        ui->stackedWidget->setCurrentIndex(1);
        u8_chemical_odinal = FUNTION_MIXED;
        ui->lbl_step_name->setText(STEP_NAME[u8_current_option_chemical]);
        break;
    }
    default :
    {
        ui->lbl_step_name->setText("ERROR");
        QMessageBox::warning(this, tr("Error"), tr("Please choice Chemical"));
        break;
    }
    }
    protocol_oligo.u8_step_cycle = u8_current_option_chemical;
    //qDebug()<< "choice" << u8_current_option_chemical;
}

void SYNO24::setUI_FirstChemical(quint8 u8_step_cycle)
{
    switch(u8_step_cycle)
    {
    case DEBLOCK_FNC:
    {

        ui->stackedWidget->setCurrentIndex(0);
        ui->selected_deblock->setChecked(true);
        ui->lbl_step_name->setText(STEP_NAME[u8_step_cycle]);
        break;
    }
    case WASH_FNC:
    {
        ui->stackedWidget->setCurrentIndex(0);
        ui->selected_washing->setChecked(true);
        ui->lbl_step_name->setText(STEP_NAME[u8_step_cycle]);
        break;
    }
    case COUPLING_FNC:
    {
        ui->stackedWidget->setCurrentIndex(1);
        ui->selected_coupling->setChecked(true);
        ui->lbl_step_name->setText(STEP_NAME[u8_step_cycle]);
        break;
    }
    case OXICATION_FNC:
    {
        ui->stackedWidget->setCurrentIndex(0);
        ui->selected_oxidation->setChecked(true);
        ui->lbl_step_name->setText(STEP_NAME[u8_step_cycle]);
        break;
    }
    case CAP_FNC:
    {
        ui->selected_capping->setChecked(true);
        ui->stackedWidget->setCurrentIndex(1);
        ui->lbl_step_name->setText(STEP_NAME[u8_step_cycle]);
        break;
    }
    case MIXED_FNC:
    {
        ui->selected_mix_fnc->setChecked(true);
        ui->stackedWidget->setCurrentIndex(1);
        ui->lbl_step_name->setText(STEP_NAME[u8_step_cycle]);
        break;
    }
    default :
    {
        ui->stackedWidget->setCurrentIndex(0);
        ui->selected_deblock->setChecked(true);
        ui->lbl_step_name->setText(STEP_NAME[u8_step_cycle]);

        break;
    }
    }
}

quint8 SYNO24::get_FirstChemical(quint8 u8_chemical)
{
    quint8 u8_chemical_odinal = 0;
    switch(u8_chemical)
    {
    case DEBLOCK_FNC:
    {
        u8_chemical_odinal = TCA_in_DCM;
        break;
    }
    case WASH_FNC:
    {
        u8_chemical_odinal = WASH_ACN_DCM;
        break;
    }
    case COUPLING_FNC:
    {
        u8_chemical_odinal = COUPLING;

        break;
    }
    case OXICATION_FNC:
    {
        u8_chemical_odinal = OXIDATION_IODINE;//_IODINE
        break;
    }
    case CAP_FNC:
    {
        u8_chemical_odinal = CAPPING;
        break;
    }
    case MIXED_FNC:
    {
        u8_chemical_odinal = FUNTION_MIXED;
        break;
    }
    default :
    {
        break;
    }
    }
    return u8_chemical_odinal;
}

void SYNO24::on_btn_ManualRun_released() // ManualRun
{
    if(syno24_machine.getAutoState() == State::STOPED)
    {
        if(syno24_machine.setManualMode())
        {
            ui->stackedWidget_Run->setCurrentIndex(1); // go to stack manualRun
        }
    }
    else
    {
        QMessageBox::warning(this,"Warning", "SYSTEM BUSY RUNNING AUTO MODE Wait and try again!");
    }
}


void SYNO24::on_btn_backAtuoRun_released() // backtoAutoRun
{
    if(syno24_machine.getManualState() == State::STOPED)
    {
        if(syno24_machine.setAutoMode())
        {
            ui->stackedWidget_Run->setCurrentIndex(0);
        }
    }
    else
    {
        QMessageBox::warning(this,"Warning", "System Running MANUAL MODE, Please Waitting for STOP");
    }
}
/*
void SYNO24::on_checkbx_manual_column_1_toggled(bool checked)
{
    ui->well_1->setChecked(checked);
    ui->well_2->setChecked(checked);
    ui->well_3->setChecked(checked);
    ui->well_4->setChecked(checked);
    ui->well_5->setChecked(checked);
    ui->well_6->setChecked(checked);
    ui->well_7->setChecked(checked);
    ui->well_8->setChecked(checked);
}


void SYNO24::on_checkbx_manual_column_2_toggled(bool checked)
{
    ui->well_9->setChecked(checked);
    ui->well_10->setChecked(checked);
    ui->well_11->setChecked(checked);
    ui->well_12->setChecked(checked);
    ui->well_13->setChecked(checked);
    ui->well_14->setChecked(checked);
    ui->well_15->setChecked(checked);
    ui->well_16->setChecked(checked);
}


void SYNO24::on_checkbx_manual_column_3_toggled(bool checked)
{
    ui->well_17->setChecked(checked);
    ui->well_18->setChecked(checked);
    ui->well_19->setChecked(checked);
    ui->well_20->setChecked(checked);
    ui->well_21->setChecked(checked);
    ui->well_22->setChecked(checked);
    ui->well_23->setChecked(checked);
    ui->well_24->setChecked(checked);
}
*/

void SYNO24::on_checkbx_manual_Allwell_toggled(bool checked)
{
    for(uint8_t idx=0; idx < MAX_WELL_AMIDITE; idx++)
    {
        checkbx_manual_well[idx]->setChecked(checked);
    }
    //    ui->well_1->setChecked(checked);
    //    ui->well_2->setChecked(checked);
    //    ui->well_3->setChecked(checked);
    //    ui->well_4->setChecked(checked);
    //    ui->well_5->setChecked(checked);
    //    ui->well_6->setChecked(checked);
    //    ui->well_7->setChecked(checked);
    //    ui->well_8->setChecked(checked);
    //    ui->well_9->setChecked(checked);
    //    ui->well_10->setChecked(checked);
    //    ui->well_11->setChecked(checked);
    //    ui->well_12->setChecked(checked);
    //    ui->well_13->setChecked(checked);
    //    ui->well_14->setChecked(checked);
    //    ui->well_15->setChecked(checked);
    //    ui->well_16->setChecked(checked);
    //    ui->well_17->setChecked(checked);
    //    ui->well_18->setChecked(checked);
    //    ui->well_19->setChecked(checked);
    //    ui->well_20->setChecked(checked);
    //    ui->well_21->setChecked(checked);
    //    ui->well_22->setChecked(checked);
    //    ui->well_23->setChecked(checked);
    //    ui->well_24->setChecked(checked);
}

void SYNO24:: Run_Manual_fill_Chemical()
{

    //syno24_machine.setManualState(STOP);
}
void SYNO24:: Run_Manual_CtrlVacuum()
{


}
void SYNO24::on_btn_StartManual_released() // fill chemical
{
    send_setting();
    // 26-03-2024 sua tinh nang va cau truc cua manual run nhung chua test
    // FIRMWARE VA SOFTWARE CHUA TEST
    syno24_machine.setManualState(State::RUNNING);
    uint16_t u16_volume_temp = ui->spbx_volume_manual->value();
    global_var.manual_run.u8_typeof_chemical = ui->cbx_type_chemical_manual_run->currentIndex();
    // 17-04-24 từ syno96 không tính thời gian nữa mà gửi trực tiếp volume
    ///global_var.manual_run.u16_volume.Data = volume.valve_calculator_timefill(global_var.manual_run.u8_typeof_chemical, u16_volume_temp);
    global_var.manual_run.u16_volume.Data = u16_volume_temp;
    uint16_t u16_time_wait_firmware = 0;
    QByteArray Command_send(LENGTH_COMMAND_SEND,0);
    Command_send[0] = CMD_MANUAL_RUN; // FILL to well
    Command_send[1] = 1;// option manual == fill chemical to well
    Command_send[2] = global_var.manual_run.u8_typeof_chemical;
    Command_send[3] = global_var.manual_run.u16_volume.Byte[0];
    Command_send[4] = global_var.manual_run.u16_volume.Byte[1];
    global_var.manual_run.u8_checked_well[0] = ui->well_1->isChecked();
    global_var.manual_run.u8_checked_well[1] = ui->well_2->isChecked();
    global_var.manual_run.u8_checked_well[2] = ui->well_3->isChecked();
    global_var.manual_run.u8_checked_well[3] = ui->well_4->isChecked();
    global_var.manual_run.u8_checked_well[4] = ui->well_5->isChecked();
    global_var.manual_run.u8_checked_well[5] = ui->well_6->isChecked();
    global_var.manual_run.u8_checked_well[6] = ui->well_7->isChecked();
    global_var.manual_run.u8_checked_well[7] = ui->well_8->isChecked();
    global_var.manual_run.u8_checked_well[8] = ui->well_9->isChecked();
    global_var.manual_run.u8_checked_well[9] = ui->well_10->isChecked();
    global_var.manual_run.u8_checked_well[10] = ui->well_11->isChecked();
    global_var.manual_run.u8_checked_well[11] = ui->well_12->isChecked();
    global_var.manual_run.u8_checked_well[12] = ui->well_13->isChecked();
    global_var.manual_run.u8_checked_well[13] = ui->well_14->isChecked();
    global_var.manual_run.u8_checked_well[14] = ui->well_15->isChecked();
    global_var.manual_run.u8_checked_well[15] = ui->well_16->isChecked();
    global_var.manual_run.u8_checked_well[16] = ui->well_17->isChecked();
    global_var.manual_run.u8_checked_well[17] = ui->well_18->isChecked();
    global_var.manual_run.u8_checked_well[18] = ui->well_19->isChecked();
    global_var.manual_run.u8_checked_well[19] = ui->well_20->isChecked();
    global_var.manual_run.u8_checked_well[20] = ui->well_21->isChecked();
    global_var.manual_run.u8_checked_well[21] = ui->well_22->isChecked();
    global_var.manual_run.u8_checked_well[22] = ui->well_23->isChecked();
    global_var.manual_run.u8_checked_well[23] = ui->well_24->isChecked();

    global_var.manual_run.u8_checked_well[24] = ui->well_25->isChecked();
    global_var.manual_run.u8_checked_well[25] = ui->well_26->isChecked();
    global_var.manual_run.u8_checked_well[26] = ui->well_27->isChecked();
    global_var.manual_run.u8_checked_well[27] = ui->well_28->isChecked();
    global_var.manual_run.u8_checked_well[28] = ui->well_29->isChecked();
    global_var.manual_run.u8_checked_well[29] = ui->well_30->isChecked();
    global_var.manual_run.u8_checked_well[30] = ui->well_31->isChecked();
    global_var.manual_run.u8_checked_well[31] = ui->well_32->isChecked();
    global_var.manual_run.u8_checked_well[32] = ui->well_33->isChecked();
    global_var.manual_run.u8_checked_well[33] = ui->well_34->isChecked();
    global_var.manual_run.u8_checked_well[34] = ui->well_35->isChecked();
    global_var.manual_run.u8_checked_well[35] = ui->well_36->isChecked();
    global_var.manual_run.u8_checked_well[36] = ui->well_37->isChecked();
    global_var.manual_run.u8_checked_well[37] = ui->well_38->isChecked();
    global_var.manual_run.u8_checked_well[38] = ui->well_39->isChecked();
    global_var.manual_run.u8_checked_well[39] = ui->well_40->isChecked();
    global_var.manual_run.u8_checked_well[40] = ui->well_41->isChecked();
    global_var.manual_run.u8_checked_well[41] = ui->well_42->isChecked();
    global_var.manual_run.u8_checked_well[42] = ui->well_43->isChecked();
    global_var.manual_run.u8_checked_well[43] = ui->well_44->isChecked();
    global_var.manual_run.u8_checked_well[44] = ui->well_45->isChecked();
    global_var.manual_run.u8_checked_well[45] = ui->well_46->isChecked();
    global_var.manual_run.u8_checked_well[46] = ui->well_47->isChecked();
    global_var.manual_run.u8_checked_well[47] = ui->well_48->isChecked();

    global_var.manual_run.u8_checked_well[48] = ui->well_49->isChecked();
    global_var.manual_run.u8_checked_well[49] = ui->well_50->isChecked();
    global_var.manual_run.u8_checked_well[50] = ui->well_51->isChecked();
    global_var.manual_run.u8_checked_well[51] = ui->well_52->isChecked();
    global_var.manual_run.u8_checked_well[52] = ui->well_53->isChecked();
    global_var.manual_run.u8_checked_well[53] = ui->well_54->isChecked();
    global_var.manual_run.u8_checked_well[54] = ui->well_55->isChecked();
    global_var.manual_run.u8_checked_well[55] = ui->well_56->isChecked();
    global_var.manual_run.u8_checked_well[56] = ui->well_57->isChecked();
    global_var.manual_run.u8_checked_well[57] = ui->well_58->isChecked();
    global_var.manual_run.u8_checked_well[58] = ui->well_59->isChecked();
    global_var.manual_run.u8_checked_well[59] = ui->well_60->isChecked();
    global_var.manual_run.u8_checked_well[60] = ui->well_61->isChecked();
    global_var.manual_run.u8_checked_well[61] = ui->well_62->isChecked();
    global_var.manual_run.u8_checked_well[62] = ui->well_63->isChecked();
    global_var.manual_run.u8_checked_well[63] = ui->well_64->isChecked();
    global_var.manual_run.u8_checked_well[64] = ui->well_65->isChecked();
    global_var.manual_run.u8_checked_well[65] = ui->well_66->isChecked();
    global_var.manual_run.u8_checked_well[66] = ui->well_67->isChecked();
    global_var.manual_run.u8_checked_well[67] = ui->well_68->isChecked();
    global_var.manual_run.u8_checked_well[68] = ui->well_69->isChecked();
    global_var.manual_run.u8_checked_well[69] = ui->well_70->isChecked();
    global_var.manual_run.u8_checked_well[70] = ui->well_71->isChecked();
    global_var.manual_run.u8_checked_well[71] = ui->well_72->isChecked();

    global_var.manual_run.u8_checked_well[72] = ui->well_73->isChecked();
    global_var.manual_run.u8_checked_well[73] = ui->well_74->isChecked();
    global_var.manual_run.u8_checked_well[74] = ui->well_75->isChecked();
    global_var.manual_run.u8_checked_well[75] = ui->well_76->isChecked();
    global_var.manual_run.u8_checked_well[76] = ui->well_77->isChecked();
    global_var.manual_run.u8_checked_well[77] = ui->well_78->isChecked();
    global_var.manual_run.u8_checked_well[78] = ui->well_79->isChecked();
    global_var.manual_run.u8_checked_well[79] = ui->well_80->isChecked();
    global_var.manual_run.u8_checked_well[80] = ui->well_81->isChecked();
    global_var.manual_run.u8_checked_well[81] = ui->well_82->isChecked();
    global_var.manual_run.u8_checked_well[82] = ui->well_83->isChecked();
    global_var.manual_run.u8_checked_well[83] = ui->well_84->isChecked();
    global_var.manual_run.u8_checked_well[84] = ui->well_85->isChecked();
    global_var.manual_run.u8_checked_well[85] = ui->well_86->isChecked();
    global_var.manual_run.u8_checked_well[86] = ui->well_87->isChecked();
    global_var.manual_run.u8_checked_well[87] = ui->well_88->isChecked();
    global_var.manual_run.u8_checked_well[88] = ui->well_89->isChecked();
    global_var.manual_run.u8_checked_well[89] = ui->well_90->isChecked();
    global_var.manual_run.u8_checked_well[90] = ui->well_91->isChecked();
    global_var.manual_run.u8_checked_well[91] = ui->well_92->isChecked();
    global_var.manual_run.u8_checked_well[92] = ui->well_93->isChecked();
    global_var.manual_run.u8_checked_well[93] = ui->well_94->isChecked();
    global_var.manual_run.u8_checked_well[94] = ui->well_95->isChecked();
    global_var.manual_run.u8_checked_well[95] = ui->well_96->isChecked();

    // trừ hoá chất trong quản lý
    for(uint8_t idx_valve = 0; idx_valve < MAX_WELL_AMIDITE; idx_valve++)
    {
        if( global_var.manual_run.u8_checked_well[idx_valve] == true)
        {
            volume.sub_volume(global_var.manual_run.u8_typeof_chemical, u16_volume_temp);
        }
    }
    // gửi command
    for(uint8_t idx_valve = 0; idx_valve < MAX_WELL_AMIDITE; idx_valve++)
    {
        // index comman bắt đầu từ 20 - 10byte đầu dùng để setting
        Command_send[idx_valve + 49] = global_var.manual_run.u8_checked_well[idx_valve];
#ifdef MAIN_WINDOW_DEBUG
        qDebug()<< "function manual run" << idx_valve << Command_send[idx_valve + 1] << " volume" << global_var.manual_run.u16_volume.Data;
#endif
    }
    u16_time_wait_firmware = 30000;
    if(STM32_COM.serial_send_command_Firmware(serialPort, Command_send, u16_time_wait_firmware))
    {
        syno24_machine.setManualState(State::STOPED);
    }
    else
    {
        QMessageBox::warning(this,"Warning", "System Error, RESTART ALL SYSTEM");
    }

    syno24_machine.printModeAndState();
    syno24_machine.setManualState(State::STOPED);
}


void SYNO24::on_btn_StartManual_CtrlVacuum_released()
{
    // 26-03-2024 sua tinh nang va cau truc cua manual run nhung chua test
    // FIRMWARE VA SOFTWARE CHUA TEST
    syno24_machine.setManualState(State::RUNNING);
    if(syno24_machine.getManualState() == State::STOPED)
    {
        ui->btn_StartManual->setEnabled(true);
        ui->btn_AutoHome_Manual->setEnabled(true);
        ui->btn_StartManual_CtrlVacuum->setEnabled(true);
    }
    else
    {
        ui->btn_StartManual->setEnabled(false);
        ui->btn_AutoHome_Manual->setEnabled(false);
        ui->btn_StartManual_CtrlVacuum->setEnabled(false);
    }
    //syno24_machine.procsess_ui();
    // Start Primming control

    uint16_t u16_volume_temp = ui->spbx_volume_manual->value();
    QByteArray Command_send(LENGTH_COMMAND_SEND,0);
    Command_send[0] = CMD_MANUAL_RUN; // FILL to well
    Command_send[1] = 2; // option manual = pushdown chemical

    global_var.manual_run.u8_option_pressure[0] = ui->cbx_option_pressure_manual->currentIndex();
    global_var.manual_run.u16tb_procs_time[0].Data = ui->spbx_time_process_presure_mamual->value();
    global_var.manual_run.u16tb_waitting_after_time[0].Data = ui->spbox_wait_manual->value();

    global_var.manual_run.u8_option_pressure[1] = ui->cbx_option_pressure_manual_2->currentIndex();
    global_var.manual_run.u16tb_procs_time[1].Data = ui->spbx_time_process_presure_mamual_2->value();
    global_var.manual_run.u16tb_waitting_after_time[1].Data = ui->spbox_wait_manual_2->value();

    global_var.manual_run.u8_option_pressure[2] = ui->cbx_option_pressure_manual_3->currentIndex();
    global_var.manual_run.u16tb_procs_time[2].Data = ui->spbx_time_process_presure_mamual_3->value();
    global_var.manual_run.u16tb_waitting_after_time[2].Data = ui->spbox_wait_manual_3->value();

    global_var.manual_run.u8_option_pressure[3] = ui->cbx_option_pressure_manual_4->currentIndex();
    global_var.manual_run.u16tb_procs_time[3].Data = ui->spbx_time_process_presure_mamual_4->value();
    global_var.manual_run.u16tb_waitting_after_time[3].Data = ui->spbox_wait_manual_4->value();


    Command_send[29] = global_var.manual_run.u8_option_pressure[0];
    Command_send[30] = global_var.manual_run.u16tb_procs_time[0].Byte[0];
    Command_send[31] = global_var.manual_run.u16tb_procs_time[0].Byte[1];
    Command_send[32] = global_var.manual_run.u16tb_waitting_after_time[0].Byte[0];
    Command_send[33] = global_var.manual_run.u16tb_waitting_after_time[0].Byte[1];
    Command_send[34] = global_var.manual_run.u8_option_pressure[1];
    Command_send[35] = global_var.manual_run.u16tb_procs_time[1].Byte[0];
    Command_send[36] = global_var.manual_run.u16tb_procs_time[1].Byte[1];
    Command_send[37] = global_var.manual_run.u16tb_waitting_after_time[1].Byte[0];
    Command_send[38] = global_var.manual_run.u16tb_waitting_after_time[1].Byte[1];
    Command_send[39] = global_var.manual_run.u8_option_pressure[2];
    Command_send[40] = global_var.manual_run.u16tb_procs_time[2].Byte[0];
    Command_send[41] = global_var.manual_run.u16tb_procs_time[2].Byte[1];
    Command_send[42] = global_var.manual_run.u16tb_waitting_after_time[2].Byte[0];
    Command_send[43] = global_var.manual_run.u16tb_waitting_after_time[2].Byte[1];
    Command_send[44] = global_var.manual_run.u8_option_pressure[3];
    Command_send[45] = global_var.manual_run.u16tb_procs_time[3].Byte[0];
    Command_send[46] = global_var.manual_run.u16tb_procs_time[3].Byte[1];
    Command_send[47] = global_var.manual_run.u16tb_waitting_after_time[3].Byte[0];
    Command_send[48] = global_var.manual_run.u16tb_waitting_after_time[3].Byte[1];
    uint32_t u32_time_wait_firmware = 60000;
    u32_time_wait_firmware = u32_time_wait_firmware + global_var.manual_run.u16tb_waitting_after_time[0].Data;
    u32_time_wait_firmware = u32_time_wait_firmware + global_var.manual_run.u16tb_waitting_after_time[1].Data;
    u32_time_wait_firmware = u32_time_wait_firmware + global_var.manual_run.u16tb_waitting_after_time[2].Data;
    u32_time_wait_firmware = u32_time_wait_firmware + global_var.manual_run.u16tb_procs_time[0].Data;
    u32_time_wait_firmware = u32_time_wait_firmware + global_var.manual_run.u16tb_procs_time[1].Data;
    u32_time_wait_firmware = u32_time_wait_firmware + global_var.manual_run.u16tb_procs_time[2].Data;
    if(STM32_COM.serial_send_command_Firmware(serialPort, Command_send, u32_time_wait_firmware))
    {
        //global_var.signal_status_oligo.b_flag_MANUAL_RUN = STOP_MANUAL_PROCESS_SYNTHETIC_OLIGO;
    }
    else
    {
        QMessageBox::warning(this,"Warning", "System Error, RESTART ALL SYSTEM");
    }

    syno24_machine.setManualState(State::STOPED);
    syno24_machine.printModeAndState();
    if(syno24_machine.getManualState() == State::STOPED)
    {
        ui->btn_StartManual->setEnabled(true);
        ui->btn_AutoHome_Manual->setEnabled(true);
        ui->btn_StartManual_CtrlVacuum->setEnabled(true);
    }
    else
    {
        ui->btn_StartManual->setEnabled(false);
        ui->btn_AutoHome_Manual->setEnabled(false);
        ui->btn_StartManual_CtrlVacuum->setEnabled(false);
    }
}

void SYNO24:: Display_Protocol_to_user()
{
    quint8 u8_number_sub = protocol_oligo.u8_number_sub;
    ui->spbox_number_sub->setValue(protocol_oligo.u8_number_sub);
    quint8 u8_number_step_current_sub = 0;
    quint8 u8_number_base_current_sub = 0;
    quint32 TimeEstimate= 0;
    QString log_str = "";
    QString log_mix_function = "";
    QString log_control_pressure = "";
    ui->textEdit_list_task_protocol->clear();
    ui->textEdit_list_task_protocol->append("Protocol setting " + QString::number(u8_number_sub)+ " sub");
    ui->textEdit_list_task_protocol->append("Sequence table :" + QString::number(global_var.signal_status_oligo.u16_max_sequence_amidite_setting) + "Mer");
    CalEstimateTimeProtocol(&TimeEstimate);
    QString timeString = convertSecondsToHHMMSS(TimeEstimate);
    //qDebug() << "TimeEstimate :" << timeString; // Output: "Formatted Time: 01:01:01"
    //ui->textEdit_list_task_protocol->append("Protocol Time Estimate : " + QString::number(TimeEstimate) + "Second");
    ui->textEdit_list_task_protocol->append("Protocol Time Estimate : " + timeString);
    for(uint8_t u8_counter_sub = 0; u8_counter_sub < u8_number_sub; u8_counter_sub++)
    {
        u8_number_base_current_sub = protocol_oligo.sub[u8_counter_sub].u8_number_base_on_sub;
        u8_number_step_current_sub = protocol_oligo.sub[u8_counter_sub].u8_number_step_on_base;
        // qDebug()<<"number_sub: "<< u8_number_sub <<"number_step: "<< u8_number_step_current_sub<<"number base of sub: "<< u8_number_base_current_sub;
        ui->textEdit_list_task_protocol->append("Sub "+ QString::number(u8_counter_sub + 1)+ ":" + " Have " + QString::number(u8_number_base_current_sub) + " Base");
        for(uint8_t u8_counter_step = 0; u8_counter_step < u8_number_step_current_sub; u8_counter_step++)
        {
            quint8 u8_first_chemical_temp = 127;
            u8_first_chemical_temp = get_FirstChemical(protocol_oligo.sub[u8_counter_sub].step[u8_counter_step].fill_chemical.u8_first_type_chemical);

            // qDebug()<< " Step in Number " << u8_first_chemical_temp<< " : "<<STEP_NAME[protocol_oligo.sub[u8_counter_sub].step[u8_counter_step].fill_chemical.u8_first_type_chemical];
            if((u8_first_chemical_temp == COUPLING) || u8_first_chemical_temp == CAPPING) // neu la coupling amidite
            {
                log_str = "         STEP " + QString::number(u8_counter_step + 1) + " "+STEP_NAME[protocol_oligo.sub[u8_counter_sub].step[u8_counter_step].fill_chemical.u8_first_type_chemical];
                ui->textEdit_list_task_protocol->append(log_str);
                log_mix_function = "        ";
                for(quint8 idx_mix_fnc = 0; idx_mix_fnc < 3; idx_mix_fnc++)
                {
                    //quint16 u16tb_Timefill_Volume_first_type.Data = 0; // neu la function coupling thi khong bom lan 1
                    log_mix_function = log_mix_function + " | "+ NAME_MIX_FUNCTION[protocol_oligo.sub[u8_counter_sub].step[u8_counter_step].fill_chemical.mix_funtion.u8_type_chemical[idx_mix_fnc]]+
                            " volume: "+ QString::number(protocol_oligo.sub[u8_counter_sub].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data) + "ul" " | ";
                }
                log_mix_function = log_mix_function + " Wait " + QString::number(protocol_oligo.sub[u8_counter_sub].step[u8_counter_step].fill_chemical.u16tb_wait_after_fill.Data) + " | ";
                ui->textEdit_list_task_protocol->moveCursor(QTextCursor::End);
                ui->textEdit_list_task_protocol->insertPlainText (log_mix_function);
                ui->textEdit_list_task_protocol->moveCursor(QTextCursor::End);
                //ui->textEdit_list_task_protocol->append(log_mix_function);
                log_mix_function.clear();
            }
            else
            {
                log_str = "         STEP " + QString::number(u8_counter_step + 1) + " " + STEP_NAME[protocol_oligo.sub[u8_counter_sub].step[u8_counter_step].fill_chemical.u8_first_type_chemical]+ "| volume: "
                        + QString::number(protocol_oligo.sub[u8_counter_sub].step[u8_counter_step].fill_chemical.u16tb_Volume.Data) + + "ul"
                        + " Wait " + QString::number(protocol_oligo.sub[u8_counter_sub].step[u8_counter_step].fill_chemical.u16tb_wait_after_fill.Data) + " | ";

                // qDebug()<<log_str;
                ui->textEdit_list_task_protocol->append(log_str);
            }

            log_control_pressure= "  ";
            for(uint8_t idx_process = 0; idx_process < 10; idx_process++)
            {
                if(protocol_oligo.sub[u8_counter_sub].step[u8_counter_step].control_pressure.u16tb_procs_time[idx_process].Data != 0 &&
                        protocol_oligo.sub[u8_counter_sub].step[u8_counter_step].control_pressure.u16tb_waitting_after_time[idx_process].Data
                        )
                    log_control_pressure = log_control_pressure + NAME_OPTION_PRESSURE[protocol_oligo.sub[u8_counter_sub].step[u8_counter_step].control_pressure.u8_option_pressure[idx_process]] +  " - "+
                            QString::number(protocol_oligo.sub[u8_counter_sub].step[u8_counter_step].control_pressure.u16tb_procs_time[idx_process].Data) + " - "
                            + QString::number(protocol_oligo.sub[u8_counter_sub].step[u8_counter_step].control_pressure.u16tb_waitting_after_time[idx_process].Data) + " | ";
            }
            ui->textEdit_list_task_protocol->moveCursor(QTextCursor::End);
            ui->textEdit_list_task_protocol->insertPlainText(log_control_pressure);
            ui->textEdit_list_task_protocol->moveCursor(QTextCursor::End);
            log_control_pressure = "";
            log_str = "";
        }
    }
}

void SYNO24::on_btn_save_protocol_released()
{
    fnc.save_protocol_toJson(&protocol_oligo, filemanager.protocol_Path);
    //saveJsonProtocolToFile
    // QJsonObject protocolJson = fnc.createProtocolJson(&protocol_oligo);
    // fnc.saveJsonProtocolToFile(protocolJson, filemanager.protocol_Path);
    calculator_volume_and_process_UI();
}

void SYNO24::send_setting()
{
    QByteArray Command_send(LENGTH_COMMAND_SEND,0);
    Command_send[0] = CMD_RECIVED_SETTING; // send setting
    for(uint8_t index_valve = 0; index_valve < MAX_NUMBER_VALVE; index_valve++)
    {
        volume.valve[index_valve].f_a.Data =  volume.valve[index_valve].a;
        volume.valve[index_valve].f_b.Data =  volume.valve[index_valve].b;
        // debug setting

        qDebug()<< "valve " << index_valve  <<"f_a " <<  volume.valve[index_valve].f_a.Data << "f_b " << volume.valve[index_valve].f_b.Data;
        //qDebug()<< "f_b" << volume.valve[index_valve].f_b.Data;
        Command_send[index_valve*8 +1] = volume.valve[index_valve].f_a.Byte[0];
        Command_send[index_valve*8 +2] = volume.valve[index_valve].f_a.Byte[1];
        Command_send[index_valve*8 +3] = volume.valve[index_valve].f_a.Byte[2];
        Command_send[index_valve*8 +4] = volume.valve[index_valve].f_a.Byte[3];
        //====================================================================
        Command_send[index_valve*8 +5] = volume.valve[index_valve].f_b.Byte[0];
        Command_send[index_valve*8 +6] = volume.valve[index_valve].f_b.Byte[1];
        Command_send[index_valve*8 +7] = volume.valve[index_valve].f_b.Byte[2];
        Command_send[index_valve*8 +8] = volume.valve[index_valve].f_b.Byte[3];
    }

    if(STM32_COM.serial_send_command_Firmware(serialPort, Command_send, 2000))
    {
        log("CMD SEND SETTING = OK");
    }
    else
    {
        log("CMD SEND SETTING = ERROR");
        QMessageBox::critical(this, tr("Error"), "NO DEVICE CONNECT, STARTUP DEVICE ERROR");
        //QMessageBox::critical(this, tr("Error: System not response, PLEASE RESTART SYSTEM! "), serialPort->errorString());
    }
}

bool SYNO24::ASK_VENDOR_ID()
{
    QByteArray Command_send(LENGTH_COMMAND_SEND,0);
    Command_send[0] = CMD_ASK_VENDOR_ID; // send setting
    if(STM32_COM.serial_send_command_Firmware(serialPort, Command_send, 2000))
    {
        log("CMD_ASK_VENDOR_ID = OK");
        return true;
    }
    else
    {
        return false;
    }

}
/*
void SYNO24::on_pushButton_released() // go HOME
{
    QByteArray Command_send(LENGTH_COMMAND_SEND,0);
    Command_send[0] = CMD_RUN2HOME; // RUN STEPPER
    //Command_send[1] = (ui->db_spbox_time_primming->value() *10);
    STM32_COM.serial_send_command_Firmware(serialPort, Command_send, 20000);
}
*/
void SYNO24:: get_sensor_humidity_tempareture()
{
    if((serialPort->isOpen()) && (syno24_machine.getMode() != Mode::MANUAL_RUN))
    {
        QByteArray Command_send(LENGTH_COMMAND_SEND,0);
        Command_send[0] = CMD_FIRMWARE_UPDATE_STATUS_AND_SENSOR; // RUN STEPPER
        if(STM32_COM.serial_send_command_Firmware(serialPort, Command_send, 20000))
        {
            log("sensor humidity tempareture = OK");
        }
    }
}
void SYNO24::wait_humidity()
{
    if( syno24_machine.getAutoState() == State::RUNNING && serialPort->isOpen())
    {
        QByteArray Command_send(LENGTH_COMMAND_SEND,0);
        Command_send[0] = CMD_FIRMWARE_UPDATE_STATUS_AND_SENSOR; // cmd get sensor
        Command_send[1] = CMD_FIRMWARE_UPDATE_STATUS_AND_SENSOR; // cmd control open air NITO
        Command_send[2] = global_var.status_and_sensor.u16tb_humidity_Preset.Byte[0]; // cmd get sensor
        Command_send[3] = global_var.status_and_sensor.u16tb_humidity_Preset.Byte[1]; // cmd control open air NITO
        if(STM32_COM.serial_send_command_Firmware(serialPort, Command_send, 20000))
        {
            log("sensor humidity tempareture = OK");
        }
    }
}

// Hàm tương tự qDebug()
void SYNO24::log(const QString& message)
{
    ui->textEdit_status_update_fw->append(message);
}

void SYNO24:: readExcelSequence(QTableView *tableView, QString path)
{
    QXlsx::Document xlsx(path);
    if (!xlsx.isLoadPackage())
    {
        qDebug() << "Failed to load Excel file. Check if the file exists and is not corrupted.";
        return;
    }
    for (int row = 2; row <= (MAX_WELL_AMIDITE + 1); ++row)
    {
        //        QString cellData = xlsx.read(row, 3).toString(); //POSITION
        //        //rowItems << new QStandardItem(cellData);
        //        cellData = xlsx.read(row, 4).toString(); // NAME
        //        //rowItems << new QStandardItem(cellData);
        //        cellData = xlsx.read(row, 6).toString(); // LENGTH
        //        //rowItems << new QStandardItem(cellData);
        //        cellData = xlsx.read(row, 5).toString();// SEQUENCE
        //        //rowItems << new QStandardItem(cellData);
        //        global_var.amidite_well[row - 2].string_sequence = xlsx.read(row, 5).toString(); // SEQUENCE
        //        global_var.amidite_well[row - 2].string_name = xlsx.read(row, 4).toString(); // NAME
        QVariant position = xlsx.read(row, 3); // POSITION
        QVariant name = xlsx.read(row, 4);    // NAME
        QVariant length = xlsx.read(row, 6);  // LENGTH
        QVariant sequence = xlsx.read(row, 5);// SEQUENCE

        if (position.isValid() && name.isValid() && length.isValid() && sequence.isValid())
        {
            global_var.amidite_well[row - 2].string_sequence = sequence.toString(); // SEQUENCE
            global_var.amidite_well[row - 2].string_name = name.toString();        // NAME
        }
        else
        {
            global_var.amidite_well[row - 2].string_sequence = ""; // SEQUENCE
            global_var.amidite_well[row - 2].string_name = "";        // NAME
            //qDebug() << "Invalid data at row:" << row;
        }
    }
    reload_table_sequence();
}
void SYNO24:: reload_table_sequence()
{
    // Generate data to tableview
    for(int row = 0; row < MAX_WELL_AMIDITE; row++)
    {
        QModelIndex index = model_table_well->index(row, 3,QModelIndex());
        model_table_well->setData(index,global_var.amidite_well[row].string_sequence);
    }
    // UPDATE LENGTH TO TABLE
    for(int row = 0; row < MAX_WELL_AMIDITE; row++)
    {
        QModelIndex index = model_table_well->index(row, 2,QModelIndex());
        model_table_well->setData(index,global_var.amidite_well[row].string_sequence.length());
    }
    // UPDATE LENGTH TO TABLE
    for(int row = 0; row < MAX_WELL_AMIDITE; row++)
    {
        QModelIndex index = model_table_well->index(row, 1,QModelIndex());
        model_table_well->setData(index,global_var.amidite_well[row].string_name);
    }
    for(quint8 col= 0; col < 12; col++)
    {
        for(int row = 0; row < 8; row++)
        {
            QModelIndex index = model_table_well->index(col*8 + row, 0,QModelIndex());
            model_table_well->setData(index,name_well_asign[row] +QString::number(col+1));
        }
    }
}
void SYNO24::on_btn_opeExcelSequence_released()
{
    QString sequenceExcel_path = QFileDialog::getOpenFileName(this, "Open file", filemanager.applicationDirpath,"Excel Files (*.xlsx)");
    if(sequenceExcel_path.isEmpty())
    {
        QMessageBox::warning(this, "Warning", " File Path protocol Empty, Please try choice again!!!");
    }
    else
    {
        readExcelSequence(ui->tableView, sequenceExcel_path);
        calculator_volume_and_process_UI();
    }
}


void SYNO24::on_btn_sequence_released()
{
    filemanager.amidite_sequence_Path = QFileDialog::getOpenFileName(this, "Open file", filemanager.amidite_sequence_Dir,"json(*.json)");
    if(filemanager.amidite_sequence_Path.isEmpty())
    {
        QMessageBox::warning(this, "Warning", " File Path protocol Empty, Please try choice again!!!");
    }
    else
    {
        fnc.read_str_amidite_fromJson(&global_var, filemanager.amidite_sequence_Path); // READING FILE SEQUENCE TO RUN
        fnc.getData2AmiditeProcess(&global_var);
        ui->lineEdit_path_sequence->setText(filemanager.amidite_sequence_Path);
        // 14/03/2023 mở comment này để chạy load giá trị lên ui
        // TÍNH TOÁN HOÁ CHÂT VÀ HIỂN THỊ SEQUENCE LÊN UI
        reload_table_sequence();
        calculator_volume_and_process_UI();
        //        // Generate data to tableview
        //        for(int row = 0; row < MAX_WELL_AMIDITE; row++)
        //        {
        //            QModelIndex index = model_table_well->index(row, 3,QModelIndex());
        //            model_table_well->setData(index,global_var.amidite_well[row].string_sequence);
        //        }
        //        // UPDATE LENGTH TO TABLE
        //        for(int row = 0; row < MAX_WELL_AMIDITE; row++)
        //        {
        //            QModelIndex index = model_table_well->index(row, 2,QModelIndex());
        //            model_table_well->setData(index,global_var.amidite_well[row].string_sequence.length());
        //        }
        //        // UPDATE LENGTH TO TABLE
        //        for(int row = 0; row < MAX_WELL_AMIDITE; row++)
        //        {
        //            QModelIndex index = model_table_well->index(row, 1,QModelIndex());
        //            model_table_well->setData(index,global_var.amidite_well[row].string_name);
        //        }
        //        for(quint8 col= 0; col < 4; col++)
        //        {
        //            for(int row = 0; row < 8; row++)
        //            {
        //                QModelIndex index = model_table_well->index(col*8 + row, 0,QModelIndex());
        //                model_table_well->setData(index,name_well_asign[row] +QString::number(col+1));
        //            }
        //        }
    }
}

// Hàm kiểm tra định dạng của văn bản trong QLineEdit
void SYNO24:: checkLineEditFormat(const QString &text) {
    // Biểu thức chính quy cho định dạng mong muốn
    QRegularExpression regex("^(\\d+,)*(\\d+)$");

    // Kiểm tra nếu văn bản không khớp với định dạng
    if (!regex.match(text).hasMatch()) {
        QMessageBox::warning(nullptr, "Lỗi", "Định dạng không hợp lệ. Xin vui lòng nhập lại theo định dạng");
        // Đặc điểm khác biệt tùy thuộc vào cách bạn muốn xử lý khi người dùng nhập sai định dạng.
    } else {
        // Định dạng hợp lệ, bạn có thể thực hiện các xử lý khác tại đây
    }
}
// Sử dụng hàm này khi người dùng kết thúc quá trình chỉnh sửa
void SYNO24:: onLineEditEditingFinished() {
    // Gọi hàm kiểm tra định dạng
    // checkLineEditFormat(u);
}

void SYNO24::on_lineEdit_special_base_textEdited(const QString &arg1)
{
    // qDebug()<< "on lineEdit special base textEdited";
    // uint16_t special_base[127];
    // Gọi hàm để đọc số từ QLineEdit và lưu vào mảng
    //readSpecialBaseFromLineEdit(ui->lineEdit_special_base, protocol_oligo.speacial_base);
    //qDebug()<< special_base;
}
void SYNO24::on_lineEdit_special_base_textChanged(const QString &arg1)
{

}
void SYNO24::readSpecialBaseFromLineEdit(QLineEdit *lineEdit, uint16_t special_base[MAX_SEQUENCE_OF_WELL]) {
    QString text = lineEdit->text();

    // Loại bỏ khoảng trắng và chia chuỗi thành danh sách các số
    QStringList numbers = text.split(QRegExp("\\s*,\\s*"), QString::SkipEmptyParts);

    // Chuyển đổi các số từ chuỗi sang uint16_t và lưu vào mảng
    int index = 0;
    for (const QString &number : numbers)
    {
        bool ok;
        uint value = number.toUInt(&ok);
        if (ok)
        {
            // Chỉ lưu giá trị nếu nó là số dương
            if (value <= MAX_SEQUENCE_OF_WELL)
            {
                special_base[index++] = static_cast<uint16_t>(value);
            }
        }
    }
    //    // Các phần tử còn lại của mảng được đặt thành giá trị mặc định hoặc 0
    //    for (; index < MAX_SEQUENCE_OF_WELL; ++index)
    //    {
    //        special_base[index] = 0;
    //    }
    printArray(special_base);
}

void SYNO24::printArray(const uint16_t special_base[MAX_SEQUENCE_OF_WELL])
{
    for (int i = 0; i < MAX_SEQUENCE_OF_WELL; ++i)
    {
        if(special_base[i] != 0)
        {
            qDebug() << "special_base[" << i << "] = " << special_base[i];
            qDebug() << "base protocol" <<protocol_oligo.speacial_base[i];
            qDebug() << "base protocol " <<m_trityl.speacial_base[i];
        }
    }
}


// Hàm kiểm tra số có trong mảng hay không
bool SYNO24:: isbaseSpecial(uint16_t number, const uint16_t special_base[MAX_SEQUENCE_OF_WELL])
{
    for (int i = 0; i < MAX_SEQUENCE_OF_WELL; ++i)
    {
        if (special_base[i] == number)
        {
            return true;
        }
    }
    return false;
}
//===========================================================
void SYNO24::onButtonReleased_Add_A()
{
    volume.onButtonReleased_Add_Chemical(A);
}
void SYNO24::onButtonReleased_Add_T()
{
    volume.onButtonReleased_Add_Chemical(T);
}
void SYNO24::onButtonReleased_Add_G()
{
    volume.onButtonReleased_Add_Chemical(G);
}
void SYNO24::onButtonReleased_Add_C()
{
    volume.onButtonReleased_Add_Chemical(C);
}
void SYNO24::onButtonReleased_Add_F1()
{
    volume.onButtonReleased_Add_Chemical(I);
}
void SYNO24::onButtonReleased_Add_F2()
{
    volume.onButtonReleased_Add_Chemical(U);
}
void SYNO24::onButtonReleased_Add_ACT()
{
    volume.onButtonReleased_Add_Chemical(Activator);
}
void SYNO24::onButtonReleased_Add_TCA()
{
    volume.onButtonReleased_Add_Chemical(TCA_in_DCM);
}
void SYNO24::onButtonReleased_Add_WASH()
{
    volume.onButtonReleased_Add_Chemical(WASH_ACN_DCM);
}
void SYNO24::onButtonReleased_Add_OX()
{
    volume.onButtonReleased_Add_Chemical(OXIDATION_IODINE);
}
void SYNO24::onButtonReleased_Add_CAPA()
{
    volume.onButtonReleased_Add_Chemical(CAPPING_CAPA);
}
void SYNO24::onButtonReleased_Add_CAPB()
{
    volume.onButtonReleased_Add_Chemical(CAPPING_CAPB);
}
// ==================================================================
void SYNO24::onButtonReleased_Sub_A()
{
    volume.onButtonReleased_Sub_Chemical(A);
}
void SYNO24::onButtonReleased_Sub_T()
{
    volume.onButtonReleased_Sub_Chemical(T);
}
void SYNO24::onButtonReleased_Sub_G()
{
    volume.onButtonReleased_Sub_Chemical(G);
}

void SYNO24::onButtonReleased_Sub_C()
{
    volume.onButtonReleased_Sub_Chemical(C);
}
void SYNO24::onButtonReleased_Sub_I() // TẠM THỜI ĐANG DÙNG I - hoặc F1
{
    volume.onButtonReleased_Sub_Chemical(I);
}
void SYNO24::onButtonReleased_Sub_U() // TẠM THỜI DÙNG U - hoặc F2
{
    volume.onButtonReleased_Sub_Chemical(U);
}
void SYNO24::onButtonReleased_Sub_ACT()
{
    volume.onButtonReleased_Sub_Chemical(Activator);
}
void SYNO24::onButtonReleased_Sub_TCA()
{
    volume.onButtonReleased_Sub_Chemical(TCA_in_DCM);
}
void SYNO24::onButtonReleased_Sub_WASH()
{
    volume.onButtonReleased_Sub_Chemical(WASH_ACN_DCM);
}
void SYNO24::onButtonReleased_Sub_OX()
{
    volume.onButtonReleased_Sub_Chemical(OXIDATION_IODINE);
}
void SYNO24::onButtonReleased_Sub_CAPA()
{
    volume.onButtonReleased_Sub_Chemical(CAPPING_CAPA);
}
void SYNO24::onButtonReleased_Sub_CAPB()
{
    volume.onButtonReleased_Sub_Chemical(CAPPING_CAPB);
}
//==========================================================================

void SYNO24:: calculator_volume_and_process_UI()
{
    volume.reset_volume_cal();
    quint8 u8_number_step_run =  0;
    quint8 u8_number_base_run =  0;
    quint16 u16_counter_base_finished = 0;
    quint8 u8_lastest_sub = 0;
    quint16 u16_max_base_setting_protocol = 0;
    quint8 u8_first_chemical_temp = 127;
    quint8 u8_number_sub_run = ui->spbox_number_sub->value(); // lấy số sub cần Run
    protocol_oligo.u8_number_sub = u8_number_sub_run;
    for(uint8_t ctn_sub = 0; ctn_sub < protocol_oligo.u8_number_sub; ctn_sub++)
    {
        u16_max_base_setting_protocol = u16_max_base_setting_protocol + protocol_oligo.sub[ctn_sub].u8_number_base_on_sub;
        if(u16_max_base_setting_protocol < global_var.signal_status_oligo.u16_max_sequence_amidite_setting)
        {
            if(protocol_oligo.u8_number_sub == 1) // truong hop chi cai dat 1 sub thi sub cuoi cung chinh là sub 0
            {
                u8_lastest_sub = 0;
            }
            else // trường hợp có nhiều sub phía sau thì cứ lấy sub tiếp theo mà chạy
            {
                if(ctn_sub < (protocol_oligo.u8_number_sub -1))
                {
                    u8_lastest_sub = ctn_sub + 1;
                }
                else
                {
                    u8_lastest_sub = ctn_sub;
                }
            }
        }
    }
    // kiểm tra xem sequence có dài hơi là protocol không
    int int_remain_base = global_var.signal_status_oligo.u16_max_sequence_amidite_setting - u16_max_base_setting_protocol;
    if(int_remain_base > 0) // sequence dài hơn protocol rồi // tự động tăng page cho sub
    {
        protocol_oligo.sub[u8_lastest_sub].u8_number_base_on_sub += int_remain_base;
        //qDebug()<< "sub cuoi cung" << u8_lastest_sub;
        //qDebug()<< "so base cua sub cuoi cung" << protocol_oligo.sub[u8_lastest_sub].u8_number_base_on_sub;
    }
    for(uint8_t u8_counter_sub_run = 0; u8_counter_sub_run < u8_number_sub_run; u8_counter_sub_run++)
    {
        // lấy số base của sub-protocol
        u8_number_base_run = protocol_oligo.sub[u8_counter_sub_run].u8_number_base_on_sub;
        //qDebug()<< "u8_number_base_run" << u8_number_base_run;
        for(quint8 u8_counter_base_on_sub = 0; u8_counter_base_on_sub < u8_number_base_run; u8_counter_base_on_sub++)
        {
            //qDebug()<< "u16_counter_base_finished" << u16_counter_base_finished;
            u8_number_step_run = protocol_oligo.sub[u8_counter_sub_run].u8_number_step_on_base;
            for(quint8 u8_counter_step = 0; u8_counter_step < u8_number_step_run; u8_counter_step++)
            {
                u8_first_chemical_temp = get_FirstChemical(protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.u8_first_type_chemical);

                if((u8_first_chemical_temp == COUPLING) || u8_first_chemical_temp == CAPPING) // neu la coupling amidite
                {
                    for(quint8 idx_mix_fnc = 0; idx_mix_fnc < 3; idx_mix_fnc++)
                    {
                        if(protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u8_type_chemical[idx_mix_fnc] == AMIDITE)
                        {
                            for(quint8 u8_idx_well = 0; u8_idx_well < MAX_WELL_AMIDITE; u8_idx_well++)
                            {
                                // 29/07/2023 lấy về giá trị gốc chưa qua convert firmware tự convert đối với function amidite
                                //global_var.amidite_well[u8_idx_well].u16_timefill_well[idx_mix_fnc].Data  = protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data;
                                volume.add_volume_amidite_cal(global_var.amidite_well[u8_idx_well].u8_sequence[u16_counter_base_finished],
                                                              protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data);
                            }

                        }
                        else // KHONG PHAI COUPLING AMIDITE
                        {

                            //                            u16tb_Timefill_Volume_function_mix[idx_mix_fnc].Data = fnc.valve_calculator_timefill(&global_var, protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u8_type_chemical[idx_mix_fnc],
                            //                                                                                                                 protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data);
                            //qDebug()<< "KHONG PHAI COUPLING AMIDITE";
                            for(quint8 u8_idx_well = 0; u8_idx_well < MAX_WELL_AMIDITE; u8_idx_well++)
                            {
                                if(global_var.amidite_well[u8_idx_well].u8_sequence[u16_counter_base_finished] != CHEMICAL_SUBTANCE_EMPTY)
                                {
                                    volume.add_volume_normal_cal(protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u8_type_chemical[idx_mix_fnc],protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data);
                                }
                            }
                        }
                    } // end 3 mix function
                }
                else // khong phai COUPLING ||  CAPPING
                {
                    //                    u16tb_Timefill_Volume_first_type.Data = fnc.valve_calculator_timefill(&global_var, u8_first_chemical_temp,
                    //                                                                                          protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.u16tb_Volume.Data);
                    for(quint8 u8_idx_well = 0; u8_idx_well < MAX_WELL_AMIDITE; u8_idx_well++)
                    {
                        if(global_var.amidite_well[u8_idx_well].u8_sequence[u16_counter_base_finished] != CHEMICAL_SUBTANCE_EMPTY)
                        {
                            volume.add_volume_normal_cal(u8_first_chemical_temp,protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.u16tb_Volume.Data);
                        }
                    }
                }
                if(u8_first_chemical_temp == COUPLING)
                {
                    switch (protocol_oligo.sub[u8_counter_sub_run].douple_coupling_option)
                    {
                    case DOUBLE_COUPLING_FIRSTBASE:
                    {
                        if(u8_counter_base_on_sub == 0)
                        {
                            for(quint8 idx_mix_fnc = 0; idx_mix_fnc < 3; idx_mix_fnc++)
                            {
                                if(protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u8_type_chemical[idx_mix_fnc] == AMIDITE)
                                {
                                    for(quint8 u8_idx_well = 0; u8_idx_well < MAX_WELL_AMIDITE; u8_idx_well++)
                                    {
                                        // 29/07/2023 lấy về giá trị gốc chưa qua convert firmware tự convert đối với function amidite
                                        //global_var.amidite_well[u8_idx_well].u16_timefill_well[idx_mix_fnc].Data  = protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data;
                                        volume.add_volume_amidite_cal(global_var.amidite_well[u8_idx_well].u8_sequence[u16_counter_base_finished],
                                                                      protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data);
                                    }
                                }
                                else // KHONG PHAI COUPLING AMIDITE
                                {

                                    //                            u16tb_Timefill_Volume_function_mix[idx_mix_fnc].Data = fnc.valve_calculator_timefill(&global_var, protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u8_type_chemical[idx_mix_fnc],
                                    //                                                                                                                 protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data);
                                    qDebug()<< "KHONG PHAI COUPLING AMIDITE";
                                    for(quint8 u8_idx_well = 0; u8_idx_well < MAX_WELL_AMIDITE; u8_idx_well++)
                                    {
                                        if(global_var.amidite_well[u8_idx_well].u8_sequence[u16_counter_base_finished] != CHEMICAL_SUBTANCE_EMPTY)
                                        {
                                            volume.add_volume_normal_cal(protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u8_type_chemical[idx_mix_fnc],
                                                                         protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data);
                                        }
                                    }
                                }
                            } // end 3 mix function
                        }
                        break;
                    }
                    case DOUBLE_COUPLING_FIRST_SECOND_BASE:
                    {
                        // double 2 base
                        if(u8_counter_base_on_sub == 0 || u8_counter_base_on_sub == 1)
                        {
                            for(quint8 idx_mix_fnc = 0; idx_mix_fnc < 3; idx_mix_fnc++)
                            {

                                if(protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u8_type_chemical[idx_mix_fnc] == AMIDITE)
                                {
                                    for(quint8 u8_idx_well = 0; u8_idx_well < MAX_WELL_AMIDITE; u8_idx_well++)
                                    {
                                        // 29/07/2023 lấy về giá trị gốc chưa qua convert firmware tự convert đối với function amidite
                                        //global_var.amidite_well[u8_idx_well].u16_timefill_well[idx_mix_fnc].Data  = protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data;
                                        volume.add_volume_amidite_cal(global_var.amidite_well[u8_idx_well].u8_sequence[u16_counter_base_finished],
                                                                      protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data);
                                    }

                                }
                                else // KHONG PHAI COUPLING AMIDITE
                                {

                                    //                            u16tb_Timefill_Volume_function_mix[idx_mix_fnc].Data = fnc.valve_calculator_timefill(&global_var, protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u8_type_chemical[idx_mix_fnc],
                                    //                                                                                                                 protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data);
                                    qDebug()<< "KHONG PHAI COUPLING AMIDITE";
                                    for(quint8 u8_idx_well = 0; u8_idx_well < MAX_WELL_AMIDITE; u8_idx_well++)
                                    {
                                        if(global_var.amidite_well[u8_idx_well].u8_sequence[u16_counter_base_finished] != CHEMICAL_SUBTANCE_EMPTY)
                                        {
                                            volume.add_volume_normal_cal(protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u8_type_chemical[idx_mix_fnc],
                                                                         protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data);
                                        }
                                    }
                                }
                            } // end 3 mix function
                        }
                        break;
                    }
                    case DOUBLE_COUPLING_ALL_BASE:
                    {
                        for(quint8 idx_mix_fnc = 0; idx_mix_fnc < 3; idx_mix_fnc++)
                        {

                            if(protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u8_type_chemical[idx_mix_fnc] == AMIDITE)
                            {
                                for(quint8 u8_idx_well = 0; u8_idx_well < MAX_WELL_AMIDITE; u8_idx_well++)
                                {
                                    // 29/07/2023 lấy về giá trị gốc chưa qua convert firmware tự convert đối với function amidite
                                    //global_var.amidite_well[u8_idx_well].u16_timefill_well[idx_mix_fnc].Data  = protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data;
                                    volume.add_volume_amidite_cal(global_var.amidite_well[u8_idx_well].u8_sequence[u16_counter_base_finished],
                                                                  protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data);
                                }

                            }
                            else // KHONG PHAI COUPLING AMIDITE
                            {

                                //                            u16tb_Timefill_Volume_function_mix[idx_mix_fnc].Data = fnc.valve_calculator_timefill(&global_var, protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u8_type_chemical[idx_mix_fnc],
                                //                                                                                                                 protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data);
                                for(quint8 u8_idx_well = 0; u8_idx_well < MAX_WELL_AMIDITE; u8_idx_well++)
                                {
                                    if(global_var.amidite_well[u8_idx_well].u8_sequence[u16_counter_base_finished] != CHEMICAL_SUBTANCE_EMPTY)
                                    {
                                        volume.add_volume_normal_cal(protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u8_type_chemical[idx_mix_fnc],protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.mix_funtion.u16tb_Volume[idx_mix_fnc].Data);
                                    }
                                }
                            }
                        } // end 3 mix function
                        break;
                    }
                    }
                }
            }// chay so step
            u16_counter_base_finished++;
        }// chay so base
    }
    volume.cal_remain_volume();
    for(quint8 u8_idx_well = 0; u8_idx_well < 12; u8_idx_well++)
    {
        //qDebug()<< "Chemical need" << NAME_MIX_FUNCTION[u8_idx_well] << volume.valve[u8_idx_well].volume_calculator_need;
        //qDebug()<< "Chemical remain" << NAME_MIX_FUNCTION[u8_idx_well] << volume.valve[u8_idx_well].volume_remain;
    }
    volume.tableView_display_data();
}

void SYNO24::copy_sub_protocol_data(sub_protocol_t &dest, const sub_protocol_t &src)
{
    for (int i = 0; i < MAX_STEP_OF_SUB; ++i) {
        dest.step[i] = src.step[i]; // Deep copy to ensure independent data
    }
    // Copy other members
    dest.u8_number_base_on_sub = src.u8_number_base_on_sub;
    dest.u8_number_step_on_base = src.u8_number_step_on_base;
    dest.douple_coupling_option = src.douple_coupling_option;
}

void SYNO24::on_btn_AutoHome_Manual_released()
{
    QByteArray Command_send(LENGTH_COMMAND_SEND,0);
    Command_send[0] = CMD_RUN2HOME; // RUN STEPPER
    STM32_COM.serial_send_command_Firmware(serialPort, Command_send, 20000);
    syno24_machine.setManualState(State::STOPED);
}


void SYNO24::on_btn_new_sequence_released()
{
    QString saveFileName = QFileDialog::getSaveFileName(this,
                                                        tr("Save Json File"),
                                                        filemanager.amidite_sequence_Dir,
                                                        tr("JSON (*.json)"));
    QFile File_Save(saveFileName);
    if( File_Save.open( QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate ) )
    {
        File_Save.close();
        filemanager.amidite_sequence_Path = saveFileName;
        ui->lineEdit_path_sequence->setText(saveFileName);
    }
    else
    {
        QMessageBox::warning(this, "Warning", " Can't Create or Read this file, Please try again!!!");
    }
}


void SYNO24::on_lineEdit_special_base_trityl_textEdited(const QString &arg1)
{
    qDebug()<< "base_trityl: ";
    //uint16_t special_base[127];
    // Gọi hàm để đọc số từ QLineEdit và lưu vào mảng
    m_trityl.cleardata();
    readSpecialBaseFromLineEdit(ui->lineEdit_special_base_trityl, m_trityl.speacial_base);
    //qDebug()<< m_trityl.speacial_base;
}

void SYNO24:: showSplashScreen()
{
    if (!splash) {
        QPixmap pixmap(":/splash/splash/Gene-Circle_2.gif");
        splash = new QSplashScreen(pixmap);
        QLabel label(splash);
        QMovie mv(":/splash/splash/Gene-Circle_2.gif");
        label.setMovie(&mv);
        mv.start();
    }
    splash->show();
    QApplication::processEvents(); // Ensure the splash screen is shown immediately

    //    QPixmap pixmap(":/splash/splash/Gene-Circle_2.gif");
    //    QSplashScreen splash(pixmap);
    //    QLabel label(&splash);
    //    QMovie mv(":/splash/splash/Gene-Circle_2.gif");
    //    label.setMovie(&mv);
    //    mv.start();
    //    splash.show();
    //delay.delay_ms(5000);
}

void SYNO24::closeSplashScreen()
{
    if (splash) {
        splash->close();
        delete splash;
        splash = nullptr;
    }
    //splash.close();
}


void SYNO24::on_btn_scanQR_sequence_released()
{
    QR_Scanner m_scanner;
    m_scanner.setModal(true);
    m_scanner.exec();
    if(m_scanner.getDirPath().isEmpty())
    {
        QMessageBox::warning(this, "Warning", " File Path protocol Empty, Please try choise again!!!");
    }
    else
    {
        readExcelSequence(ui->tableView, m_scanner.getDirPath());
        calculator_volume_and_process_UI();
        QMessageBox::information(this, "Information", " Import OK");
    }
}



void SYNO24::on_btn_savecurrent_dmt_released()
{
    global_var.DMT_step.bSingnal_DMTOff = ui->checkbx_DMT_off->isChecked();
    global_var.DMT_step.fill_chemical.u8_first_type_chemical = DMT_OFF;
    //global_var.DMT_step.fill_chemical.u16tb_Volume.Data = ui->
    global_var.DMT_step.fill_chemical.mix_funtion.u8_type_chemical[0] = ui->cbx_type_chemical_dmt1->currentIndex(); // debug Giang
    global_var.DMT_step.fill_chemical.mix_funtion.u16tb_Volume[0].Data = ui->spbx_volume_dmt1->value();
    global_var.DMT_step.fill_chemical.mix_funtion.u8_type_chemical[1] = ui->cbx_type_chemical_dmt2->currentIndex(); // debug Giang
    global_var.DMT_step.fill_chemical.mix_funtion.u16tb_Volume[1].Data = ui->spbx_volume_dmt2->value();
    global_var.DMT_step.fill_chemical.mix_funtion.u8_type_chemical[2] = ui->cbx_type_chemical_dmt3->currentIndex(); // debug Giang
    global_var.DMT_step.fill_chemical.mix_funtion.u16tb_Volume[2].Data = ui->spbx_volume_dmt3->value();

    global_var.DMT_step.fill_chemical.u16tb_wait_after_fill.Data = ui->spbox_wait_after_fill_dmt->value();


    global_var.DMT_step.control_pressure.u8_option_pressure[0] = ui->cbx_optionDMT_pressure_state_1->currentIndex();
    global_var.DMT_step.control_pressure.u16tb_procs_time[0].Data   =  ui->spbx_DMTtime_process_state_1->value();
    global_var.DMT_step.control_pressure.u16tb_waitting_after_time[0].Data   =  ui->spbox_waitDMT_state_1->value();

    global_var.DMT_step.control_pressure.u8_option_pressure[1] = ui->cbx_optionDMT_pressure_state_2->currentIndex();
    global_var.DMT_step.control_pressure.u16tb_procs_time[1].Data   =  ui->spbx_DMTtime_process_state_2->value();
    global_var.DMT_step.control_pressure.u16tb_waitting_after_time[1].Data   =  ui->spbox_waitDMT_state_2->value();

    global_var.DMT_step.control_pressure.u8_option_pressure[2] = ui->cbx_optionDMT_pressure_state_3->currentIndex();
    global_var.DMT_step.control_pressure.u16tb_procs_time[2].Data   =  ui->spbx_DMTtime_process_state_3->value();
    global_var.DMT_step.control_pressure.u16tb_waitting_after_time[2].Data   =  ui->spbox_waitDMT_state_3->value();

    global_var.DMT_step.control_pressure.u8_option_pressure[3] = ui->cbx_optionDMT_pressure_state_4->currentIndex();
    global_var.DMT_step.control_pressure.u16tb_procs_time[3].Data   =  ui->spbx_DMTtime_process_state_4->value();
    global_var.DMT_step.control_pressure.u16tb_waitting_after_time[3].Data   =  ui->spbox_waitDMT_state_4->value();

    global_var.DMT_step.control_pressure.u8_option_pressure[4] = ui->cbx_optionDMT_pressure_state_5->currentIndex();
    global_var.DMT_step.control_pressure.u16tb_procs_time[4].Data   =  ui->spbx_DMTtime_process_state_5->value();
    global_var.DMT_step.control_pressure.u16tb_waitting_after_time[4].Data   =  ui->spbox_waitDMT_state_5->value();

    global_var.DMT_step.control_pressure.u8_option_pressure[5] = ui->cbx_optionDMT_pressure_state_6->currentIndex();
    global_var.DMT_step.control_pressure.u16tb_procs_time[5].Data   =  ui->spbx_DMTtime_process_state_6->value();
    global_var.DMT_step.control_pressure.u16tb_waitting_after_time[5].Data   =  ui->spbox_waitDMT_state_6->value();

    global_var.DMT_step.control_pressure.u8_option_pressure[6] = ui->cbx_optionDMT_pressure_state_7->currentIndex();
    global_var.DMT_step.control_pressure.u16tb_procs_time[6].Data   =  ui->spbx_DMTtime_process_state_7->value();
    global_var.DMT_step.control_pressure.u16tb_waitting_after_time[6].Data   =  ui->spbox_waitDMT_state_7->value();

    global_var.DMT_step.control_pressure.u8_option_pressure[7] = ui->cbx_optionDMT_pressure_state_8->currentIndex();
    global_var.DMT_step.control_pressure.u16tb_procs_time[7].Data   =  ui->spbx_DMTtime_process_state_8->value();
    global_var.DMT_step.control_pressure.u16tb_waitting_after_time[7].Data   =  ui->spbox_waitDMT_state_8->value();

    global_var.DMT_step.control_pressure.u8_option_pressure[8] = ui->cbx_optionDMT_pressure_state_9->currentIndex();
    global_var.DMT_step.control_pressure.u16tb_procs_time[8].Data   =  ui->spbx_DMTtime_process_state_9->value();
    global_var.DMT_step.control_pressure.u16tb_waitting_after_time[8].Data   =  ui->spbox_waitDMT_state_9->value();

    global_var.DMT_step.control_pressure.u8_option_pressure[9] = ui->cbx_optionDMT_pressure_state_10->currentIndex();
    global_var.DMT_step.control_pressure.u16tb_procs_time[9].Data   =  ui->spbx_DMTtime_process_state_10->value();
    global_var.DMT_step.control_pressure.u16tb_waitting_after_time[9].Data   =  ui->spbox_waitDMT_state_10->value();
    global_var.DMT_step.strspecialBase = ui->lineEdit_special_base_trityl->text();
    m_DMT.save_DMT(global_var);
}


void SYNO24::on_btn_clear_data_step_dmt_released()
{
    ui->cbx_type_chemical_dmt1->setCurrentIndex(0);
    ui->spbx_volume_dmt1->setValue(0);
    ui->cbx_type_chemical_dmt2->setCurrentIndex(0);
    ui->spbx_volume_dmt2->setValue(0);
    ui->cbx_type_chemical_dmt3->setCurrentIndex(0);
    ui->spbx_volume_dmt3->setValue(0);
    // ====================================================
    ui->cbx_optionDMT_pressure_state_1->setCurrentIndex(0);
    ui->spbx_DMTtime_process_state_1->setValue(0);
    ui->spbox_waitDMT_state_1->setValue(0);
    // ====================================================
    ui->cbx_optionDMT_pressure_state_2->setCurrentIndex(0);
    ui->spbx_DMTtime_process_state_2->setValue(0);
    ui->spbox_waitDMT_state_2->setValue(0);
    // ====================================================
    ui->cbx_optionDMT_pressure_state_3->setCurrentIndex(0);
    ui->spbx_DMTtime_process_state_3->setValue(0);
    ui->spbox_waitDMT_state_3->setValue(0);

    ui->cbx_optionDMT_pressure_state_4->setCurrentIndex(0);
    ui->spbx_DMTtime_process_state_4->setValue(0);
    ui->spbox_waitDMT_state_4->setValue(0);

    ui->cbx_optionDMT_pressure_state_5->setCurrentIndex(0);
    ui->spbx_DMTtime_process_state_5->setValue(0);
    ui->spbox_waitDMT_state_5->setValue(0);

    ui->cbx_optionDMT_pressure_state_6->setCurrentIndex(0);
    ui->spbx_DMTtime_process_state_6->setValue(0);
    ui->spbox_waitDMT_state_6->setValue(0);

    ui->cbx_optionDMT_pressure_state_7->setCurrentIndex(0);
    ui->spbx_DMTtime_process_state_7->setValue(0);
    ui->spbox_waitDMT_state_7->setValue(0);

    ui->cbx_optionDMT_pressure_state_8->setCurrentIndex(0);
    ui->spbx_DMTtime_process_state_8->setValue(0);
    ui->spbox_waitDMT_state_8->setValue(0);

    ui->cbx_optionDMT_pressure_state_9->setCurrentIndex(0);
    ui->spbx_DMTtime_process_state_9->setValue(0);
    ui->spbox_waitDMT_state_9->setValue(0);

    ui->cbx_optionDMT_pressure_state_10->setCurrentIndex(0);
    ui->spbx_DMTtime_process_state_10->setValue(0);
    ui->spbox_waitDMT_state_10->setValue(0);
}

void SYNO24::LoadDMT()
{
    ui->checkbx_DMT_off->setChecked(global_var.DMT_step.bSingnal_DMTOff);
    ui->spbx_volume_dmt1->setValue(global_var.DMT_step.fill_chemical.mix_funtion.u16tb_Volume[0].Data);
    ui->spbx_volume_dmt2->setValue(global_var.DMT_step.fill_chemical.mix_funtion.u16tb_Volume[1].Data);
    ui->spbx_volume_dmt3->setValue(global_var.DMT_step.fill_chemical.mix_funtion.u16tb_Volume[2].Data);
    ui->spbox_wait_after_fill_dmt->setValue(global_var.DMT_step.fill_chemical.u16tb_wait_after_fill.Data);
    ui->cbx_type_chemical_dmt1->setCurrentIndex(global_var.DMT_step.fill_chemical.mix_funtion.u8_type_chemical[0]);
    ui->cbx_type_chemical_dmt2->setCurrentIndex(global_var.DMT_step.fill_chemical.mix_funtion.u8_type_chemical[1]);
    ui->cbx_type_chemical_dmt3->setCurrentIndex(global_var.DMT_step.fill_chemical.mix_funtion.u8_type_chemical[2]);
    ui->cbx_optionDMT_pressure_state_1->setCurrentIndex(global_var.DMT_step.control_pressure.u8_option_pressure[0]);
    ui->spbx_DMTtime_process_state_1->setValue(global_var.DMT_step.control_pressure.u16tb_procs_time[0].Data );
    ui->spbox_waitDMT_state_1->setValue(global_var.DMT_step.control_pressure.u16tb_waitting_after_time[0].Data) ;

    ui->cbx_optionDMT_pressure_state_2->setCurrentIndex(global_var.DMT_step.control_pressure.u8_option_pressure[1]);
    ui->spbx_DMTtime_process_state_2->setValue(global_var.DMT_step.control_pressure.u16tb_procs_time[1].Data );
    ui->spbox_waitDMT_state_2->setValue(global_var.DMT_step.control_pressure.u16tb_waitting_after_time[1].Data );

    ui->cbx_optionDMT_pressure_state_3->setCurrentIndex(global_var.DMT_step.control_pressure.u8_option_pressure[2]);
    ui->spbx_DMTtime_process_state_3->setValue(global_var.DMT_step.control_pressure.u16tb_procs_time[2].Data );
    ui->spbox_waitDMT_state_3->setValue(global_var.DMT_step.control_pressure.u16tb_waitting_after_time[2].Data );

    ui->cbx_optionDMT_pressure_state_4->setCurrentIndex(global_var.DMT_step.control_pressure.u8_option_pressure[3]);
    ui->spbx_DMTtime_process_state_4->setValue(global_var.DMT_step.control_pressure.u16tb_procs_time[3].Data );
    ui->spbox_waitDMT_state_4->setValue(global_var.DMT_step.control_pressure.u16tb_waitting_after_time[3].Data );

    ui->cbx_optionDMT_pressure_state_5->setCurrentIndex(global_var.DMT_step.control_pressure.u8_option_pressure[4]);
    ui->spbx_DMTtime_process_state_5->setValue(global_var.DMT_step.control_pressure.u16tb_procs_time[4].Data );
    ui->spbox_waitDMT_state_5->setValue(global_var.DMT_step.control_pressure.u16tb_waitting_after_time[4].Data );

    ui->cbx_optionDMT_pressure_state_6->setCurrentIndex(global_var.DMT_step.control_pressure.u8_option_pressure[5]);
    ui->spbx_DMTtime_process_state_6->setValue(global_var.DMT_step.control_pressure.u16tb_procs_time[5].Data );
    ui->spbox_waitDMT_state_6->setValue(global_var.DMT_step.control_pressure.u16tb_waitting_after_time[5].Data );

    ui->cbx_optionDMT_pressure_state_7->setCurrentIndex(global_var.DMT_step.control_pressure.u8_option_pressure[6]);
    ui->spbx_DMTtime_process_state_7->setValue(global_var.DMT_step.control_pressure.u16tb_procs_time[6].Data );
    ui->spbox_waitDMT_state_7->setValue(global_var.DMT_step.control_pressure.u16tb_waitting_after_time[6].Data );

    ui->cbx_optionDMT_pressure_state_8->setCurrentIndex(global_var.DMT_step.control_pressure.u8_option_pressure[7]);
    ui->spbx_DMTtime_process_state_8->setValue(global_var.DMT_step.control_pressure.u16tb_procs_time[7].Data );
    ui->spbox_waitDMT_state_8->setValue(global_var.DMT_step.control_pressure.u16tb_waitting_after_time[7].Data );

    ui->cbx_optionDMT_pressure_state_9->setCurrentIndex(global_var.DMT_step.control_pressure.u8_option_pressure[8]);
    ui->spbx_DMTtime_process_state_9->setValue(global_var.DMT_step.control_pressure.u16tb_procs_time[8].Data );
    ui->spbox_waitDMT_state_9->setValue(global_var.DMT_step.control_pressure.u16tb_waitting_after_time[8].Data );

    ui->cbx_optionDMT_pressure_state_10->setCurrentIndex(global_var.DMT_step.control_pressure.u8_option_pressure[9]);
    ui->spbx_DMTtime_process_state_10->setValue(global_var.DMT_step.control_pressure.u16tb_procs_time[9].Data );
    ui->spbox_waitDMT_state_10->setValue(global_var.DMT_step.control_pressure.u16tb_waitting_after_time[9].Data );
}


void SYNO24::CalEstimateTimeProtocol(quint32 *TimeEstimate)
{
    qDebug()<< "CalEstimateTimeProtocol : ";
    quint8 u8_number_step_run =  0;
    quint8 u8_number_base_run =  0;
    quint16 u16_counter_base_finished = 1;
    quint8 u8_lastest_sub = 0;
    quint16 u16_max_base_setting_protocol = 0;
    quint8 u8_first_chemical_temp = 127;
    quint8 u8_number_sub_run = ui->spbox_number_sub->value(); // lấy số sub cần Run
    quint32 u32_time_oligo_process_step;
    quint64 u32_time_oligo_process_base;
    quint64 u32_time_oligo_process_protocol;
    protocol_oligo.u8_number_sub = u8_number_sub_run;
    for(uint8_t ctn_sub = 0; ctn_sub < protocol_oligo.u8_number_sub; ctn_sub++)
    {
        u16_max_base_setting_protocol = u16_max_base_setting_protocol + protocol_oligo.sub[ctn_sub].u8_number_base_on_sub;
        if(u16_max_base_setting_protocol < global_var.signal_status_oligo.u16_max_sequence_amidite_setting)
        {
            if(protocol_oligo.u8_number_sub == 1) // truong hop chi cai dat 1 sub thi sub cuoi cung chinh là sub 0
            {
                u8_lastest_sub = 0;
            }
            else // trường hợp có nhiều sub phía sau thì cứ lấy sub tiếp theo mà chạy
            {
                if(ctn_sub < (protocol_oligo.u8_number_sub -1))
                {
                    u8_lastest_sub = ctn_sub + 1;
                }
                else
                {
                    u8_lastest_sub = ctn_sub;
                }
            }
        }
    }
    // kiểm tra xem sequence có dài hơi là protocol không
    int int_remain_base = global_var.signal_status_oligo.u16_max_sequence_amidite_setting - u16_max_base_setting_protocol;
    if(int_remain_base > 0) // sequence dài hơn protocol rồi // tự động tăng page cho sub
    {
        protocol_oligo.sub[u8_lastest_sub].u8_number_base_on_sub += int_remain_base;
        //qDebug()<< "sub cuoi cung" << u8_lastest_sub;
        //qDebug()<< "so base cua sub cuoi cung" << protocol_oligo.sub[u8_lastest_sub].u8_number_base_on_sub;
    }
    u32_time_oligo_process_protocol = 0;
    for(uint8_t u8_counter_sub_run = 0; u8_counter_sub_run < u8_number_sub_run; u8_counter_sub_run++)
    {
        // lấy số base của sub-protocol
        u8_number_base_run = protocol_oligo.sub[u8_counter_sub_run].u8_number_base_on_sub;
        //qDebug()<< "u8_number_base_run" << u8_number_base_run;
        u32_time_oligo_process_base = 0;
        for(quint8 u8_counter_base_on_sub = 0; u8_counter_base_on_sub < u8_number_base_run; u8_counter_base_on_sub++)
        {
            if(u16_counter_base_finished < global_var.signal_status_oligo.u16_max_sequence_amidite_setting)
            {
                qDebug()<< "u16_counter_base_finished" << u16_counter_base_finished;
                u8_number_step_run = protocol_oligo.sub[u8_counter_sub_run].u8_number_step_on_base;
                u32_time_oligo_process_step = 0;// ofset for running time // cần phải tính toán thật kĩ cho từng base
                for(quint8 idx = 0; idx < MAX_WELL_AMIDITE; idx++)
                {
                    if(global_var.amidite_well[idx].u8_sequence[u8_counter_base_on_sub] != CHEMICAL_SUBTANCE_EMPTY)
                    {
                        u32_time_oligo_process_step = u32_time_oligo_process_step + 1400;//  + thêm thời gian chạy di chuyển đến giếng
                        u32_time_oligo_process_step = u32_time_oligo_process_step + 1000;//  + thêm thời gian bơm hóa chất
                    }
                }
                for(quint8 u8_counter_step = 0; u8_counter_step < u8_number_step_run; u8_counter_step++)
                {
                    u32_time_oligo_process_step += protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.u16tb_wait_after_fill.Data;
                    for(uint8_t idx_process = 0; idx_process < 10; idx_process++)
                    {
                        u32_time_oligo_process_step +=  protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].control_pressure.u16tb_procs_time[idx_process].Data;
                        u32_time_oligo_process_step +=  protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].control_pressure.u16tb_waitting_after_time[idx_process].Data;
                    }
                    if(u8_counter_base_on_sub == 0 && protocol_oligo.sub[u8_counter_sub_run].step[u8_counter_step].fill_chemical.u8_first_type_chemical == COUPLING) // neu co coupling thi base dau tien se them primming + bu thời gian này thêm
                    {
                        u32_time_oligo_process_step = u32_time_oligo_process_step * 2;
                    }
                }
                u32_time_oligo_process_base += (u32_time_oligo_process_step / 1000); // millis to seconds
                u16_counter_base_finished ++;
            }
            qDebug()<< "Base : " << u8_counter_base_on_sub+ 1 << "Sub : " << u8_counter_sub_run + 1<< "Time Estimate : " << u32_time_oligo_process_base;
        }
        u32_time_oligo_process_protocol += u32_time_oligo_process_base;
        qDebug()<<"Sub : " << u8_counter_sub_run + 1 << "Time Estimate" << u32_time_oligo_process_protocol; // seconds
    }
    *TimeEstimate = u32_time_oligo_process_protocol * 130 /100 ;
}

QString SYNO24:: convertSecondsToHHMMSS(quint32 TimeEstimate) {
    // Chuyển đổi giây thành giờ, phút, giây
    int hours = TimeEstimate / 3600;
    int minutes = (TimeEstimate % 3600) / 60;
    int seconds = TimeEstimate % 60;

    // Sử dụng QTime để định dạng hh:mm:ss
    return QString("%1:%2:%3")
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
}



void SYNO24::updateMonitor()
{
    // Kiểm tra xem circleWidget có được khởi tạo chưa
    if (!circleWidget) {
        qDebug() << "circleWidget is not initialized!";
        return;
    }
    for (int cols = 0; cols < 12; cols++) { // 12 CỘT
        for (int rows = 0; rows < 8; rows++) { // 8 HÀNG
            // Kiểm tra trạng thái của từng dot và cập nhật
            if (global_var.amidite_well[cols * 8 + rows].string_sequence.length() == 0) {
                // Nếu chuỗi trống, đánh dấu là Empty
                circleWidget->updateDotStatus(cols, rows, Empty);
                //qDebug() << "Updating dot status: col=" << cols << ", row=" << rows;
            } else {
                // Nếu chuỗi không rỗng, cập nhật trạng thái InProgress (hoặc Completed, Off, tùy theo logic)
                circleWidget->updateDotStatus(cols, rows, InProgress);
                //qDebug() << "Updating dot status: col=" << cols << ", row=" << rows;
            }
        }
    }
    //circleWidget->update();
}
// Hàm để thay đổi màu của một ô trong QTableWidget
void SYNO24::setCellColor(QTableWidget* tableWidget, int row, int col, const QColor& color) {
    // Kiểm tra để đảm bảo rằng row và col hợp lệ và ô không null
    if (row >= 0 && row < tableWidget->rowCount() && col >= 0 && col < tableWidget->columnCount()) {
        QTableWidgetItem* item = tableWidget->item(row, col);
        if (item == nullptr) {
            // Nếu không có ô, tạo mới một ô
            item = new QTableWidgetItem();
            tableWidget->setItem(row, col, item);
        }
        item->setBackground(QBrush(color));
    }
}

void  SYNO24::MonitorPlateUpdateUI(uint16_t baseFinished) {
    // Ví dụ: Thiết lập màu sắc cho một số ô trong bảng
    for (int cols = 0; cols < 12; cols++)
    { // 12 CỘT
        for (int rows = 0; rows < 8; rows++)
        { // 8 HÀNG
            // nếu seqence vị trí này rỗng thì màu xám
            if (global_var.amidite_well[cols * 8 + rows].string_sequence.length() == 0) {
                setCellColor(ui->tableWidget, rows, cols, Qt::gray);
            } else {
                if(global_var.signal_kill.well_index[cols * 8 + rows] == true) // neu bi kill
                {
                    setCellColor(ui->tableWidget, rows, cols, Qt::red);    //
                }
                else
                {
                    // nếu không phải rỗng
                    if((global_var.amidite_well[cols * 8 + rows].string_sequence.length() > 0 && syno24_machine.getAutoState() == State::STOPED) ||
                            (syno24_machine.getAutoState() == State::STOPED && global_var.amidite_well[cols * 8 + rows].string_sequence.length() >= baseFinished ))
                    {
                        setCellColor(ui->tableWidget, rows, cols, Qt::blue);    //
                    }
                    else
                    {
                        if(syno24_machine.getAutoState() == State::RUNNING && global_var.amidite_well[cols * 8 + rows].string_sequence.length() >= baseFinished)
                        {
                            setCellColor(ui->tableWidget, rows, cols,Qt::green );    //
                        }
                    }
                }
            }
        }
    }
}

void SYNO24::on_spbox_num_step_sub_1_valueChanged(int arg1)
{
    static QRadioButton *sub1_step[MAX_STEP_OF_SUB] = {
        ui->sub1_step_1,
        ui->sub1_step_2,
        ui->sub1_step_3,
        ui->sub1_step_4,
        ui->sub1_step_5,
        ui->sub1_step_6,
        ui->sub1_step_7,
        ui->sub1_step_8,
        ui->sub1_step_9,
        ui->sub1_step_10,
        ui->sub1_step_11,
        ui->sub1_step_12
    };
    for(int i = 0 ; i < arg1; i++)
    {
        // ui->sub1_step_12.hitButton();
    }
}

void SYNO24::startCountdown(float interval_time) {
    secondsRemaining = interval_time;
    timerEstimate->start(1000);
}

void SYNO24::updateCountdown() {
    if (secondsRemaining > 0) {
        secondsRemaining--;
        int hours = secondsRemaining / 3600; // Tính số giờ
        int minutes = (secondsRemaining % 3600) / 60; // Tính số phút còn lại
        int seconds = secondsRemaining % 60; // Tính số giây còn lại
        // Định dạng chuỗi hiển thị giờ:phút:giây
        QString displayText = QString("%1:%2:%3")
                .arg(hours, 2, 10, QChar('0'))
                .arg(minutes, 2, 10, QChar('0'))
                .arg(seconds, 2, 10, QChar('0'));
        // Hiển thị chuỗi trên LCD
        ui->lcdNumber->display(displayText);
    } else {
        timerEstimate->stop(); // Dừng bộ đếm khi hết giờ
        // Định dạng chuỗi hiển thị giờ:phút:giây
        QString displayText = QString("%1:%2:%3")
                .arg(0, 2, 10, QChar('0'))
                .arg(0, 2, 10, QChar('0'))
                .arg(0, 2, 10, QChar('0'));
        // Hiển thị chuỗi trên LCD
        ui->lcdNumber->display(displayText);
    }
}

void SYNO24::stopCountdown() {
    timerEstimate->stop();  // Dừng timer

    // Đặt LCD về 00:00:00
    QString displayText = QString("%1:%2:%3")
            .arg(0, 2, 10, QChar('0'))
            .arg(0, 2, 10, QChar('0'))
            .arg(0, 2, 10, QChar('0'));
    ui->lcdNumber->display(displayText);
}

void SYNO24:: writeSettings() {
    QSettings settings("config.ini", QSettings::IniFormat);
    // Lưu cổng COM hiện tại
    settings.beginGroup("SerialPort");
    settings.setValue("LastPort", STM32_COM.currentPort);
    settings.endGroup();

    // Lưu dữ liệu bảng
    settings.beginGroup("TableMultiplier");
    settings.remove(""); // Xóa dữ liệu cũ trước khi lưu mới

    for (size_t i = 0; i < m_tableDataMultiplier.rows.size(); ++i) {
        const auto& row = m_tableDataMultiplier.rows[i];
        settings.beginGroup(QString::number(i));
        settings.setValue("From", row.from);
        settings.setValue("To", row.to);
        settings.setValue("Multiplier", row.multiplier);
        settings.endGroup();
    }

    // Lưu tổng số hàng
    settings.setValue("RowCount", static_cast<int>(m_tableDataMultiplier.rows.size()));
    settings.endGroup();

    qDebug() << "[INFO] Saved LastPort to config.ini:" << STM32_COM.currentPort;
}

void SYNO24:: readSettings() {
    QSettings settings("config.ini", QSettings::IniFormat);

    // Đọc cổng COM cũ
    settings.beginGroup("SerialPort");
    QString oldPort = settings.value("LastPort", "").toString(); // Mặc định là chuỗi rỗng nếu không có giá trị
    settings.endGroup();

    if (!oldPort.isEmpty()) {
        qDebug() << "[INFO] Loaded LastPort from config.ini:" << oldPort;
        // Tự động quét và chọn cổng COM cũ
        scanAndSelectPort(oldPort);
        fnc_openSerialPort();
        STM32_COM.currentPort = oldPort;
    } else {
        qDebug() << "[WARNING] No LastPort found in config.ini";
    }
    // Đọc dữ liệu bảng
    // Đọc dữ liệu bảng
    settings.beginGroup("TableMultiplier");

    // Xóa dữ liệu cũ
    m_tableDataMultiplier.rows.clear();

    // Đọc số hàng
    int rowCount = settings.value("RowCount", 0).toInt();

    // Đọc từng hàng
    for (int i = 0; i < rowCount; ++i) {
        QString groupKey = QString::number(i);

        if (settings.childGroups().contains(groupKey)) {
            settings.beginGroup(groupKey);

            TableRowData row;
            row.from = settings.value("From", 0).toInt();
            row.to = settings.value("To", 0).toInt();
            row.multiplier = settings.value("Multiplier", 0).toInt();

            settings.endGroup();

            m_tableDataMultiplier.rows.push_back(row);

            qDebug() << "[DEBUG] Loaded row" << i
                     << "From:" << row.from
                     << "To:" << row.to
                     << "Multiplier:" << row.multiplier;
        }
    }
    settings.endGroup();
    // Cập nhật lại TableView
    updateTableMultiplierView();
    qDebug() << "[INFO] Loaded" << m_tableDataMultiplier.rows.size() << "rows from config.ini.";
}


void SYNO24::saveDMTtimeProcess(QSpinBox* DMTtime_process[], int count, const QString& iniFileName) {
    QSettings settings(iniFileName, QSettings::IniFormat);

    settings.beginGroup("DMTtimeProcess");
    for (int i = 0; i < count; ++i) {
        if (DMTtime_process[i]) {
            settings.setValue(QString("Process_%1").arg(i + 1), DMTtime_process[i]->value());
        }
    }
    settings.endGroup();
    qDebug() << "DMTtimeProcess values saved to" << iniFileName;
}


void SYNO24::loadDMTtimeProcess(QSpinBox* DMTtime_process[], int count, const QString& iniFileName) {
    QSettings settings(iniFileName, QSettings::IniFormat);

    settings.beginGroup("DMTtimeProcess");
    for (int i = 0; i < count; ++i) {
        if (DMTtime_process[i]) {
            int value = settings.value(QString("Process_%1").arg(i + 1), 0).toInt(); // Mặc định là 0
            DMTtime_process[i]->setValue(value);
        }
    }
    settings.endGroup();
    qDebug() << "DMTtimeProcess values loaded from" << iniFileName;
}

void SYNO24::on_tabWidget_main_currentChanged(int index)
{
#ifdef DEBUG_SOFTWARE

#else
    if(index == SYSTEM_TAB)
    {
        if(syno24_machine.getAutoState() == State::STOPED && syno24_machine.getManualState() == State::STOPED)
        {

            ui->tabWidget_main->setCurrentIndex(SYSTEM_TAB);
        }
        else
        {
            ui->tabWidget_main->setCurrentIndex(RUN_TAB);
        }
    }
    else
    {
        //ui->tabWidget_main->setCurrentIndex();
    }

    if(index == RUN_TAB && STM32_COM.flag_connecttion != true)
    {
        QMessageBox::critical(this, tr("Error"), "PLEASE CONNECT WITH SYNO24X");
        ui->tabWidget_main->setCurrentIndex(0);
    }
#endif
}






void SYNO24::on_btnCopySub_released()
{
    // Hiển thị hộp thoại chọn SubProtocol nguồn và đích
    bool ok;
    QStringList subProtocolNames;
    for(int i = 0; i <  protocol_oligo.u8_number_sub; i++)
    {
        subProtocolNames << ( radioSubsellect[i]->text());
    }

    QString sourceName = QInputDialog::getItem(this, "Chọn SubProtocol nguồn", "SubProtocol nguồn:", subProtocolNames, 0, false, &ok);
    if (!ok || sourceName.isEmpty()) {
        qWarning() << "Không có SubProtocol nguồn được chọn!";
        return;
    }

    QString destinationName = QInputDialog::getItem(this, "Chọn SubProtocol đích", "SubProtocol đích:", subProtocolNames, 0, false, &ok);
    if (!ok || destinationName.isEmpty()) {
        qWarning() << "Không có SubProtocol đích được chọn!";
        return;
    }

    // Lấy chỉ số của SubProtocol nguồn và đích
    int sourceIndex = subProtocolNames.indexOf(sourceName);
    int destinationIndex = subProtocolNames.indexOf(destinationName);
    // Copy
    copy_sub_protocol_data(protocol_oligo.sub[destinationIndex], protocol_oligo.sub[sourceIndex]);
    load_protocol_to_ui(destinationIndex, 0);
}


void SYNO24::on_btnDeleteSub_released()
{
    // Hiển thị hộp thoại chọn SubProtocol cần xóa
    bool ok;
    QStringList subProtocolNames;
    for(int i = 0; i < protocol_oligo.u8_number_sub; i++)
    {
        subProtocolNames << radioSubsellect[i]->text();
    }

    QString subToDelete = QInputDialog::getItem(this, "Select SubProtocol to Delete",
                                                "SubProtocol:", subProtocolNames, 0, false, &ok);
    if (!ok || subToDelete.isEmpty()) {
        qWarning() << "No SubProtocol selected for deletion!";
        return;
    }

    int indexToDelete = subProtocolNames.indexOf(subToDelete);

    // Xác nhận người dùng có chắc chắn muốn xóa không
    QMessageBox::StandardButton confirm = QMessageBox::question(this, "Confirm Deletion",
                                                                QString("Are you sure you want to delete SubProtocol '%1'?").arg(subToDelete),
                                                                QMessageBox::Yes | QMessageBox::No);
    if (confirm != QMessageBox::Yes) {
        return;
    }

    // Xóa và dồn dữ liệu
    for (int i = indexToDelete; i < protocol_oligo.u8_number_sub - 1; ++i)
    {
        copy_sub_protocol_data(protocol_oligo.sub[i], protocol_oligo.sub[i + 1]);
        //radioSubsellect[i]->setText(radioSubsellect[i + 1]->text());
    }

    // Giảm số lượng SubProtocol
    protocol_oligo.u8_number_sub--;

    // Cập nhật giao diện
    Display_Protocol_to_user();
    calculator_volume_and_process_UI();

    qDebug() << "SubProtocol deleted successfully.";
}



void SYNO24::on_btn_addsub_released()
{
    ui->spbox_number_sub->setValue(ui->spbox_number_sub->value() + 1);
    protocol_oligo.u8_number_sub =  ui->spbox_number_sub->value();
}


void SYNO24::on_btn_delsub_released()
{
    ui->spbox_number_sub->setValue(ui->spbox_number_sub->value() - 1);
    protocol_oligo.u8_number_sub =  ui->spbox_number_sub->value();
}



void SYNO24::showTritylMessageBox()
{
    // Tạo và hiển thị QMessageBox
    msgBoxTrityl = new QMessageBox(this);
    msgBoxTrityl->setWindowTitle("TRITYL SELLECTION");
    msgBoxTrityl->setText("Insert collection plate Specify\n Choise Yes will move to position Trityl \n Choise No will Continuos Synthesis ");
    msgBoxTrityl->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBoxTrityl->setDefaultButton(QMessageBox::No);

    // Bắt đầu bộ đếm thời gian
    timermsgTrityl->start(120000);

    // Hiển thị QMessageBox và lấy kết quả (sử dụng QEventLoop)
    msgBoxTrityl->show();
    QEventLoop loop;
    connect(msgBoxTrityl, &QMessageBox::finished, &loop, &QEventLoop::quit);
    loop.exec();

    // Lấy kết quả
    if (msgBoxTrityl->result() == QMessageBox::Yes) {
        tritylSelectionResult = true;
    } else {
        tritylSelectionResult = false; // No hoặc timeout
    }

    // Xử lý kết quả
    if (tritylSelectionResult) {
        qDebug() << "User selected Yes (Trityl)";
        // Hành động cho lựa chọn Yes
    } else {
        qDebug() << "User selected No or Timeout (Trityl)";
        // Hành động cho lựa chọn No/Timeout
    }
    timermsgTrityl->stop(); //Dừng timer
    delete msgBoxTrityl;
    msgBoxTrityl = nullptr;
}

void SYNO24::onTritylTimeout()
{
    qDebug() << "Trityl Timeout occurred!";
    timermsgTrityl->stop(); // Dừng timer

    if (msgBoxTrityl) {
        msgBoxTrityl->reject(); // Đóng QMessageBox (tương đương với chọn No)
    }
}
void SYNO24::stopTritylTimer()
{
    if (timermsgTrityl) {
        timermsgTrityl->stop();
        qDebug() << "Trityl timer stopped externally.";
    }
}

void SYNO24::on_mgsbox_released()
{
    showTritylMessageBox();
    qDebug()<< "tritylSelectionResult" << tritylSelectionResult;
}

void SYNO24::on_btn_tabKillSequenceRun_released()
{
    if(syno24_machine.getAutoState() != State::RUNNING)
    {
        KillSequence m_killsequence;
        m_killsequence.setModal(true);
        m_killsequence.setKillSequence(global_var.signal_kill.well_index);
        // m_killsequence.exec();
        // Hiển thị dialog và kiểm tra kết quả
        if (m_killsequence.exec() == QDialog::Accepted) {
            qDebug() << "KillSequence OK.";
            m_killsequence.getKillSequence(global_var.signal_kill.well_index);
        } else {
            // Người dùng đã nhấn "Cancel" hoặc đóng dialog
            qDebug() << "KillSequence was closed without Tắt không đúng cách.";
        }
        MonitorPlateUpdateUI(0);
    }
    else
    {
        QMessageBox::information(this, "Information", " Please STOP or PAUSE system to Kill sequence");
    }
}


void SYNO24::updateTableMultiplierView()
{
    /*
    m_tableModel->clear(); // Xóa dữ liệu cũ
    m_tableModel->setHorizontalHeaderLabels({"From", "To", "Multiplier"});

    for (const auto& rowData : m_tableDataMultiplier.rows) {
        QList<QStandardItem*> items;
        items.append(new QStandardItem(QString::number(rowData.from)));
        items.append(new QStandardItem(QString::number(rowData.to)));
        items.append(new QStandardItem(QString::number(rowData.multiplier)));
        m_tableModel->appendRow(items);
    }

    // Cập nhật số hàng trong model (nếu cần)
    m_tableModel->setRowCount(m_tableDataMultiplier.rowCount());
    */
    if (!m_tableModel) {
        qDebug() << "[ERROR] Table model is null!";
        return;
    }

    m_tableModel->clear();
    m_tableModel->setHorizontalHeaderLabels({"From", "To", "Multiplier"});

    for (const auto& rowData : m_tableDataMultiplier.rows) {
        QList<QStandardItem*> items;

        // Tạo items với căn giữa
        QStandardItem* fromItem = new QStandardItem(QString::number(rowData.from));
        QStandardItem* toItem = new QStandardItem(QString::number(rowData.to));
        QStandardItem* multiplierItem = new QStandardItem(QString::number(rowData.multiplier));

        fromItem->setTextAlignment(Qt::AlignCenter);
        toItem->setTextAlignment(Qt::AlignCenter);
        multiplierItem->setTextAlignment(Qt::AlignCenter);

        items.append(fromItem);
        items.append(toItem);
        items.append(multiplierItem);

        m_tableModel->appendRow(items);
    }
}

void SYNO24::on_btn_AddStateMultiplier_released()
{
    int fromValue = ui->spbx_formDoneOligo->value();
    int toValue = ui->spbx_ToDoneOligo->value();
    int multiplierValue = ui->spbx_multiplier->value();

    // Thêm dữ liệu vào struct
    m_tableDataMultiplier.rows.push_back({fromValue, toValue, multiplierValue});
    // Sử dụng vòng lặp for truyền thống với index
    for (size_t i = 0; i <  m_tableDataMultiplier.rows.size(); ++i) {
        TableRowData rowData = m_tableDataMultiplier.rows[i];
        qDebug()<< "Index: " << i
                << "From: " << rowData.from
                << ", To: " << rowData.to
                << ", Multiplier: " << rowData.multiplier;
    }
    // Cập nhật lại TableView
    updateTableMultiplierView();

}


void SYNO24::on_btn_DeleteMultiplier_released()
{
    QModelIndexList selectedIndexes = ui->tableView_2->selectionModel()->selectedRows();

    if (selectedIndexes.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select a row to delete.");
        return;
    }

    // Xóa các hàng đã chọn (duyệt từ dưới lên để tránh lỗi index)
    for (int i = selectedIndexes.count() - 1; i >= 0; --i) {
        int rowIndex = selectedIndexes.at(i).row();
        m_tableDataMultiplier.rows.erase(m_tableDataMultiplier.rows.begin() + rowIndex);
    }
    updateTableMultiplierView();
    for (size_t i = 0; i < m_tableDataMultiplier.rows.size(); ++i) {
        TableRowData rowData = m_tableDataMultiplier.rows[i];
        qDebug() << "Index: " << i
                 << "From: " << rowData.from
                 << ", To: " << rowData.to
                 << ", Multiplier: " << rowData.multiplier;
    }
}

// Hàm mới để tìm hệ số nhân dựa trên tỉ lệ phần trăm
int SYNO24::findMultiplierByPercentage(double percentage)
{
    for (const auto& rowData : m_tableDataMultiplier.rows) {
        if (percentage >= rowData.from && percentage <= rowData.to) {
            return rowData.multiplier;
        }
    }
    return 100; // Ví dụ: trả về 100 nếu không tìm thấy 100% tức là về nhân tỉ lệ
}

void SYNO24::on_btn_InsertStep_released()
{
    bool ok; // Biến kiểm tra xem người dùng có nhấn OK trên QInputDialog hay không
    int sourceSubIndex = -1, sourceStepIndex = -1, destSubIndex = -1, insertionIndex = -1;

    // --- 0. Kiểm tra xem có SubProtocol nào tồn tại không ---
    if (protocol_oligo.u8_number_sub == 0) {
        QMessageBox::warning(this, "No Sub-Protocols", "There are no sub-protocols available.");
        return;
    }
    // Giới hạn số SubProtocol hiển thị trong lựa chọn (ví dụ: tối đa 5)
    int maxSubsToShow = qMin((int)protocol_oligo.u8_number_sub, MAX_SUB_OF_PROTOCOL);
    if (maxSubsToShow <=0) return; // Trường hợp dự phòng

    // --- 1. Chọn SubProtocol Nguồn ---
    QStringList subProtocolItems; // Danh sách tên SubProtocol để chọn
    for (int i = 0; i < maxSubsToShow; ++i) {
        subProtocolItems << QString("Sub %1").arg(i + 1);
    }

    QString selectedSourceSubStr = QInputDialog::getItem(this, "Select Source Sub-Protocol", // Tiêu đề cửa sổ
                                                         "Copy step FROM which sub-protocol:",     // Nhãn hướng dẫn
                                                         subProtocolItems,                         // Danh sách lựa chọn
                                                         0, false, &ok);
    if (!ok || selectedSourceSubStr.isEmpty()) return; // Người dùng hủy
    sourceSubIndex = subProtocolItems.indexOf(selectedSourceSubStr); // Lấy chỉ số (0-based)

    // --- 2. Chọn Step Nguồn ---
    sub_protocol_t &sourceSub = protocol_oligo.sub[sourceSubIndex];
    // Kiểm tra xem SubProtocol nguồn có Step nào không
    if (sourceSub.u8_number_step_on_base == 0) {
        QMessageBox::information(this, "Information", QString("Source SubProtocol (Sub %1) has no steps to copy from.")
                                 .arg(sourceSubIndex + 1));
        return;
    }

    // Tạo danh sách các Step trong SubProtocol nguồn
    QStringList sourceStepItems;
    for (int i = 0; i < sourceSub.u8_number_step_on_base; ++i) {
        sourceStepItems << QString("Step %1").arg(i + 1);
    }

    QString selectedSourceStepStr = QInputDialog::getItem(this, "Select Source Step",
                                                          QString("Select Step to copy FROM Sub %1:").arg(sourceSubIndex + 1),
                                                          sourceStepItems, 0, false, &ok);
    if (!ok || selectedSourceStepStr.isEmpty()) return; // Người dùng hủy
    sourceStepIndex = sourceStepItems.indexOf(selectedSourceStepStr); // Lấy chỉ số Step nguồn

    // --- 3. Chọn SubProtocol Đích ---
    // Sử dụng lại danh sách subProtocolItems đã tạo ở bước 1
    QString selectedDestSubStr = QInputDialog::getItem(this, "Select Destination Sub-Protocol",
                                                       "Insert step INTO which sub-protocol:",
                                                       subProtocolItems, 0, false, &ok);
    if (!ok || selectedDestSubStr.isEmpty()) return; // Người dùng hủy
    destSubIndex = subProtocolItems.indexOf(selectedDestSubStr); // Lấy chỉ số SubProtocol đích

    // --- 4. Chọn Vị Trí Chèn trong SubProtocol Đích ---
    sub_protocol_t &destSub = protocol_oligo.sub[destSubIndex];

    // Kiểm tra xem SubProtocol đích có đầy không TRƯỚC KHI hỏi vị trí chèn
    if (destSub.u8_number_step_on_base >= MAX_STEP_OF_SUB) {
        QMessageBox::warning(this, "Error", QString("Cannot insert Step: Destination SubProtocol (Sub %1) is full!")
                             .arg(destSubIndex + 1));
        return;
    }

    // Tạo danh sách các vị trí chèn có thể có
    QStringList insertionPointItems;
    for (int i = 0; i <= destSub.u8_number_step_on_base; ++i) {
        if (i < destSub.u8_number_step_on_base) {
            insertionPointItems << QString("Insert before Step %1").arg(i + 1);
        } else {
            // Vị trí cuối cùng
            insertionPointItems << QString("Insert at the end (after Step %1)").arg(destSub.u8_number_step_on_base);
        }
    }
    // Xử lý trường hợp SubProtocol đích rỗng
    if (destSub.u8_number_step_on_base == 0) {
        insertionPointItems.clear();
        insertionPointItems << QString("Insert at the beginning");
    }

    QString insertionPointStr = QInputDialog::getItem(this, "Select Insertion Point",
                                                      QString("Select where to insert INTO Sub %1:").arg(destSubIndex + 1),
                                                      insertionPointItems, 0, false, &ok);
    if (!ok || insertionPointStr.isEmpty()) return; // Người dùng hủy
    insertionIndex = insertionPointItems.indexOf(insertionPointStr); // Lấy chỉ số vị trí chèn

    // --- 5. Gọi hàm logic cốt lõi để thực hiện công việc ---
    copyAndInsertStep(sourceSubIndex, sourceStepIndex, destSubIndex, insertionIndex);

    // --- 6. Hiển thị thông báo thành công ---
    QMessageBox::information(this, "Success", QString("Successfully copied Step %1 from Sub %2 and inserted at position %3 in Sub %4.")
                             .arg(sourceStepIndex + 1) // +1 để hiển thị cho người dùng (1-based)
                             .arg(sourceSubIndex + 1)
                             .arg(insertionIndex + 1)
                             .arg(destSubIndex + 1));
}

void SYNO24::copyAndInsertStep(int sourceSubIndex, int sourceStepIndex, int destSubIndex, int insertionIndex) {
    // --- Kiểm tra tính hợp lệ của chỉ số SubProtocol nguồn và đích ---
    if (sourceSubIndex < 0 || sourceSubIndex >= protocol_oligo.u8_number_sub ||
            destSubIndex < 0 || destSubIndex >= protocol_oligo.u8_number_sub) {
        qWarning() << "copyAndInsertStep - Invalid source or destination sub-protocol index.";
        QMessageBox::critical(this, "Critical Error", "Invalid source or destination SubProtocol index provided.");
        return;
    }

    // --- Lấy tham chiếu đến SubProtocol nguồn và đích ---
    sub_protocol_t &sourceSub = protocol_oligo.sub[sourceSubIndex];
    sub_protocol_t &destSub = protocol_oligo.sub[destSubIndex];

    // --- Kiểm tra tính hợp lệ của chỉ số Step nguồn ---
    if (sourceStepIndex < 0 || sourceStepIndex >= sourceSub.u8_number_step_on_base) {
        qWarning() << "copyAndInsertStep - Invalid source step index:" << sourceStepIndex;
        // Thông báo lỗi rõ ràng hơn
        QMessageBox::warning(this, "Error", QString("Invalid source Step index (%1) in Sub %2!")
                             .arg(sourceStepIndex + 1).arg(sourceSubIndex + 1));
        return;
    }

    // --- Kiểm tra tính hợp lệ của vị trí chèn trong SubProtocol đích ---
    // Vị trí chèn hợp lệ từ 0 đến số step hiện có (để chèn vào cuối)
    if (insertionIndex < 0 || insertionIndex > destSub.u8_number_step_on_base) {
        qWarning() << "copyAndInsertStep - Invalid insertion index:" << insertionIndex << "for destination sub" << destSubIndex;
        QMessageBox::warning(this, "Error", QString("Invalid insertion position (%1) in Sub %2!")
                             .arg(insertionIndex + 1).arg(destSubIndex + 1));
        return;
    }

    // --- Kiểm tra xem SubProtocol đích đã đầy chưa ---
    if (destSub.u8_number_step_on_base >= MAX_STEP_OF_SUB) {
        qWarning() << "copyAndInsertStep - Destination sub-protocol" << destSubIndex << "is full.";
        QMessageBox::warning(this, "Error", QString("Cannot insert Step: Destination SubProtocol (Sub %1) is full!")
                             .arg(destSubIndex + 1));
        return;
    }

    // --- Thực hiện sao chép và chèn ---
    // 1. Lấy bản sao của dữ liệu Step nguồn
    Step_process_parameter_t sourceStepData = sourceSub.step[sourceStepIndex];

    // 2. Dịch chuyển các Step trong SubProtocol đích để tạo không gian
    // Lặp ngược từ cuối về vị trí chèn
    for (int i = destSub.u8_number_step_on_base - 1; i >= insertionIndex; --i) {
        destSub.step[i + 1] = destSub.step[i]; // Dời step i -> i+1
    }

    // 3. Chèn dữ liệu Step đã sao chép vào vị trí đích
    destSub.step[insertionIndex] = sourceStepData;

    // 4. Tăng số lượng Step của SubProtocol đích
    destSub.u8_number_step_on_base++;

    qDebug() << "Successfully copied Step" << sourceStepIndex << "from Sub" << sourceSubIndex
             << "and inserted at index" << insertionIndex << "in Sub" << destSubIndex;

    // --- Cập nhật Giao diện Người dùng ---
    // Load lại giao diện cho SubProtocol ĐÍCH, và chọn Step vừa được chèn.
    load_protocol_to_ui(destSubIndex, insertionIndex);
    Display_Protocol_to_user(); // Cập nhật hiển thị chung (hoặc chỉ cập nhật cho destSubIndex?)
    calculator_volume_and_process_UI(); // Tính toán lại dựa trên thay đổi ở SubProtocol đích
}
void SYNO24::deleteStep(int subIndex, int stepIndex) {
    // --- 1. Kiểm tra tính hợp lệ của chỉ số SubProtocol ---
    if (subIndex < 0 || subIndex >= protocol_oligo.u8_number_sub) {
        qWarning() << "deleteStep - Invalid sub-protocol index:" << subIndex;
        QMessageBox::critical(this, "Lỗi nghiêm trọng", QString("Chỉ số SubProtocol không hợp lệ (%1).").arg(subIndex + 1));
        return;
    }

    // --- Lấy tham chiếu đến SubProtocol mục tiêu ---
    sub_protocol_t &targetSub = protocol_oligo.sub[subIndex];

    // --- 2. Kiểm tra tính hợp lệ của chỉ số Step cần xóa ---
    // Chỉ số step phải nằm trong phạm vi các step đang được sử dụng
    if (stepIndex < 0 || stepIndex >= targetSub.u8_number_step_on_base) {
        qWarning() << "deleteStep - Invalid step index:" << stepIndex << "for sub" << subIndex;
        if (targetSub.u8_number_step_on_base == 0) {
            QMessageBox::warning(this, "Lỗi", QString("Không thể xóa Step: SubProtocol (Sub %1) hiện đang trống!").arg(subIndex + 1));
        } else {
            QMessageBox::warning(this, "Lỗi", QString("Chỉ số Step không hợp lệ (%1) trong Sub %2! Chỉ số hợp lệ từ 1 đến %3.")
                                 .arg(stepIndex + 1).arg(subIndex + 1).arg(targetSub.u8_number_step_on_base));
        }
        return;
    }

    // --- 3. Thực hiện xóa và dịch chuyển ---
    qDebug() << "Attempting to delete Step" << stepIndex << "from Sub" << subIndex;

    // Dịch chuyển các Step phía sau lên một vị trí
    // Vòng lặp bắt đầu từ Step ngay sau Step bị xóa (stepIndex + 1)
    // và kết thúc ở Step cuối cùng hiện có (u8_number_step_on_base - 1)
    for (int i = stepIndex + 1; i < targetSub.u8_number_step_on_base; ++i) {
        targetSub.step[i - 1] = targetSub.step[i]; // Di chuyển step[i] về vị trí step[i-1]
    }

    // --- 4. Giảm số lượng Step của SubProtocol ---
    // Sau khi dịch chuyển, giảm bộ đếm số step đi 1
    targetSub.u8_number_step_on_base--;

    // (Tùy chọn) Xóa dữ liệu của step cuối cùng cũ (giờ không còn dùng đến)
    // Mặc dù không bắt buộc vì u8_number_step_on_base đã giảm, nhưng có thể giúp tránh nhầm lẫn khi debug
    if (targetSub.u8_number_step_on_base >= 0 && targetSub.u8_number_step_on_base < MAX_STEP_OF_SUB) {
        // Tạo một step rỗng hoặc mặc định để gán
        // Step_process_parameter_t emptyStep = {}; // Hoặc khởi tạo theo cách phù hợp
        // targetSub.step[targetSub.u8_number_step_on_base] = emptyStep;
        // Hoặc đơn giản là không cần làm gì nếu cấu trúc Step không chứa con trỏ hay tài nguyên cần giải phóng đặc biệt
    }
    qDebug() << "Successfully deleted Step" << stepIndex << "from Sub" << subIndex
             << ". New step count:" << targetSub.u8_number_step_on_base;

    // --- 5. Cập nhật Giao diện Người dùng ---
    // Load lại giao diện cho SubProtocol vừa bị thay đổi.
    // Chọn step ở vị trí vừa xóa (nếu còn step) hoặc step cuối cùng mới.
    int newSelectedIndex = -1;
    if (targetSub.u8_number_step_on_base > 0) {
        // Chọn step tại index cũ nếu nó vẫn hợp lệ, nếu không chọn step cuối cùng mới
        newSelectedIndex = qMin(stepIndex, targetSub.u8_number_step_on_base - 1);
    }
    load_protocol_to_ui(subIndex, newSelectedIndex); // Load lại UI cho sub này, chọn step mới (hoặc không chọn nếu sub rỗng)

    Display_Protocol_to_user();           // Cập nhật hiển thị tổng thể (nếu cần)
    calculator_volume_and_process_UI(); // Tính toán lại các thông số dựa trên thay đổi
}
void SYNO24::on_btnDeleteStep_released()
{
    bool ok; // Biến kiểm tra xem người dùng có nhấn OK trên QInputDialog hay không
    int subIndexToDelete = -1;
    int stepIndexToDelete = -1;

    // --- 0. Kiểm tra xem có SubProtocol nào tồn tại không ---
    if (protocol_oligo.u8_number_sub == 0) {
        QMessageBox::warning(this, "Không có SubProtocol", "Hiện không có SubProtocol nào.");
        return;
    }

    // --- 1. Chọn SubProtocol để xóa Step ---
    QStringList subProtocolItems; // Danh sách tên SubProtocol để chọn
    // Lấy tất cả các SubProtocol hiện có
    for (int i = 0; i < protocol_oligo.u8_number_sub; ++i) {
        subProtocolItems << QString("Sub %1").arg(i + 1); // Hiển thị 1-based
    }

    QString selectedSubStr = QInputDialog::getItem(this, "Chọn SubProtocol", // Tiêu đề cửa sổ
                                                   "Xóa Step từ SubProtocol nào:", // Nhãn hướng dẫn
                                                   subProtocolItems,            // Danh sách lựa chọn
                                                   0,                           // Chỉ số chọn mặc định
                                                   false,                       // Có cho phép sửa text không? (false)
                                                   &ok);                        // Con trỏ tới biến bool kiểm tra OK/Cancel

    if (!ok || selectedSubStr.isEmpty()) {
        qDebug() << "Người dùng đã hủy chọn SubProtocol.";
        return; // Người dùng nhấn Cancel hoặc không chọn gì
    }
    subIndexToDelete = subProtocolItems.indexOf(selectedSubStr); // Lấy chỉ số 0-based

    // --- 2. Chọn Step cần xóa từ SubProtocol đã chọn ---
    // Kiểm tra xem chỉ số Sub vừa lấy có hợp lệ không (dự phòng)
    if (subIndexToDelete < 0 || subIndexToDelete >= protocol_oligo.u8_number_sub) {
        qWarning() << "Lỗi logic: Chỉ số Sub không hợp lệ sau khi chọn:" << subIndexToDelete;
        return;
    }

    sub_protocol_t &selectedSub = protocol_oligo.sub[subIndexToDelete];

    // Kiểm tra xem SubProtocol đã chọn có Step nào không
    if (selectedSub.u8_number_step_on_base == 0) {
        QMessageBox::information(this, "Thông tin", QString("SubProtocol được chọn (Sub %1) không có Step nào để xóa.")
                                 .arg(subIndexToDelete + 1));
        return;
    }

    // Tạo danh sách các Step trong SubProtocol đã chọn
    QStringList stepItems;
    for (int i = 0; i < selectedSub.u8_number_step_on_base; ++i) {
        stepItems << QString("Step %1").arg(i + 1); // Hiển thị 1-based
    }

    QString selectedStepStr = QInputDialog::getItem(this, "Chọn Step để Xóa",
                                                    QString("Chọn Step cần xóa từ Sub %1:").arg(subIndexToDelete + 1),
                                                    stepItems, 0, false, &ok);

    if (!ok || selectedStepStr.isEmpty()) {
        qDebug() << "Người dùng đã hủy chọn Step.";
        return; // Người dùng nhấn Cancel
    }
    stepIndexToDelete = stepItems.indexOf(selectedStepStr); // Lấy chỉ số Step 0-based

    // Kiểm tra xem chỉ số Step vừa lấy có hợp lệ không (dự phòng)
    if (stepIndexToDelete < 0 || stepIndexToDelete >= selectedSub.u8_number_step_on_base) {
        qWarning() << "Lỗi logic: Chỉ số Step không hợp lệ sau khi chọn:" << stepIndexToDelete;
        return;
    }

    // --- 3. Gọi hàm logic cốt lõi để thực hiện xóa ---
    qDebug() << "Yêu cầu xóa - Sub Index:" << subIndexToDelete << ", Step Index:" << stepIndexToDelete;
    deleteStep(subIndexToDelete, stepIndexToDelete);
    // Hàm deleteStep đã tự cập nhật UI và tính toán lại
    // Bạn có thể thêm một thông báo thành công ở đây nếu muốn
    // --- (Tùy chọn) 4. Hiển thị thông báo thành công ---
    QMessageBox::information(this, "Thành công", QString("Đã xóa thành công Step %1 khỏi Sub %2.")
                             .arg(stepIndexToDelete + 1) // +1 để hiển thị cho người dùng (1-based)
                             .arg(subIndexToDelete + 1));
}


void SYNO24::ManualControlSystem()
{
    //  FAN_SV = 0, // 33 xa khi nito giam do am
    //	LED_RED_SV  = 1, // 34
    //	LED_GREEN_SV  = 2, // 35
    //	V38_EMPTY  = 3, // 36  high push
    //	LOW_PUSH_SV  = 4,// V37
    //	HIGH_PUSH_SV = 5,
    //	V39_EMPTY = 6,
    //	OPEN_NITOR_SV  = 7,
    //qDebug()<< "Control MANUAL System ";
    QByteArray Command_send(LENGTH_COMMAND_SEND,0);
    Command_send[0] = CMD_CONTROL_SYNCHRONIZE_IO; // RUN STEPPER
    //    Command_send[1] = ui->checkbx_control_V_1->isChecked() ? 1 : 0;
    //    Command_send[1] = ui->checkbx_control_V_1->isChecked()? 1 : 0;
    //    Command_send[2] = ui->checkbx_control_V_2->isChecked()? 1 : 0;
    //    Command_send[3] = ui->checkbx_control_V_3->isChecked()? 1 : 0;
    //    Command_send[4] = ui->checkbx_control_V_4->isChecked()? 1 : 0;
    //    Command_send[5] = ui->checkbx_control_V_5->isChecked()? 1 : 0;
    //    Command_send[6] = ui->checkbx_control_V_6->isChecked()? 1 : 0;
    //    Command_send[7] = ui->checkbx_control_V_7->isChecked()? 1 : 0;
    //    Command_send[8] = ui->checkbx_control_V_8->isChecked()? 1 : 0;
    //    Command_send[9] = ui->checkbx_control_V_9->isChecked()? 1 : 0;
    //    Command_send[10] = ui->checkbx_control_V_10->isChecked()? 1 : 0;
    //    Command_send[11] = ui->checkbx_control_V_11->isChecked()? 1 : 0;
    //    Command_send[12] = ui->checkbx_control_V_12->isChecked()? 1 : 0;
    //    Command_send[13] = ui->checkbx_control_V_13->isChecked()? 1 : 0;
    //    Command_send[14] = ui->checkbx_control_V_14->isChecked()? 1 : 0;
    //    Command_send[15] = ui->checkbx_control_V_15->isChecked()? 1 : 0;
    //    Command_send[16] = ui->checkbx_control_V_16->isChecked()? 1 : 0;
    //    Command_send[17] = ui->checkbx_control_V_17->isChecked()? 1 : 0;
    uint8_t idx = 31;
    Command_send[idx] = ui->btnFAN->isChecked()? 1 : 0; // 0
    idx++;
    //Command_send[19] =  // RED LED // 1
    idx++;
    //Command_send[20] //3 // GREEN LED //2
    idx++;
    Command_send[idx] = ui->btn_FanVacuumBox->isChecked()? 1 : 0;//FAN IN BOX 3
    idx++;
    Command_send[idx] = ui->btnLowPushSV->isChecked();//5
    idx++;
    Command_send[idx] = ui->btn_HighPushSV->isChecked();//6
    idx++;
    Command_send[idx] =  ui->btnMediumPushSV->isChecked();//6// empty//7
    idx++;
    Command_send[idx] =  ui->btnOpenAirNitor->isChecked();//8
    idx++;
    for(int i = 31; i < 37; i++)
    {
        qDebug()<< "index" << i<< static_cast<int>(Command_send[i]);
    }
    STM32_COM.serial_send_command_Firmware(serialPort, Command_send, 1000);
}


void SYNO24:: GetFeatureVacuumBox()
{
    global_var.advanced_setting.VacuumBox.Enablefeature = ui->chkbxEnaVaccuumBox->isChecked()? 1 : 0;
    global_var.advanced_setting.VacuumBox.En_WASH =  ui->chkbxVacuumBoxWashing->isChecked()? 1 : 0;
    global_var.advanced_setting.VacuumBox.En_Deblock =  ui->chkbxVacuumBoxDeblock->isChecked()? 1 : 0;
    global_var.advanced_setting.VacuumBox.En_Cap =  ui->chkbxVacuumBoxCapping->isChecked()? 1 : 0;
    global_var.advanced_setting.VacuumBox.En_Coupling =  ui->chkbxVacuumBoxCoupling->isChecked()? 1 : 0;
    global_var.advanced_setting.VacuumBox.En_Ox =  ui->chkbxVacuumBoxOx->isChecked()? 1 : 0;
    global_var.advanced_setting.VacuumBox.time.Data = ui->spbx_time_FanVacuumBox->value();
    qDebug()<< "GetFeatureVacuumBox Enablefeature : " << global_var.advanced_setting.VacuumBox.Enablefeature;
    readSpecialBaseFromLineEdit(ui->lineEdit_special_base_VacuumBox, m_baseVacuumBox.speacial_base);
}

void SYNO24::on_lineEdit_special_base_VacuumBox_textEdited(const QString &arg1)
{
    qDebug()<< "Vacuumbox edited ";
    //uint16_t special_base[127];
    // Gọi hàm để đọc số từ QLineEdit và lưu vào mảng
    m_baseVacuumBox.cleardata();
    readSpecialBaseFromLineEdit(ui->lineEdit_special_base_VacuumBox, m_baseVacuumBox.speacial_base);
}


void SYNO24::on_btnSaveReagentFillCoupling_released()
{
    global_var.advanced_setting.FillChemistryDone.EnableFillWellDone = ui->chkbxEnableReagentFillDone;
    global_var.advanced_setting.FillChemistryDone.En_WASH = ui->chkbxDeblock->isChecked();
    global_var.advanced_setting.FillChemistryDone.En_Deblock = ui->chkbxDeBlock->isChecked();
    global_var.advanced_setting.FillChemistryDone.En_Coupling = ui->chkbxCoupling->isChecked();
    global_var.advanced_setting.FillChemistryDone.En_Cap = ui->chkbxCapping->isChecked();
    global_var.advanced_setting.FillChemistryDone.En_Ox = ui->chkbxVacuumBoxOx->isChecked();
    global_var.advanced_setting.FillChemistryDone.typeReagent =   ui->cbx_type_reagentDelivery->currentIndex();
    global_var.advanced_setting.FillChemistryDone.volumeWASH.Data = ui->spbx_volume_deliveryWash->value();
    global_var.advanced_setting.FillChemistryDone.volumeDeblock.Data = ui->spbx_volume_deliveryDeblock->value();
    global_var.advanced_setting.FillChemistryDone.volumeCoupling.Data = ui->spbx_volume_deliveryOxidation->value();
    global_var.advanced_setting.FillChemistryDone.volumeCap.Data = ui->spbx_volume_deliveryCapping->value();
    global_var.advanced_setting.FillChemistryDone.volumeOx.Data = ui->spbx_volume_deliveryOxidation->value();
}


void SYNO24::UpdateUISTTRun( uint8_t state)
{
    //ui->btnFuncRun->setIcon(QIcon(":/images/images/number-1 (1).png"));
    //:/image/images/number-0.png
    QString iconName = QString(":/image/images/number-%1.png").arg(state);
    ui->btnFuncRun->setIcon(QIcon(iconName));
    ui->lbl_stt_sub_step->setText("RUN ON SUB : " + QString::number(global_var.updateSTTRun2UI.currentSub + 1)
                                  + "| STEP : " + QString::number(global_var.updateSTTRun2UI.currentStep + 1));
    if(state == 0)
    {
        ui->lbl_stt_fw_fb->setText("Delivery Reagent & Wait");
    }
}
void SYNO24::onCountdownFinished()
{
    ui->prgBar_Push->setValue(0);
    ui->prgBar_Wait->setValue(0);
}

void SYNO24::on_chkbox_Allbase_toggled(bool checked)
{
    QString Basestr = "";
    if(checked)
    {
        Basestr = fnc.generateNumberString(global_var.signal_status_oligo.u16_max_sequence_amidite_setting);
        ui->lineEdit_special_base_VacuumBox->setText(Basestr);
    }
    else
    {
        ui->lineEdit_special_base_VacuumBox->setText(Basestr);
    }
}


void SYNO24::loadValveDataToTable() {
    // Giả sử bạn có QTableWidget tên là "tableWidget" trong file .ui
    QTableWidget* table = ui->tableWidget_valveinfo;

    // Số lượng valve
    const int valveCount = MAX_NUMBER_VALVE;

    // Thiết lập bảng
    table->setRowCount(valveCount);
    table->setColumnCount(5);

    // Đặt tiêu đề cột
    table->setHorizontalHeaderLabels({
                                         "Name of Valve",
                                         "t1",
                                         "vol1",
                                         "t2",
                                         "vol2"
                                     });

    QStringList valveNames = {
        "A1", "A2", "A3", "A4",
        "T1", "T2", "T3", "T4",
        "G1", "G2", "G3", "G4",
        "C1", "C2", "C3", "C4",
        "U", "I",
        "ACTI1", "ACTI2", "ACTI3", "ACTI4",
        "TCA1", "TCA2", "TCA3", "TCA4",
        "WASH1", "WASH2", "WASH3", "WASH4",
        "OX1", "OX2", "OX3", "OX4",
        "CAPB1", "CAPB2", "CAPB3", "CAPB4",
        "CAPA1", "CAPA2", "CAPA3", "CAPA4"
    };
    // Duyệt qua từng valve để điền dữ liệu
    for (int i = 0; i < valveCount; ++i) {
        // Cột 0: Số hiệu valve
        QTableWidgetItem *itemNumber = new QTableWidgetItem(QString::number(i));
        itemNumber->setTextAlignment(Qt::AlignCenter);

        // Cột 1: Tên valve
        QTableWidgetItem *itemName = new QTableWidgetItem(valveNames[i]);
        itemName->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        // Cột 2: t1
        QTableWidgetItem *itemT1 = new QTableWidgetItem(QString::number(volume.valve[i].t1));

        // Cột 3: vol1
        QTableWidgetItem *itemVol1 = new QTableWidgetItem(QString::number(volume.valve[i].vol1));

        // Cột 4: t2
        QTableWidgetItem *itemT2 = new QTableWidgetItem(QString::number(volume.valve[i].t2));

        // Cột 5: vol2
        QTableWidgetItem *itemVol2 = new QTableWidgetItem(QString::number(volume.valve[i].vol2));

        // Thêm vào bảng
        table->setItem(i, 0, itemName);
        table->setItem(i, 1, itemT1);
        table->setItem(i, 2, itemVol1);
        table->setItem(i, 3, itemT2);
        table->setItem(i, 4, itemVol2);
    }

    // Tự động điều chỉnh độ rộng cột theo nội dung (tuỳ chọn)
    //table->resizeColumnsToContents();
}

void SYNO24::on_cbx_valve_selected_currentIndexChanged(int index)
{
    ui->db_Spbox_calib_time_1->setValue(volume.valve[index].t1);
    ui->spbox_calib_V1->setValue( volume.valve[index].vol1);
    ui->db_Spbox_calib_time_2->setValue(volume.valve[index].t2);
    ui->spbox_calib_V2->setValue( volume.valve[index].vol2);
    loadValveDataToTable();
}


void SYNO24:: saveTableDataToVolume() {
    QTableWidget* table = ui->tableWidget_valveinfo;
    const int valveCount = MAX_NUMBER_VALVE; // Ví dụ: 40

    for (int i = 0; i < valveCount; ++i) {
        // Truy cập các ô tương ứng trong hàng i, các cột 1 -> 4
        QTableWidgetItem *itemT1 = table->item(i, 1);   // cột 1: t1 (float)
        QTableWidgetItem *itemVol1 = table->item(i, 2); // cột 2: vol1 (int32_t)
        QTableWidgetItem *itemT2 = table->item(i, 3);   // cột 3: t2 (float)
        QTableWidgetItem *itemVol2 = table->item(i, 4); // cột 4: vol2 (int32_t)

        if (itemT1 && itemVol1 && itemT2 && itemVol2) {
            bool ok;

            // Chuyển đổi giá trị và lưu vào cấu trúc dữ liệu
            volume.valve[i].t1 = itemT1->text().toFloat(&ok);
            if (!ok) volume.valve[i].t1 = 0.0f;

            volume.valve[i].vol1 = itemVol1->text().toInt(&ok);
            if (!ok) volume.valve[i].vol1 = 0;

            volume.valve[i].t2 = itemT2->text().toFloat(&ok);
            if (!ok) volume.valve[i].t2 = 0.0f;

            volume.valve[i].vol2 = itemVol2->text().toInt(&ok);
            if (!ok) volume.valve[i].vol2 = 0;
        }
    }

    qDebug() << "Đã lưu dữ liệu từ bảng vào mảng volume.valve[]";
}

void SYNO24::on_btn_saveCalibTable_released()
{
    saveTableDataToVolume();
}


void SYNO24::on_btn_FanVacuumBox_released()
{

}

