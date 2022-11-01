/******************************************************************
* UDP线程类
********************************************************************/
#ifndef UDPTHREAD_H
#define UDPTHREAD_H

#include <QObject>
#include <QUdpSocket>
#include "pvariable.h"
#include <QTimer>
#include <QTime>
#include <math.h>
#include <qdir.h>
#include <QSerialPort>        //提供访问串口的功能
#include <QSerialPortInfo>    //提供系统中存在的串口的信息
#include <QNetworkInterface>

#include "win_qextserialport.h"
#include "qextserialbase.h"
class ProdataSend;
class UdpThread : public QObject
{
    Q_OBJECT
public:
    explicit UdpThread(QObject *parent = nullptr);
public:
    // QUdpSocket *mysocket;
signals://信号
    void send_ok();
    void recv_ok();
    void resolved_state(QByteArray);//解析信号
    void Romotedata(QString);
public slots://槽
    void RecvData();
    void RecvCalData();
    void RecvRemoteData();
    void SendQueries();
    void SendCalQueries();
    void SendCal_Track();
    void SendCal_RtimeAngle();
    void SendCal_Cal_RadioSource();
    void SendCal_sweep();
private:
    void ACU_Analysis(QByteArray byte);
    void Drive_Analysis(QByteArray byte);
    void Pal_Analysis(QByteArray byte);
    void Rec_Analysis(QByteArray byte);

    void Cal_State(QByteArray byte);
    void Cal_Track(QByteArray byte);
    void Cal_Asgui(QByteArray byte);
    void Cal_RadioSource(QByteArray byte);
    void Cal_sweep(QByteArray byte);
    void Cal_sweepReport(QByteArray byte);
    void initSerial();
    QByteArray intToByte(int i, int size,int type);
    void initSend();
    QTimer *timer;
   
public:
    QUdpSocket *mysocket;
    QUdpSocket *m_calsocket;
    QUdpSocket *m_Remotesocket;
    QTimer     *m_timer;
    QTimer     *m_caltimer;
    QStringList  msg;
    QByteArray SourceAdd;//源地址
    QByteArray DestinaAdd;//目的地址
    QByteArray Number;//顺序号
    int SendNum=1;//发送的顺序号
    int size; //数据域长度
    double lastAZ=0;//与当前方位角详见计算速度
    double lastEL=0;
    QDateTime lastTime;
    QString Code;
    int num=0;
    bool AcuFlag;
    int CalFlage=0;
    int nums=0;
    int Flag;
    int t;
    QByteArray data;
    double az;
    double el;
    int Type=0;//远控发送哪个信号的标志
    int AngleReportFlag=0;
    bool PerTestFlage=false;//动态性能测试标志
    double AZangle;//离散程引文件的参数
    double ELangle;//离散程引文件的参数
    int AZmax=0;//方位最大速度
    int ELmax=0;//俯仰最大速度
    int AZmaxadd=0;//方位最大加速度
    int ELmaxadd=0;//俯仰最大加速度
    double PAZ=0,PEL=0;//离散程引文件的参数
    QThread *ProThread;
    ProdataSend *Prodatasend;
    int proType=0;//远控程引文件处理的状态
    QByteArray datagram;//组播通信接收到的信息
};

#endif // UDPTHREAD_H
