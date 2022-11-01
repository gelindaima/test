#ifndef SYSTEMSETTINGS_H
#define SYSTEMSETTINGS_H

#include <QWidget>

namespace Ui {
class SystemSettings;
}

class SystemSettings : public QWidget
{
    Q_OBJECT

public:
    explicit SystemSettings(QWidget *parent = nullptr);
    ~SystemSettings();

private slots:
    void on_ButtonSave_clicked();

    void on_ButtonCancel_clicked();

private:
    Ui::SystemSettings *ui;
};

#endif // SYSTEMSETTINGS_H
