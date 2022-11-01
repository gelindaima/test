#ifndef PRODATASEND_H
#define PRODATASEND_H
#include <QThread>
#include <QUdpSocket>
#include <QTimer>
class ProdataSend : public QThread
{
public:
    ProdataSend();
    void run();
    //QUdpSocket *SendSocket;
   // QTimer mytime;
    double FirstAZ,FirstEL;
    double aa =0;
    double bb =0;
    int number=0;
    int i=0;
    double PAZ;//程引方位
    double PEL;//程引俯仰
    long long Pnumber;//程引文件发送次数
public slots:
    void Startrun();
};

#endif // PRODATASEND_H
