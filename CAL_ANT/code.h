#ifndef CODE_H
#define CODE_H

#include <QDialog>
#include <Qtimer>
#include <list>
#include <QDebug>
namespace Ui {
class Code;
}

class Code : public QDialog
{
    Q_OBJECT

public:
    explicit Code(QWidget *parent = nullptr);
    ~Code();

public:
    void initwidget();
private slots:
    void on_pushButton_ACU_clicked();

    void on_pushButton_Remote_clicked();

    void on_pushButton_Cal_clicked();

private:
    Ui::Code *ui;
    QTimer *upstate;
    QString Title="";
};

#endif // CODE_H
