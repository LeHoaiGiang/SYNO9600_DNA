#ifndef TRITRY_COLLECTION_H
#define TRITRY_COLLECTION_H

#include <QMainWindow>

namespace Ui {
class tritry_collection;
}

class tritry_collection : public QMainWindow
{
    Q_OBJECT

public:
    explicit tritry_collection(QWidget *parent = nullptr);
    ~tritry_collection();

private:
    Ui::tritry_collection *ui;
};

#endif // TRITRY_COLLECTION_H
