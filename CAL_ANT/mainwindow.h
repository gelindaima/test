#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QButtonGroup>
#include <QFileDialog>
#include <QtCharts>
#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>
#include <QFile>
#include <list>
#include <QThread>
#include <QtNetwork\QNetworkAccessManager>
#include <QtNetwork\QNetworkRequest>
#include <QtNetwork\QNetworkReply>
#include "udpthread.h"
#include "code.h"
#include "ftpserver.h"
#include "prodatasend.h"
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
public:
    double GetAE(double dX,double dY,double dZ,bool bAZ);
    void Insertlog(QString content);
    void chekstate();
    void initwidget();
    void initQChart();
    void ReadProfile();//读取程引文件
    void WriteLog();//日志文件
    void SendProData();//发送程引文件内容
    void SetUpFtp();//创建FTP
public slots:
    void UdpRcev();
private slots:
    void on_starstop_clicked();

    void on_change_clicked();

    void on_Cal_clicked();

    void on_stow_clicked();

    void on_record_clicked();

    void on_read_clicked();

    void on_exit_clicked();

    void on_ButtonSystemSettings_clicked();

    void on_ButtonELUp_clicked();

    void on_ButtonElDown_clicked();

    void on_ButtonAzLeft_clicked();

    void on_ButtonAzRight_clicked();

    void on_ButtonZero_clicked();

    void on_ACU_clicked();

    void on_RadioPosition_clicked();

    void on_RadioVel_clicked();

    void on_ButtonMove_clicked();

    void on_RadioCustomsize_clicked();

    void on_radioFirst_clicked();

    void on_radioSecond_clicked();

    void on_ButtonChooseFile_clicked();

    void on_ButtonProgramStart_clicked();

    void on_SourceCode_clicked();

    void on_ButtonProgramStop_clicked();

    void on_ButtonStop_clicked();


    void on_Standby_clicked();

    void on_pushButton_CalFlag_clicked();

    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;
    QUdpSocket * sendsocket;
    UdpThread * udpsocket;
    QThread * udpthread;
    QThread * mycomthread;
    QTimer *upstate;
    QTimer *Prostate;
    ProdataSend *Prodatasend;
    QElapsedTimer *timer;
    QLineSeries *seriesAZ;//AZ曲线
    QLineSeries *seriesEL;//EL曲线
    QChart *chart;//曲线图
    QString ProFileName;//程引文件名称
    //QStringList Prodata;
    double PAZ;//程引方位
    double PEL;//程引俯仰
    long long Pnumber;//程引文件发送次数
    QString LogFolderpath;//日志文件路径
    QString LogPath;//日志路径
    QString LogTime;//日志文件时间
    double FirstAZ,FirstEL;
    double aa =0;
    double bb =0;
    int number=0;
    int i=0;
    FtpServer FTPserver;//FTP服务器
    void init_UdpSocket();
    void init_Calparameter();
    bool Openflage=false;//上下电标志
};
#endif // MAINWINDOW_H
