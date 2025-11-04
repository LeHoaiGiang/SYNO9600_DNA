#include "killsequence.h"
#include "ui_killsequence.h"
#include "qdebug.h"
KillSequence::KillSequence(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::KillSequence)
{

    ui->setupUi(this);
    this->setWindowTitle("Kill Sequence");
    chkboxWellKill[0] = ui->well_1;
    chkboxWellKill[1] = ui->well_2;
    chkboxWellKill[2] = ui->well_3;
    chkboxWellKill[3] = ui->well_4;
    chkboxWellKill[4] = ui->well_5;
    chkboxWellKill[5] = ui->well_6;
    chkboxWellKill[6] = ui->well_7;
    chkboxWellKill[7] = ui->well_8;
    chkboxWellKill[8] = ui->well_9;
    chkboxWellKill[9] = ui->well_10;
    chkboxWellKill[10] = ui->well_11;
    chkboxWellKill[11] = ui->well_12;
    chkboxWellKill[12] = ui->well_13;
    chkboxWellKill[13] = ui->well_14;
    chkboxWellKill[14] = ui->well_15;
    chkboxWellKill[15] = ui->well_16;
    chkboxWellKill[16] = ui->well_17;
    chkboxWellKill[17] = ui->well_18;
    chkboxWellKill[18] = ui->well_19;
    chkboxWellKill[19] = ui->well_20;
    chkboxWellKill[20] = ui->well_21;
    chkboxWellKill[21] = ui->well_22;
    chkboxWellKill[22] = ui->well_23;
    chkboxWellKill[23] = ui->well_24;
    chkboxWellKill[24] = ui->well_25;
    chkboxWellKill[25] = ui->well_26;
    chkboxWellKill[26] = ui->well_27;
    chkboxWellKill[27] = ui->well_28;
    chkboxWellKill[28] = ui->well_29;
    chkboxWellKill[29] = ui->well_30;
    chkboxWellKill[30] = ui->well_31;
    chkboxWellKill[31] = ui->well_32;
    chkboxWellKill[32] = ui->well_33;
    chkboxWellKill[33] = ui->well_34;
    chkboxWellKill[34] = ui->well_35;
    chkboxWellKill[35] = ui->well_36;
    chkboxWellKill[36] = ui->well_37;
    chkboxWellKill[37] = ui->well_38;
    chkboxWellKill[38] = ui->well_39;
    chkboxWellKill[39] = ui->well_40;
    chkboxWellKill[40] = ui->well_41;
    chkboxWellKill[41] = ui->well_42;
    chkboxWellKill[42] = ui->well_43;
    chkboxWellKill[43] = ui->well_44;
    chkboxWellKill[44] = ui->well_45;
    chkboxWellKill[45] = ui->well_46;
    chkboxWellKill[46] = ui->well_47;
    chkboxWellKill[47] = ui->well_48;
    chkboxWellKill[48] = ui->well_49;
    chkboxWellKill[49] = ui->well_50;
    chkboxWellKill[50] = ui->well_51;
    chkboxWellKill[51] = ui->well_52;
    chkboxWellKill[52] = ui->well_53;
    chkboxWellKill[53] = ui->well_54;
    chkboxWellKill[54] = ui->well_55;
    chkboxWellKill[55] = ui->well_56;
    chkboxWellKill[56] = ui->well_57;
    chkboxWellKill[57] = ui->well_58;
    chkboxWellKill[58] = ui->well_59;
    chkboxWellKill[59] = ui->well_60;
    chkboxWellKill[60] = ui->well_61;
    chkboxWellKill[61] = ui->well_62;
    chkboxWellKill[62] = ui->well_63;
    chkboxWellKill[63] = ui->well_64;
    chkboxWellKill[64] = ui->well_65;
    chkboxWellKill[65] = ui->well_66;
    chkboxWellKill[66] = ui->well_67;
    chkboxWellKill[67] = ui->well_68;
    chkboxWellKill[68] = ui->well_69;
    chkboxWellKill[69] = ui->well_70;
    chkboxWellKill[70] = ui->well_71;
    chkboxWellKill[71] = ui->well_72;
    chkboxWellKill[72] = ui->well_73;
    chkboxWellKill[73] = ui->well_74;
    chkboxWellKill[74] = ui->well_75;
    chkboxWellKill[75] = ui->well_76;
    chkboxWellKill[76] = ui->well_77;
    chkboxWellKill[77] = ui->well_78;
    chkboxWellKill[78] = ui->well_79;
    chkboxWellKill[79] = ui->well_80;
    chkboxWellKill[80] = ui->well_81;
    chkboxWellKill[81] = ui->well_82;
    chkboxWellKill[82] = ui->well_83;
    chkboxWellKill[83] = ui->well_84;
    chkboxWellKill[84] = ui->well_85;
    chkboxWellKill[85] = ui->well_86;
    chkboxWellKill[86] = ui->well_87;
    chkboxWellKill[87] = ui->well_88;
    chkboxWellKill[88] = ui->well_89;
    chkboxWellKill[89] = ui->well_90;
    chkboxWellKill[90] = ui->well_91;
    chkboxWellKill[91] = ui->well_92;
    chkboxWellKill[92] = ui->well_93;
    chkboxWellKill[93] = ui->well_94;
    chkboxWellKill[94] = ui->well_95;
    chkboxWellKill[95] = ui->well_96;

}

KillSequence::~KillSequence()
{
    delete ui;
}

void KillSequence::on_btn_saveKill_released()
{
    for(int i =0; i < 96; i++)
    {
        signalKill[i] = chkboxWellKill[i]->isChecked();
    }
    accept();                             // Đóng cửa sổ
}


// Hàm để set giá trị cho các checkbox từ mảng quint8_t
void KillSequence::  setKillSequence(const quint8 values[]) {
    for (int i = 0; i < 96; ++i) {
        if (chkboxWellKill[i]) { // Kiểm tra xem checkbox có tồn tại không
            chkboxWellKill[i]->setChecked(values[i] != 0); // Set giá trị true/false
        } else {
         //   qWarning() << "Checkbox at index" << i << "is null!";
        }
    }
}

// Hàm để set giá trị cho các checkbox từ mảng quint8_t
void KillSequence::  getKillSequence(quint8 values[]) {
    for (int i = 0; i < 96; ++i) {
        if (chkboxWellKill[i]) { // Kiểm tra xem checkbox có tồn tại không
            //chkboxWellKill[i]->setChecked(values[i] != 0); // Set giá trị true/false
            values[i] = chkboxWellKill[i]->isChecked();
        } else {
         //   qWarning() << "Checkbox at index" << i << "is null!";
        }
    }
}
