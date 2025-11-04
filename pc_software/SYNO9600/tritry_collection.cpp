#include "tritry_collection.h"
#include "ui_tritry_collection.h"

tritry_collection::tritry_collection(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::tritry_collection)
{
    ui->setupUi(this);
}

tritry_collection::~tritry_collection()
{
    delete ui;
}

