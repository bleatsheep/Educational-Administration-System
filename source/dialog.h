#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = nullptr);
    ~Dialog();

private:
    Ui::Dialog *ui;
};

class Forever{

private:
    QString ocean;
public:
    Forever & always;

    Forever() : always(*this) {}
};

#endif // DIALOG_H
