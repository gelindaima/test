#ifndef SYSTEMSET_H
#define SYSTEMSET_H

#include <QDialog>

namespace Ui {
class systemset;
}

class systemset : public QDialog
{
    Q_OBJECT

public:
    explicit systemset(QWidget *parent = nullptr);
    ~systemset();

    void updataXML();

    void initwidget();

private slots:
    void on_ButtonSave_clicked();

    void on_ButtonCancel_clicked();

private:
    Ui::systemset *ui;
};

#endif // SYSTEMSET_H
