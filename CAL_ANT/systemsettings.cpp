#include "systemsettings.h"
#include "ui_systemsettings.h"

SystemSettings::SystemSettings(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SystemSettings)
{
    ui->setupUi(this);
}

SystemSettings::~SystemSettings()
{
    delete ui;
}

/****************************************************************
 *                                                              *
 * on_ButtonCancel_clicked() 保存设置
 *                                                              *
*****************************************************************/
void SystemSettings::on_ButtonSave_clicked()
{

}

/****************************************************************
 *                                                              *
 * on_ButtonCancel_clicked() 取消不保存设置并退出界面
 *                                                              *
*****************************************************************/
void SystemSettings::on_ButtonCancel_clicked()
{

}

