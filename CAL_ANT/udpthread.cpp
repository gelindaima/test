
#include "udpthread.h"
#include"prodatasend.h"
extern systemsetdata m_settings;
extern ACUState m_ACU;//ACU
extern CalibrationState m_Cal;//标效
extern RemoteState m_Remote;//远控参数
extern QMutex m_ACULock;//用于ACU通信上锁
extern QMutex m_CalLock;//用于标效通信上锁
extern Prodata m_Prodata;
extern QList<Prodata>m_Prodatanode;
extern int  flage;
extern CalQueryPsrameter m_CalQuery;//标校查询参数
extern std::list<QStringList>m_SourceCode;//源码显示
extern bool ProsendState;
extern Win_QextSerialPort* myCom;
extern bool CalFlag;
#pragma execution_character_set("utf-8")
UdpThread::UdpThread(QObject *parent) : QObject(parent)
{ 
    m_timer = new QTimer(this);
    m_timer->setTimerType(Qt::PreciseTimer);
    m_timer->start(50);

    m_caltimer = new QTimer(this);
    m_caltimer->setTimerType(Qt::PreciseTimer);
    m_caltimer->start(1000);

//    mysocket = new QUdpSocket(this);
//    mysocket->bind(m_settings.m_LocalPort);

    initSerial();

    initSend();

    QNetworkInterface networkReomte, networkCal;
    QList<QNetworkInterface>info=QNetworkInterface::allInterfaces();
    for(int i=0;i<info.size();i++)
    {
        QNetworkInterface face = info[i];
        QList<QNetworkAddressEntry> addlist = info[i].addressEntries();
        for(int j=0;j<addlist.size();j++)
        {
            QNetworkAddressEntry  add = addlist[j];
            if(add.ip()==QHostAddress("129.20.1.71"))
            {
                networkReomte=face;
            }
            else if(add.ip()==QHostAddress("192.168.1.102"))
            {
                networkCal=face;
            }
        }

    }

    m_calsocket = new QUdpSocket(this);
    //m_calsocket->bind(m_settings.m_CalibrationPort);
    m_calsocket->bind(QHostAddress::AnyIPv4,m_settings.m_CalibrationPort,QUdpSocket::ShareAddress);
    qDebug()<<m_settings.m_CalibrationPort;
    qDebug()<<QUdpSocket::ShareAddress;
    m_calsocket->setMulticastInterface(networkCal);
    m_calsocket->joinMulticastGroup(m_settings.m_CalibrationIp,networkCal);
    m_Remotesocket = new QUdpSocket(this);
    //bool b =m_Remotesocket->bind(m_settings.m_RemoteConPort);
    m_Remotesocket->bind(QHostAddress::AnyIPv4,m_settings.m_LocalConPort,QUdpSocket::ShareAddress);
    m_Remotesocket->setMulticastInterface(networkReomte);
    m_Remotesocket->joinMulticastGroup(m_settings.m_localConIp,networkReomte);

    Prodatasend = new ProdataSend;
    ProThread = new QThread;
    Prodatasend->moveToThread(ProThread);

    connect(ProThread,&QThread::started,Prodatasend,&ProdataSend::Startrun);
    connect(m_calsocket,&QUdpSocket::readyRead,this,&UdpThread::RecvCalData);//标校
    connect(m_Remotesocket,&QUdpSocket::readyRead,this,&UdpThread::RecvRemoteData);//远控
    connect(m_timer,&QTimer::timeout,this,&UdpThread::SendQueries);//ACU查询命令
  // connect(m_caltimer,&QTimer::timeout,this,&UdpThread::SendCalQueries);//标校查询命令、
    connect(m_caltimer,&QTimer::timeout,[=](){
        if(CalFlage==0)
        {
            //SendCalQueries();
            CalFlage++;
        }
        else if(CalFlage==1)
        {
            //SendCal_Track();
            CalFlage++;
        }
        else if(CalFlage==2)
        {
            //SendCal_RtimeAngle();
            CalFlage++;
        }
        else if(CalFlage==3)
        {
            SendCal_Cal_RadioSource();
            CalFlage++;
        }
        else if(CalFlage==4)
        {
            //SendCal_sweep();
            CalFlage=0;
        }
    });
}
/*********************************************************************************************
 * SendQueries() ACU发送查询命令
**********************************************************************************************/
void UdpThread::SendQueries()
{

    if(num==0)
    {
       myCom->write(Query_State,sizeof (Query_State));//伺服状态
       num++;
    }
    else if(num==1)
    {
        myCom->write(Query_DevState,sizeof (Query_DevState));//驱动状态
        num=0;
    }
//    else if(num==2)
//    {
//        myCom->write(Query_PolState,sizeof (Query_PolState));//极化工作状态
//        num=0;
//    }


//    myCom->write(Query_RecState,sizeof (Query_RecState));
    QDateTime Dtime = QDateTime::currentDateTime();
    msg<<Dtime.toString("yyyy-MM-dd HH:mm:ss     ")+"下游设备控制单元发送："+Query_State;
    msg<<Dtime.toString("yyyy-MM-dd HH:mm:ss     ")+"下游设备控制单元发送："+Query_DevState;
//    msg<<Dtime.toString("yyyy-MM-dd HH:mm:ss     ")+"下游设备控制单元发送："+Query_PolState;
//    msg<<Dtime.toString("yyyy-MM-dd HH:mm:ss     ")+"下游设备控制单元发送："+Query_RecState;
    //m_SourceCode.push_back(msg);
    msg.clear();
    if(CalFlag==true)
    {
        SendCal_RtimeAngle();
    }

}
/*********************************************************************************************
 * SendCalQueries() 发送标校查询命令
**********************************************************************************************/
void UdpThread::SendCalQueries()
{
    QByteArray Data;
    //流水号
    if(SendNum<1000)
    {
        QByteArray ee;
        ee.resize(sizeof(int));
        memcpy(ee.data(),&SendNum,sizeof(int));
        Data.append(ee);
        SendNum++;
    }
    else if(SendNum==1000)
    {
        QByteArray ee;
        ee.resize(sizeof(int));
        memcpy(ee.data(),&SendNum,sizeof(int));
        Data.append(ee);
        SendNum=1;
    }
    //qDebug()<<SendNum;
    size = Data.size();

    QByteArray FrameHead;
    //信源地址
    SourceAdd[0]=0x71;
    SourceAdd[1]=0x00;
    SourceAdd[2]=0x00;
    SourceAdd[3]=0x00;
    SourceAdd.resize(4);
    //目的地址
    DestinaAdd[0]=0x00;
    DestinaAdd[1]=0x00;
    DestinaAdd[2]=0x00;
    DestinaAdd[3]=0x00;
    DestinaAdd.resize(4);
    FrameHead = SourceAdd+DestinaAdd;
    //信息类别
    FrameHead[8] = 0x00;
    FrameHead[9] = 0xF0;
    FrameHead[10] = 0x00;
    FrameHead[11] = 0x00;
    //数据域长度
    int a =size;
    QByteArray ee;
    ee.resize(sizeof(int));
    memcpy(ee.data(),&a,sizeof(int));
    FrameHead.append(ee);

    QByteArray Record;//总数据
    Record = FrameHead+Data;
    m_calsocket->writeDatagram(Record.data(),Record.size(),m_settings.m_CalibrationIp,m_settings.m_CalibrationPort);
    QDateTime Dtime = QDateTime::currentDateTime();
    QString data = QString::fromLatin1(Record.data());
    msg<<Dtime.toString("yyyy-MM-dd HH:mm:ss     ")+"标校单元发送："+QString::fromUtf8(Record.data(),Record.size());
    m_SourceCode.push_back(msg);
    msg.clear();

}
/*********************************************************************************************
 * SendCal_Track() 发送标校轨道查询命令
**********************************************************************************************/
void UdpThread::SendCal_Track()
{
    QByteArray Data;
    //执行标识
    Data.append('\x00');
    Data.append(0x1E);
    Data.append('\x00');
    Data.append('\x00');
    //预报方式
    Data.append(m_CalQuery.ForecastMode);
    //卷织标识
    Data.append(m_CalQuery.ConvolutionFlage);
    //射电星编号
    Data.append(m_CalQuery.RadioStarNum);
    //扫描方式
    Data.append(m_CalQuery.ScanningModee);
    //扫描范围
    int a =m_CalQuery.ScanningRange;
    QByteArray ee;
    ee.resize(sizeof(int));
    memcpy(ee.data(),&a,sizeof(int));
    Data.append(ee);
    //扫描速度
    a =m_CalQuery.ScanningSpeed;
    ee.clear();
    ee.resize(sizeof(int));
    memcpy(ee.data(),&a,sizeof(int));
    Data.append(ee);
    //指定方位
    a =m_CalQuery.SpecifyAZ;
    ee.clear();
    ee.resize(sizeof(int));
    memcpy(ee.data(),&a,sizeof(int));
    Data.append(ee);
    //指定俯仰
    a =m_CalQuery.SpecifyEL;
    ee.clear();
    ee.resize(sizeof(int));
    memcpy(ee.data(),&a,sizeof(int));
    Data.append(ee);
    //温度
    a =m_CalQuery.Temperature;
    ee.clear();
    ee.resize(sizeof(int));
    memcpy(ee.data(),&a,sizeof(int));
    Data.append(ee);
    //湿度
    a =m_CalQuery.Humidity;
    ee.clear();
    ee.resize(sizeof(int));
    memcpy(ee.data(),&a,sizeof(int));
    Data.append(ee);
    //气压
    a =m_CalQuery.Pressure;
    ee.clear();
    ee.resize(sizeof(int));
    memcpy(ee.data(),&a,sizeof(int));
    Data.append(ee);
    //频率
    a =m_CalQuery.Frequency;
    ee.clear();
    ee.resize(sizeof(int));
    memcpy(ee.data(),&a,sizeof(int));
    Data.append(ee);

    //年
    Data.append(2);
    Data.append('\x00');
    Data.append(2);
    Data.append(1);
    //月
    Data.append(1);
    Data.append('\x00');
    //日
    Data.append(2);
    Data.append(5);
    //时
    Data.append(1);
    Data.append(5);
    //分
    Data.append(2);
    Data.append(6);
    //秒
    Data.append(4);
    Data.append(3);
    //毫秒
    Data.append(2);
    Data.append(5);
    //采样率
    Data.append(1);
    Data.append('\x00');
    size = Data.size();


    QByteArray FrameHead;
    //信源地址
    SourceAdd[0]=0x71;
    SourceAdd[1]=0x00;
    SourceAdd[2]=0x00;
    SourceAdd[3]=0x00;
    SourceAdd.resize(4);
    //目的地址
    DestinaAdd[0]=0x00;
    DestinaAdd[1]=0x00;
    DestinaAdd[2]=0x00;
    DestinaAdd[3]=0x00;
    DestinaAdd.resize(4);
    FrameHead = SourceAdd+DestinaAdd;
    //信息类别
    FrameHead[8] = 0x02;
    FrameHead[9] = 0xF0;
    FrameHead[10] = 0x00;
    FrameHead[11] = 0x00;
    //数据域长度
    a =size;
    ee.clear();
    ee.resize(sizeof(int));
    memcpy(ee.data(),&a,sizeof(int));
    FrameHead.append(ee);

    QByteArray Record;//总数据
    Record = FrameHead+Data;
    m_calsocket->writeDatagram(Record.data(),Record.size(),m_settings.m_CalibrationIp,m_settings.m_CalibrationPort);
    QDateTime Dtime = QDateTime::currentDateTime();
    msg<<Dtime.toString("yyyy-MM-dd HH:mm:ss     ")+"标校单元发送："+QString::fromUtf8(Record.data(),Record.size());
    m_SourceCode.push_back(msg);
    msg.clear();
}
/*********************************************************************************************
 * SendCal_RtimeAngle() 发送标校实时角度命令
**********************************************************************************************/
void UdpThread::SendCal_RtimeAngle()
{
    QByteArray Data;
    QDateTime Time = QDateTime::currentDateTime();

    //年
    int A = Time.toString("yyyy").toInt();
    QByteArray ee;
    ee.resize(sizeof(int));
    memcpy(ee.data(),&A,sizeof(int));
    Data.append(ee);
    //月
    A = Time.toString("MM").toInt();
    ee.clear();
    ee.resize(sizeof(int));
    memcpy(ee.data(),&A,sizeof(int));
    Data.append(ee);
    //日
    A = Time.toString("dd").toInt();
    ee.clear();
    ee.resize(sizeof(int));
    memcpy(ee.data(),&A,sizeof(int));
    Data.append(ee);
    //时
    A = Time.toString("hh").toInt();
    ee.clear();
    ee.resize(sizeof(int));
    memcpy(ee.data(),&A,sizeof(int));
    Data.append(ee);
    //分
    A = Time.toString("mm").toInt();
    ee.clear();
    ee.resize(sizeof(int));
    memcpy(ee.data(),&A,sizeof(int));
    Data.append(ee);
    //秒
    A = Time.toString("ss").toInt();
    ee.clear();
    ee.resize(sizeof(int));
    memcpy(ee.data(),&A,sizeof(int));
    Data.append(ee);
    //毫秒
    A = Time.toString("zz").toInt();
    ee.clear();
    ee.resize(sizeof(int));
    memcpy(ee.data(),&A,sizeof(int));
    Data.append(ee);
    //方位角
    double a =m_ACU.m_NowAz;
    ee.clear();
    ee.resize(sizeof(double));
    memcpy(ee.data(),&a,sizeof(double));
    Data.append(ee);
    //俯仰角
    a =m_ACU.m_NowEl;
    ee.clear();
    ee.resize(sizeof(double));
    memcpy(ee.data(),&a,sizeof(double));
    Data.append(ee);
    //AGC(暂时不知道是啥)
    a =0;
    ee.clear();
    ee.resize(sizeof(double));
    memcpy(ee.data(),&a,sizeof(double));
    Data.append(ee);
    size = Data.size();

    QByteArray FrameHead;
    //信源地址
    SourceAdd[0]=0x71;
    SourceAdd[1]=0x00;
    SourceAdd[2]=0x00;
    SourceAdd[3]=0x00;
    SourceAdd.resize(4);
    //目的地址
    DestinaAdd[0]=0x00;
    DestinaAdd[1]=0x00;
    DestinaAdd[2]=0x00;
    DestinaAdd[3]=0x00;
    DestinaAdd.resize(4);
    FrameHead = SourceAdd+DestinaAdd;
    //信息类别
    FrameHead[8] = 0x06;
    FrameHead[9] = 0xF0;
    FrameHead[10] = 0x00;
    FrameHead[11] = 0x00;
    //数据域长度
    a =size;
    ee.clear();
    ee.resize(sizeof(int));
    memcpy(ee.data(),&a,sizeof(int));
    FrameHead.append(ee);

    QByteArray Record;//总数据
    Record = FrameHead+Data;
    m_calsocket->writeDatagram(Record.data(),Record.size(),m_settings.m_CalibrationIp,m_settings.m_CalibrationPort);
    QDateTime Dtime = QDateTime::currentDateTime();

    msg<<Dtime.toString("yyyy-MM-dd HH:mm:ss     ")+"标校单元发送："+QString::fromUtf8(Record.data(),Record.size());
    m_SourceCode.push_back(msg);
    msg.clear();
}
/*********************************************************************************************
 * SendCal_RtimeAngle() 发送标校射电源查询命令
**********************************************************************************************/
void UdpThread::SendCal_Cal_RadioSource()
{
    QByteArray Data;
    //顺序号
//    Data.append(0x01);
//    Data.append('\x00');
    size = Data.size();

    QByteArray FrameHead;
    //信源地址
    SourceAdd[0]=0x71;
    SourceAdd[1]=0x00;
    SourceAdd[2]=0x00;
    SourceAdd[3]=0x00;
    SourceAdd.resize(4);
    //目的地址
    DestinaAdd[0]=0x00;
    DestinaAdd[1]=0x00;
    DestinaAdd[2]=0x00;
    DestinaAdd[3]=0x00;
    DestinaAdd.resize(4);
    FrameHead = SourceAdd+DestinaAdd;
    //信息类别
    FrameHead[8] = 0x08;
    FrameHead[9] = 0xF0;
    FrameHead[10] = 0x00;
    FrameHead[11] = 0x00;
    //数据域长度
    int a =size;
    QByteArray ee;
    ee.resize(sizeof(int));
    memcpy(ee.data(),&a,sizeof(int));
    FrameHead.append(ee);

    QByteArray Record;//总数据
    Record = FrameHead+Data;
    m_calsocket->writeDatagram(Record.data(),Record.size(),m_settings.m_CalibrationIp,m_settings.m_CalibrationPort);
    QDateTime Dtime = QDateTime::currentDateTime();
    msg<<Dtime.toString("yyyy-MM-dd HH:mm:ss     ")+"标校单元发送："+QString::fromUtf8(Record.data(),Record.size());
    m_SourceCode.push_back(msg);
    msg.clear();
}
/*********************************************************************************************
 * SendCal_sweep() 发送标校指向标校控制命令
**********************************************************************************************/
void UdpThread::SendCal_sweep()
{
    QByteArray Data;
    //控制方式
    Data.append(0x01);
    //当前标校点开始日期
    Data.append(0x26);
    Data.append(0x10);
    Data.append(0x21);
    Data.append(0x20);
    //当前标校点开始时间
    int hour = m_CalQuery.Starthour;
    int min = m_CalQuery.Startmin;
    int sec = m_CalQuery.Startsec;
    int msec = m_CalQuery.Startmsec;
    int a =hour*60*60*10000+min*60*10000+sec*10000+msec*10;
    QByteArray ee;
    ee.resize(sizeof(int));
    memcpy(ee.data(),&a,sizeof(int));
    Data.append(ee);
    //当前标校点结束日期
    Data.append(0x27);
    Data.append(0x10);
    Data.append(0x21);
    Data.append(0x20);
    //当前标校点结束时间
    hour = m_CalQuery.Endhour;
    min = m_CalQuery.Endmin;
    sec = m_CalQuery.Endsec;
    msec = m_CalQuery.Endmsec;
    a =hour*60*60*10000+min*60*10000+sec*10000+msec*10;
    ee.clear();
    ee.resize(sizeof(int));
    memcpy(ee.data(),&a,sizeof(int));
    Data.append(ee);
    size = Data.size();

    QByteArray FrameHead;
    //信源地址
    SourceAdd[0]=0x71;
    SourceAdd[1]=0x00;
    SourceAdd[2]=0x00;
    SourceAdd[3]=0x00;
    SourceAdd.resize(4);
    //目的地址
    DestinaAdd[0]=0x00;
    DestinaAdd[1]=0x00;
    DestinaAdd[2]=0x00;
    DestinaAdd[3]=0x00;
    DestinaAdd.resize(4);
    FrameHead = SourceAdd+DestinaAdd;
    //信息类别
    FrameHead[8] = 0x09;
    FrameHead[9] = 0xF0;
    FrameHead[10] = 0x00;
    FrameHead[11] = 0x00;
    //数据域长度
    a =size;
    ee.clear();
    ee.resize(sizeof(int));
    memcpy(ee.data(),&a,sizeof(int));
    FrameHead.append(ee);

    QByteArray Record;//总数据
    Record = FrameHead+Data;
    m_calsocket->writeDatagram(Record.data(),Record.size(),m_settings.m_CalibrationIp,m_settings.m_CalibrationPort);
    QDateTime Dtime = QDateTime::currentDateTime();
    msg<<Dtime.toString("yyyy-MM-dd HH:mm:ss     ")+"标校单元发送："+QString::fromUtf8(Record.data(),Record.size());
    m_SourceCode.push_back(msg);
    msg.clear();
}


/*********************************************************************************************
 * RecvData() 接收UDP包
**********************************************************************************************/
void UdpThread::RecvData()
{
//        myCom->waitForReadyRead(20);
//        if(myCom->bytesAvailable()>=50)
//        {


        QByteArray datagram = myCom->readAll();
        //data= myCom->readAll();
        //qDebug()<<"receive data = "<<datagram.data();
        m_ACU.state=true;

         //datagram.resize(mysocket->pendingDatagramSize());
         //mysocket->readDatagram(datagram.data(),datagram.size(), &sender, &senderPort);
          //Code =datagram.data();
          QDateTime Dtime = QDateTime::currentDateTime();
          if(datagram.size() == 0) return;
//          qDebug()<<"receive data = "<<datagram.data();
          if(nums == 0)
          {
              if(datagram[5]=='2')
              {
                  Flag=0;
              }
              else if(datagram[5]=='3')
              {
                  Flag=1;
              }
          }
          if(Flag==0)
          {
              if(nums < 2)
              {
                  data.append(datagram.data());
                  nums++;
              }
              else
              {
                  data.append(datagram.data());
                  nums=0;
                  m_ACULock.lock();
                  //在此添加伺服协议解析代码
                  ACU_Analysis(data);
                  m_ACULock.unlock();
//                  qDebug()<<"final acu data = "<<data.data();
                  data.clear();

              }
          }
          else if(Flag==1)
          {
              if(nums < 1)
              {
                  data.append(datagram.data());
                  nums++;
              }
              else
              {
                  data.append(datagram.data());
                  nums=0;
                  m_ACULock.lock();
                  Drive_Analysis(data);
                  m_ACULock.unlock();
//                  qDebug()<<"final driver data = "<<data.data();
                  data.clear();

              }
          }
//          else if(num==2)
//          {
//              if(nums < 2)
//              {
//                  data.append(datagram.data());
//                  nums++;
//              }
//              else
//              {
//                  data.append(datagram.data());
//                  nums=0;
//                  m_ACULock.lock();
//                  Pal_Analysis(data);
//                  m_ACULock.unlock();
//              }
//          }
//        qDebug()<<"final data = "<<data.data();
//          if(num < 2)
//          {
//              data.append(datagram.data());
//              num++;
//          }
//          else
//          {
//              data.append(datagram.data());
//              qDebug()<<"final data = "<<data;
//              num=0;
//              if(data.size()>0)
//              {
//                  msg<<Dtime.toString("yyyy-MM-dd HH:mm:ss     ")+"下游设备控制单元接收："+data;
//                  m_SourceCode.push_back(msg);
//                  msg.clear();
//              }

//             if(data[0] == '>' && data[1] == '0' && data[2] == '0'  && data[3] == '0')
//             {
//                 if (data[5] == '2')//伺服
//                 {
//                     m_ACULock.lock();
//                     //在此添加伺服协议解析代码
//                     ACU_Analysis(data);
//                     m_ACULock.unlock();
//                 }
//                 else if (data[5] == '1')//极化
//                 {
//                     //在此添加极化协议解析代码
//                     m_ACULock.lock();
//                     Pal_Analysis(data);
//                     m_ACULock.unlock();
//                 }
//                 else if (data[5] == '3')//驱动器详情
//                 {
//                     //在此添加驱动详情解析代码
//                     m_ACULock.lock();
//                     Drive_Analysis(data);
//                     m_ACULock.unlock();
//                 }
//                 else if (data[5] == '4')//接收机
//                 {
//                     //在此添加接收机协议解析代码
//                     m_ACULock.lock();
//                     Rec_Analysis(data);
//                     m_ACULock.unlock();
//                 }

//             }
             //data.clear();
//          }

//    }
//    if(mysocket->pendingDatagramSize()<=0)
//    {
//        m_ACU.state=false;
//    }
//        }
}
/*********************************************************************************************
 * RecvCalData() 接收标校包
**********************************************************************************************/
void UdpThread::RecvCalData()
{

    while(m_calsocket->hasPendingDatagrams())
    {
        m_Cal.State=true;
        QByteArray datagram;
        QHostAddress sender;   //客户端sender.toString()IP地址
        quint16 senderPort;   //客户端端口

        datagram.resize(m_calsocket->pendingDatagramSize());
        m_calsocket->readDatagram(datagram.data(),datagram.size(), &sender, &senderPort);
        QDateTime Dtime = QDateTime::currentDateTime();
        msg<<Dtime.toString("yyyy-MM-dd HH:mm:ss     ")+"标校单元接收："+datagram;
        m_SourceCode.push_back(msg);
        msg.clear();
        //qDebug()<<QString::number(datagram[8])<<QString::number(datagram[9]);
        //状态查询(上报)
        if(QString::number(datagram[8])=="0"&&QString::number(datagram[9])=="-16")
        {
            m_CalLock.lock();
            Cal_State(datagram);
            m_CalLock.unlock();
        }
        //轨道预报(响应)
        else if(QString::number(datagram[8])=="2"&&QString::number(datagram[9])=="-16")
        {
            m_CalLock.lock();
            Cal_Track(datagram);
            m_CalLock.unlock();
        }
        //星位引导
        else if(QString::number(datagram[8])=="5"&&QString::number(datagram[9])=="-16")
        {
            m_CalLock.lock();
            Cal_Asgui(datagram);
            m_CalLock.unlock();
        }
        //实时角度
//        else if(datagram[8] == 'F'&&datagram[9]=='0'&&datagram[10]=='0'&&datagram[11]=='6')
//        {
//            m_CalLock.lock();
//            //Cal_RtimeAngle(datagram);
//            m_CalLock.unlock();
//        }
        //射电源查询命令(响应)
        else if(QString::number(datagram[8])=="8"&&QString::number(datagram[9])=="-16")
        {
            m_CalLock.lock();
            Cal_RadioSource(datagram);
            m_CalLock.unlock();
        }
        //指向标校控制(响应)
        else if(QString::number(datagram[8])=="9"&&QString::number(datagram[9])=="-16")
        {
            m_CalLock.lock();
            Cal_sweep(datagram);
            m_CalLock.unlock();
        }
        //指向标校控制(上报)
        else if(QString::number(datagram[8])=="16"&&QString::number(datagram[9])=="-16")
        {
            m_CalLock.lock();
            Cal_sweepReport(datagram);
            m_CalLock.unlock();

        }
    }
    if(m_calsocket->pendingDatagramSize()<=0)
    {
        m_CalLock.lock();
        m_Cal.State=false;
        m_CalLock.unlock();
    }
}
/*********************************************************************************************
 * RecvRemoteData() 接收远控包
**********************************************************************************************/
void UdpThread::RecvRemoteData()
{
    while(m_Remotesocket->hasPendingDatagrams())
    {
        QHostAddress sender;   //客户端sender.toString()IP地址
        quint16 senderPort;   //客户端端口
        datagram.resize(m_Remotesocket->pendingDatagramSize());
        m_Remotesocket->readDatagram(datagram.data(),datagram.size(), &sender, &senderPort);
        msg<<datagram.data();
        m_SourceCode.push_back(msg);
        msg.clear();
        DestinaAdd = datagram.mid(0,4);//发送时的目的地址
        SourceAdd = datagram.mid(4,4);//发送时的源地址
//        qDebug()<<DestinaAdd;
//        qDebug()<<SourceAdd;
        int tem =0;
        bool a;


        if(QString::number(datagram[0])=="0"&&QString::number(datagram[1])=="0"&&QString::number(datagram[2])=="0"&&QString::number(datagram[3])=="0"&&QString::number(datagram[4])=="113"&&QString::number(datagram[5])=="0"&&QString::number(datagram[6])=="0"&&QString::number(datagram[7])=="0")
        {
            if(QString::number(datagram[8])=="1"&&QString::number(datagram[9])=="-16"&&QString::number(datagram[10])=="0"&&QString::number(datagram[11])=="0")
            {
                qDebug()<<"+++++++++++++++++++++++++++++++++";
                int a=4;
                bool bl;
                QString str,str1;
                int tem,tem1;
                double AZdata,ELdata;
                str = QString::number(datagram[28]);
                tem1 = str.toInt(&bl,10);
                str1= QString::number(datagram[29]);
                tem= str1.toInt(&bl,10);
                if(tem==65&&tem1==1)
                {
                    Type=1;
                    Number = datagram.mid(30,2);//顺序号
                    emit Romotedata("收到远控待机指令");
                    QString data = QString("<000/99\r");
                    myCom->write(data.toUtf8(),data.size());
                    //天线工作方式—待机
                    QByteArray Data;//数据域
                    //接收到的信息类别
                    Data[0]= 0x01;
                    Data[1]= 0xF0;
                    Data[2]= 0x00;
                    Data[3]= 0x00;
                    //标识
                    Data[4]= 0x01;
                    Data[5]= 0x41;
                    Data[6]= 0x00;
                    Data[7]= 0x00;
                    //顺序号
                    Data.append(Number);
                    //控制结果
                    if(flage==1)
                    {
                       Data[10]= 0x00;
                    }
                    else if(flage==2)
                    {
                        Data[10]= 0x01;
                    }
                    size = Data.size();

                    QByteArray FrameHead;//帧头
                    FrameHead.append(SourceAdd+DestinaAdd);
                    //信息类别
                    FrameHead[8] = 0x00;
                    FrameHead[9] = 0xF1;
                    FrameHead[10] = 0x00;
                    FrameHead[11] = 0x00;
                    //保留
                    FrameHead[12] = 0x00;
                    FrameHead[13] = 0x00;
                    FrameHead[14] = 0x00;
                    FrameHead[15] = 0x00;
                    //日期
                    FrameHead[16] = 0x00;
                    FrameHead[17] = 0x00;
                    FrameHead[18]= 0x00;
                    FrameHead[19]= 0x00;
                    //时间
                    FrameHead[20]= 0x00;
                    FrameHead[21]= 0x00;
                    FrameHead[22]= 0x00;
                    FrameHead[23]= 0x00;
                    //长度
                    int aa = size;
                    QByteArray ee;
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    FrameHead.append(ee);

                    QByteArray Record;//总数据
                    Record = FrameHead+Data;
                    m_Remotesocket->writeDatagram(Record.data(),Record.size(),m_settings.m_RemoteConIP,m_settings.m_RemoteConPort);
                }
                else if(tem==65&&tem1==2)
                {
                    emit Romotedata("收到远控指向指令");
                    //天线工作方式—指向

                    Type=2;

                    /***************************响应***************************/
                    Number = datagram.mid(30,2);
                    int AZ;
                    memcpy(&AZ,datagram.mid(32,4).data(),sizeof (int));
                    az=(1.0*AZ)/1000;
                    QString num,data;
                    num= QString("%1").arg(az,7,'f',3,QChar('0'));
                    data = QString("<000/04_0,+%0\r").arg(num);
                    myCom->write(data.toUtf8(),data.size());
                    int EL ;
                    memcpy(&EL,datagram.mid(36,4).data(),sizeof (int));
                    //= datagram.mid(36,4).toInt();
                     el = (1.0*EL)/1000;
                    num= QString("%1").arg(el,7,'f',3,QChar('0'));
                    data = QString("<000/04_1,+%0\r").arg(num);
                    myCom->write(data.toUtf8(),data.size());
                    QByteArray Data;

                    //接收到的信息类别
                    Data[0]= 0x01;
                    Data[1]= 0xF0;
                    Data[2]= 0x00;
                    Data[3]= 0x00;
                    //标识
                    Data[4]= 0x02;
                    Data[5]= 0x41;
                    Data[6]= 0x00;
                    Data[7]= 0x00;
                    //顺序号
                    Data.append(Number);
                    //控制结果
                    if(flage==1)
                    {
                       Data[10]= 0x00;
                    }
                    else if(flage==2)
                    {
                        Data[10]= 0x01;
                    }
                    size = Data.size();

                    QByteArray FrameHead;//帧头
                    FrameHead.append(SourceAdd+DestinaAdd);
                    //信息类别
                    FrameHead[8] = 0x00;
                    FrameHead[9] = 0xF1;
                    FrameHead[10] = 0x00;
                    FrameHead[11] = 0x00;
                    //保留
                    FrameHead[12] = 0x00;
                    FrameHead[13] = 0x00;
                    FrameHead[14] = 0x00;
                    FrameHead[15] = 0x00;
                    //日期
                    FrameHead[16] = 0x00;
                    FrameHead[17] = 0x00;
                    FrameHead[18]= 0x00;
                    FrameHead[19]= 0x00;
                    //时间
                    FrameHead[20]= 0x00;
                    FrameHead[21]= 0x00;
                    FrameHead[22]= 0x00;
                    FrameHead[23]= 0x00;
                    //长度
                    int aa = size;
                    QByteArray ee;
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    FrameHead.append(ee);

                    QByteArray Record;//总数据
                    Record = FrameHead+Data;
                    m_Remotesocket->writeDatagram(Record.data(),Record.size(),m_settings.m_RemoteConIP,m_settings.m_RemoteConPort);
                    /***************************上报***************************/
                    Data.clear();
                    //接收到的信息类别
//                        Data[0]= 0x01;
//                        Data[1]= 0xF0;
//                        Data[3]= 0x00;
//                        Data[4]= 0x00;
//                        //标识
//                        Data[4]= 0x02;
//                        Data[5]= 0x41;
//                        Data[6]= 0x00;
//                        Data[7]= 0x00;
                    //标识
                    Data[0]= 0x02;
                    Data[1]= 0x41;
                    //指向状态

                    Data[2]=0x02;
                    size=Data.size();

                    FrameHead.clear();
                    FrameHead.append(SourceAdd+DestinaAdd);
                    //信息类别
                    FrameHead[8] = 0x01;
                    FrameHead[9] = 0xF1;
                    FrameHead[10] = 0x00;
                    FrameHead[11] = 0x00;
                    //保留
                    FrameHead[12] = 0x00;
                    FrameHead[13] = 0x00;
                    FrameHead[14] = 0x00;
                    FrameHead[15] = 0x00;
                    //日期
                    FrameHead[16] = 0x00;
                    FrameHead[17] = 0x00;
                    FrameHead[18]= 0x00;
                    FrameHead[19]= 0x00;
                    //时间
                    FrameHead[20]= 0x00;
                    FrameHead[21]= 0x00;
                    FrameHead[22]= 0x00;
                    FrameHead[23]= 0x00;
                    //长度
                    aa = size;
                    ee.clear();
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    FrameHead.append(ee);

                    Record.clear();//总数据
                    Record = FrameHead+Data;
                    m_Remotesocket->writeDatagram(Record.data(),Record.size(),m_settings.m_RemoteConIP,m_settings.m_RemoteConPort);
                }
                else if(tem==65&&tem1==3)
                {
                    emit Romotedata("收到远控程序引导指令");
                    //天线工作方式—程序引导
                    QString ProFileName=datagram.mid(32,60);
                    qDebug()<<"文件名"<<ProFileName;
                    QFile file(m_settings.m_FtpPath+"/"+ProFileName);
                    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                        Message(1,1,"文件打开失败，请重试");
                        return;
                    }
                    QDateTime temp,Protime;
                    QDateTime now = QDateTime::currentDateTime();
                    double  max=0,tem=0,min=0,tem1=0;
                    QDir dir(m_settings.m_FtpPath);
                    QStringList Files = dir.entryList(QStringList()<<ProFileName,QDir::Files|QDir::Readable,QDir::Name);
                    foreach (QString temp, Files) {
                        if(temp==ProFileName)
                        {
                            qDebug()<<"远控程引";
                            QFile file(m_settings.m_FtpPath+"/"+ProFileName);
                            qDebug()<<m_settings.m_FtpPath+"/"+ProFileName;
                            if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                                proType=1;
                                return;
                            }

                            QDateTime temp,Protime;
                            QDateTime now = QDateTime::currentDateTime();
                            double  max=0,tem=0,min=0,tem1=0;
                            int i = 0;
                            int j =0;
                            m_Prodatanode.clear();
                            while(!file.atEnd()) {

                                QString Pro = file.readLine();
                                if(i>=1)//第一次为数据头 不截取
                                {
                                     QStringList aa= Pro.split('\t');
                                     m_Prodata.Time = UTCTOLocalTime(QDateTime::fromString(aa[1],"yyyy-MM-dd'T'hh:mm:ss"));
                                    if(m_Prodata.Time>now)
                                    {
                                        PAZ= aa[2].toDouble();
                                        m_Prodata.AZ=PAZ;
                                        //ui->label_PAZ->setText(QString::number(PAZ));
                                        //每一次的俯仰角度
                                        PEL= aa[3].toDouble();
                                        m_Prodata.EL=PEL;
                                        if(j==0)//第一次的数据直接发送
                                        {
                                            m_Prodata.AZAngle=0;
                                            m_Prodata.ELAngle=0;
                                            m_Prodata.FirstAZ=PAZ;
                                            m_Prodata.FirstEL=PEL;
                                            tem=PAZ;
                                            tem1=PEL;
                                            temp=m_Prodata.Time;
                                            max=PEL;
                                            min=PEL;
                                            m_Prodatanode.push_back(m_Prodata);
                                        }
                                        else if(j>0)//后面数据先解析再发送
                                        {
                                            int bb=temp.secsTo(m_Prodata.Time);
                                            m_Prodata.number=(bb*1000)/20;
                                            for(int i = 0;i<m_Prodata.number;i++)
                                            {
                                                temp=temp.addMSecs(20);
                                                m_Prodata.Time=temp;
                                                m_Prodata.AZAngle+=((PAZ-tem)/(bb*1000))*20;
                                                m_Prodata.ELAngle+=((PEL-tem1)/(bb*1000))*20;
                                                m_Prodatanode.push_back(m_Prodata);
                                            }
                                            tem=PAZ;
                                            tem1=PEL;
                                        }
                                        j++;
                                    }
                                }
                                i++;
                            }
                            if(m_Prodatanode.size()==0)
                            {
                                proType=2;
                                Type=0;
                                ProsendState=true;
                            }
                            else
                            {
                                proType=1;
                                ProsendState=true;
                                ProThread->start();
                            }
                        }
                    /***************************响应***************************/
                    Number = datagram.mid(30,2);

                    Type=3;
                    QByteArray Data;
                    //接收到的信息类别
                    Data[0]= 0x01;
                    Data[1]= 0xF0;
                    Data[2]= 0x00;
                    Data[3]= 0x00;
                    //标识
                    Data[4]= 0x03;
                    Data[5]= 0x41;
                    Data[6]= 0x00;
                    Data[7]= 0x00;
                    //顺序号
                    Data.append(Number);
                    //控制结果
                    if(flage==1)
                    {
                       Data[10]= 0x00;
                    }
                    else if(flage==2)
                    {
                        Data[10]= 0x01;
                    }
                    size = Data.size();

                    QByteArray FrameHead;
                    FrameHead.append(SourceAdd+DestinaAdd);
                    //信息类别
                    FrameHead[8] = 0x00;
                    FrameHead[9] = 0xF1;
                    FrameHead[10] = 0x00;
                    FrameHead[11] = 0x00;
                    //保留
                    FrameHead[12] = 0x00;
                    FrameHead[13] = 0x00;
                    FrameHead[14] = 0x00;
                    FrameHead[15] = 0x00;
                    //日期
                    FrameHead[16] = 0x00;
                    FrameHead[17] = 0x00;
                    FrameHead[18]= 0x00;
                    FrameHead[19]= 0x00;
                    //时间
                    FrameHead[20]= 0x00;
                    FrameHead[21]= 0x00;
                    FrameHead[22]= 0x00;
                    FrameHead[23]= 0x00;
                    //长度
                    int aa = size;
                    QByteArray ee;
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    FrameHead.append(ee);


                    QByteArray Record;//总数据
                    Record = FrameHead+Data;
                    m_Remotesocket->writeDatagram(Record.data(),Record.size(),m_settings.m_RemoteConIP,m_settings.m_RemoteConPort);
                    qDebug()<<m_settings.m_RemoteConIP;
                    qDebug()<<m_settings.m_RemoteConPort;
                    /***************************上报***************************/
                    Data.clear();
                    //标识
                    Data[0]= 0x03;
                    Data[1]= 0x41;
                    //程引状态
                    Data[2]=0x01;
                    //偏置状态
                    Data[3]=0x01;
                    //方位偏置
                    aa = 0;
                    ee.clear();
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    Data.append(ee);
                    //俯仰偏置
                    aa = 0;
                    ee.clear();
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    Data.append(ee);
                    //时间偏置
                    aa = 0;
                    ee.clear();
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    Data.append(ee);
                    size = Data.size();

                    FrameHead.clear();
                    FrameHead.append(SourceAdd+DestinaAdd);
                    //信息类别
                    FrameHead[8] = 0x01;
                    FrameHead[9] = 0xF1;
                    FrameHead[10] = 0x00;
                    FrameHead[11] = 0x00;
                    //保留
                    FrameHead[12] = 0x00;
                    FrameHead[13] = 0x00;
                    FrameHead[14] = 0x00;
                    FrameHead[15] = 0x00;
                    //日期
                    FrameHead[16] = 0x00;
                    FrameHead[17] = 0x00;
                    FrameHead[18]= 0x00;
                    FrameHead[19]= 0x00;
                    //时间
                    FrameHead[20]= 0x00;
                    FrameHead[21]= 0x00;
                    FrameHead[22]= 0x00;
                    FrameHead[23]= 0x00;
                    //长度
                    aa = size;
                    ee.clear();
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    FrameHead.append(ee);


                    Record.clear();//总数据
                    Record = FrameHead+Data;
                    m_Remotesocket->writeDatagram(Record.data(),Record.size(),m_settings.m_RemoteConIP,m_settings.m_RemoteConPort);
                  }
                }
                else if(tem==65&&tem1==4)
                {
                    Type=4;
                    emit Romotedata("收到远控收藏指令");
                    //天线工作方式—收藏
                    /***************************响应***************************/
                    Number = datagram.mid(30,2);
                    QString num,data;
                    num= QString("%1").arg(170,7,'f',3,QChar('0'));
                    data = QString("<000/04_0,+%0\r").arg(num);
                    myCom->write(data.toUtf8(),data.size());
                    num= QString("%1").arg(88,7,'f',3,QChar('0'));
                    data = QString("<000/04_1,+%0\r").arg(num);
                    myCom->write(data.toUtf8(),data.size());
                    QByteArray Data;
                    //接收到的信息类别
                    Data[0]= 0x01;
                    Data[1]= 0xF0;
                    Data[2]= 0x00;
                    Data[3]= 0x00;
                    //标识
                    Data[4]= 0x04;
                    Data[5]= 0x41;
                    Data[6]= 0x00;
                    Data[7]= 0x00;
                    //顺序号
                    Data.append(Number);
                    //控制结果
                    if(flage==1)
                    {
                       Data[10]= 0x00;
                    }
                    else if(flage==2)
                    {
                        Data[10]= 0x01;
                    }
                    //长度
                    size = Data.size();


                    QByteArray FrameHead;
                    FrameHead.append(SourceAdd+DestinaAdd);
                    //信息类别
                    FrameHead[8] = 0x00;
                    FrameHead[9] = 0xF1;
                    FrameHead[10] = 0x00;
                    FrameHead[11] = 0x00;
                    //保留
                    FrameHead[12] = 0x00;
                    FrameHead[13] = 0x00;
                    FrameHead[14] = 0x00;
                    FrameHead[15] = 0x00;
                    //日期
                    FrameHead[16] = 0x00;
                    FrameHead[17] = 0x00;
                    FrameHead[18]= 0x00;
                    FrameHead[19]= 0x00;
                    //时间
                    FrameHead[20]= 0x00;
                    FrameHead[21]= 0x00;
                    FrameHead[22]= 0x00;
                    FrameHead[23]= 0x00;
                    //长度
                    int aa = size;
                    QByteArray ee;
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    FrameHead.append(ee);


                    QByteArray Record;//总数据
                    Record = FrameHead+Data;
                    m_Remotesocket->writeDatagram(Record.data(),Record.size(),m_settings.m_RemoteConIP,m_settings.m_RemoteConPort);
                    /***************************上报***************************/
                    Data.clear();
                    //接收到的信息类别
//                        Data[0]= 0x01;
//                        Data[1]= 0xF0;
//                        Data[2]= 0x00;
//                        Data[3]= 0x00;
                    //标识
                    Data[0]= 0x04;
                    Data[1]= 0x41;
                    //收藏状态
                    Data[2]=0x01;
                    size = Data.size();


                    FrameHead.clear();
                    FrameHead.append(SourceAdd+DestinaAdd);
                    //信息类别
                    FrameHead[8] = 0x01;
                    FrameHead[9] = 0xF1;
                    FrameHead[10] = 0x00;
                    FrameHead[11] = 0x00;
                    //保留
                    FrameHead[12] = 0x00;
                    FrameHead[13] = 0x00;
                    FrameHead[14] = 0x00;
                    FrameHead[15] = 0x00;
                    //日期
                    FrameHead[16] = 0x00;
                    FrameHead[17] = 0x00;
                    FrameHead[18]= 0x00;
                    FrameHead[19]= 0x00;
                    //时间
                    FrameHead[20]= 0x00;
                    FrameHead[21]= 0x00;
                    FrameHead[22]= 0x00;
                    FrameHead[23]= 0x00;
                    //长度
                    aa = size;
                    ee.clear();
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    FrameHead.append(ee);


                    Record.clear();//总数据
                    Record = FrameHead+Data;
                    m_Remotesocket->writeDatagram(Record.data(),Record.size(),m_settings.m_RemoteConIP,m_settings.m_RemoteConPort);
                }
                else if(tem==65&&tem1==5)
                {
                    emit Romotedata("收到远控动态特性测试指令");
                    if((int)m_ACU.m_NowAz>=90&&(int)m_ACU.m_NowAz<180)
                    {
                        AZangle =m_ACU.m_NowAz-10;
                    }
                    else if((int)m_ACU.m_NowAz>0&&(int)m_ACU.m_NowAz<90)
                    {
                        AZangle =m_ACU.m_NowAz+10;
                    }
                    if((int)m_ACU.m_NowEl>0&&(int)m_ACU.m_NowEl<=45)
                    {
                        ELangle=m_ACU.m_NowEl+10;
                    }
                    else if((int)m_ACU.m_NowEl>45&&(int)m_ACU.m_NowEl<88)
                    {
                        ELangle=m_ACU.m_NowEl-10;
                    }
                    if(QString::number(datagram[32])=="1")
                    {
                      PerTestFlage=true;
                    }
                    else
                    {
                        PerTestFlage=false;
                    }
                    Type=5;
                    //动态特性测试
                    /***************************响应***************************/
                    Number = datagram.mid(30,2);
                    QByteArray Data;
                    //接收到的信息类别
                    Data[0]= 0x01;
                    Data[1]= 0xF0;
                    Data[2]= 0x00;
                    Data[3]= 0x00;
                    //标识
                    Data[4]= 0x05;
                    Data[5]= 0x41;
                    Data[6]= 0x00;
                    Data[7]= 0x00;
                    //顺序号
                    Data.append(Number);
                    //控制结果
                    if(flage==1)
                    {
                       Data[10]= 0x00;
                    }
                    else if(flage==2)
                    {
                        Data[10]= 0x01;
                    }
                    size = Data.size();


                    QByteArray FrameHead;
                    FrameHead.append(SourceAdd+DestinaAdd);
                    //信息类别
                    FrameHead[8] = 0x00;
                    FrameHead[9] = 0xF1;
                    FrameHead[10] = 0x00;
                    FrameHead[11] = 0x00;
                    //保留
                    FrameHead[12] = 0x00;
                    FrameHead[13] = 0x00;
                    FrameHead[14] = 0x00;
                    FrameHead[15] = 0x00;
                    //日期
                    FrameHead[16] = 0x00;
                    FrameHead[17] = 0x00;
                    FrameHead[18]= 0x00;
                    FrameHead[19]= 0x00;
                    //时间
                    FrameHead[20]= 0x00;
                    FrameHead[21]= 0x00;
                    FrameHead[22]= 0x00;
                    FrameHead[23]= 0x00;
                    //长度
                    int aa = size;
                    QByteArray ee;
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    FrameHead.append(ee);

                    QByteArray Record;//总数据
                    Record = FrameHead+Data;
                    m_Remotesocket->writeDatagram(Record.data(),Record.size(),m_settings.m_RemoteConIP,m_settings.m_RemoteConPort);
                    /***************************上报***************************/
                    Data.clear();
                    //接收到的信息类别
//                        Data[0]= 0x01;
//                        Data[1]= 0xF0;
//                        Data[2]= 0x00;
//                        Data[3]= 0x00;
                    //标识
                    Data[0]= 0x05;
                    Data[1]= 0x41;
                    //测试状态
                    Data[2]=0x01;
                    //方位最大速度
                    aa = 0;
                    ee.clear();
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    Data.append(ee);
                    //俯仰最大速度
                    aa = 0;
                    ee.clear();
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    Data.append(ee);
                    //方位最大加速度
                    aa = 0;
                    ee.clear();
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    Data.append(ee);
                    //俯仰最大加速度
                    aa = 0;
                    ee.clear();
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    Data.append(ee);
                    //数据域长度
                    size = Data.size();

                    FrameHead.clear();
                    FrameHead.append(SourceAdd+DestinaAdd);
                    //信息类别
                    FrameHead[8] = 0x01;
                    FrameHead[9] = 0xF1;
                    FrameHead[10] = 0x00;
                    FrameHead[11] = 0x00;
                    //保留
                    FrameHead[12] = 0x00;
                    FrameHead[13] = 0x00;
                    FrameHead[14] = 0x00;
                    FrameHead[15] = 0x00;
                    //日期
                    FrameHead[16] = 0x00;
                    FrameHead[17] = 0x00;
                    FrameHead[18]= 0x00;
                    FrameHead[19]= 0x00;
                    //时间
                    FrameHead[20]= 0x00;
                    FrameHead[21]= 0x00;
                    FrameHead[22]= 0x00;
                    FrameHead[23]= 0x00;
                    //长度
                    aa = size;
                    ee.clear();
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    FrameHead.append(ee);

                    Record.clear();//总数据
                    Record = FrameHead+Data;
                    m_Remotesocket->writeDatagram(Record.data(),Record.size(),m_settings.m_RemoteConIP,m_settings.m_RemoteConPort);
                }
                else if(tem==65&&tem1==6)
                {
                    Type=6;
                    emit Romotedata("收到远控铃声警告控制指令");
                    //铃声告警控制(暂时保留)
                    Number = datagram.mid(30,2);
                    QByteArray Data;
                    //接收到的信息类别
                    Data[0]= 0x01;
                    Data[1]= 0xF0;
                    Data[2]= 0x00;
                    Data[3]= 0x00;
                    //标识
                    Data[4]= 0x06;
                    Data[5]= 0x41;
                    Data[6]= 0x00;
                    Data[7]= 0x00;
                    //顺序号
                    Data.append(Number);
                    //控制结果
                    if(flage==1)
                    {
                       Data[10]= 0x00;
                    }
                    else if(flage==2)
                    {
                        Data[10]= 0x01;
                    }

                    size = Data.size();
                    QByteArray FrameHead;
                    FrameHead.append(SourceAdd+DestinaAdd);
                    //信息类别
                    FrameHead[8] = 0x00;
                    FrameHead[9] = 0xF1;
                    FrameHead[10] = 0x00;
                    FrameHead[11] = 0x00;
                    //保留
                    FrameHead[12] = 0x00;
                    FrameHead[13] = 0x00;
                    FrameHead[14] = 0x00;
                    FrameHead[15] = 0x00;
                    //日期
                    FrameHead[16] = 0x00;
                    FrameHead[17] = 0x00;
                    FrameHead[18]= 0x00;
                    FrameHead[19]= 0x00;
                    //时间
                    FrameHead[20]= 0x00;
                    FrameHead[21]= 0x00;
                    FrameHead[22]= 0x00;
                    FrameHead[23]= 0x00;
                    //长度
                    int aa = size;
                    QByteArray ee;
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    FrameHead.append(ee);
                    QByteArray Record = FrameHead+Data;
                    m_Remotesocket->writeDatagram(Record.data(),Record.size(),m_settings.m_RemoteConIP,m_settings.m_RemoteConPort);
                }
                else if(tem==65&&tem1==7)
                {
                    Type=7;
                    emit Romotedata("收到远控灯光警告控制指令");
                    //灯光告警控制(暂时保留)
                    Number = datagram.mid(30,2);
                    QByteArray Data;
                    //接收到的信息类别
                    Data[0]= 0x01;
                    Data[1]= 0xF0;
                    Data[2]= 0x00;
                    Data[3]= 0x00;
                    //标识
                    Data[4]= 0x07;
                    Data[5]= 0x41;
                    Data[6]= 0x00;
                    Data[7]= 0x00;
                    //顺序号
                    Data.append(Number);
                    //控制结果
                    if(flage==1)
                    {
                       Data[10]= 0x00;
                    }
                    else if(flage==2)
                    {
                        Data[10]= 0x01;
                    }

                    size = Data.size();
                    QByteArray FrameHead;
                    FrameHead.append(SourceAdd+DestinaAdd);
                    //信息类别
                    FrameHead[8] = 0x00;
                    FrameHead[9] = 0xF1;
                    FrameHead[10] = 0x00;
                    FrameHead[11] = 0x00;
                    //保留
                    FrameHead[12] = 0x00;
                    FrameHead[13] = 0x00;
                    FrameHead[14] = 0x00;
                    FrameHead[15] = 0x00;
                    //日期
                    FrameHead[16] = 0x00;
                    FrameHead[17] = 0x00;
                    FrameHead[18]= 0x00;
                    FrameHead[19]= 0x00;
                    //时间
                    FrameHead[20]= 0x00;
                    FrameHead[21]= 0x00;
                    FrameHead[22]= 0x00;
                    FrameHead[23]= 0x00;
                    //长度
                    int aa = size;
                    QByteArray ee;
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    FrameHead.append(ee);
                    QByteArray Record = FrameHead+Data;
                    m_Remotesocket->writeDatagram(Record.data(),Record.size(),m_settings.m_RemoteConIP,m_settings.m_RemoteConPort);
                }
                else if(tem==65&&tem1==8)
                {
                    emit Romotedata("收到远控射电源查询命令指令");
                    Type=8;
                    //射电源查询命令
                    /***************************响应***************************/
                    Number = datagram.mid(30,2);
                    QByteArray Data;
                    //接收到的信息类别
                    Data[0]= 0x01;
                    Data[1]= 0xF0;
                    Data[2]= 0x00;
                    Data[3]= 0x00;
                    //标识
                    Data[4]= 0x08;
                    Data[5]= 0x41;
                    Data[6]= 0x00;
                    Data[7]= 0x00;
                    //顺序号
                    Data.append(Number);
                    //控制结果
                    if(flage==1)
                    {
                       Data[10]= 0x00;
                    }
                    else if(flage==2)
                    {
                        Data[10]= 0x01;
                    }
                    //数据域长度
                    size = Data.size();


                    QByteArray FrameHead;
                    FrameHead.append(SourceAdd+DestinaAdd);
                    //信息类别
                    FrameHead[8] = 0x00;
                    FrameHead[9] = 0xF1;
                    FrameHead[10] = 0x00;
                    FrameHead[11] = 0x00;
                    //保留
                    FrameHead[12] = 0x00;
                    FrameHead[13] = 0x00;
                    FrameHead[14] = 0x00;
                    FrameHead[15] = 0x00;
                    //日期
                    FrameHead[16] = 0x00;
                    FrameHead[17] = 0x00;
                    FrameHead[18]= 0x00;
                    FrameHead[19]= 0x00;
                    //时间
                    FrameHead[20]= 0x00;
                    FrameHead[21]= 0x00;
                    FrameHead[22]= 0x00;
                    FrameHead[23]= 0x00;
                    //长度
                    int aa = size;
                    QByteArray ee;
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    FrameHead.append(ee);


                    QByteArray  Record;//总数据
                    Record = FrameHead+Data;
                    m_Remotesocket->writeDatagram(Record.data(),Record.size(),m_settings.m_RemoteConIP,m_settings.m_RemoteConPort);
                    /***************************上报***************************/
                    Data.clear();
                    //接收到的信息类别
//                        Data[0]= 0x01;
//                        Data[1]= 0xF0;
//                        Data[2]= 0x00;
//                        Data[3]= 0x00;
                    //标识
                    Data[0]= 0x08;
                    Data[1]= 0x41;
                    //生成预报文件数量
                    Data[2]=0x01;
                    //文件名(字节数不够就在后面添0)
                    QByteArray a;
                    QString Name="obj_yyyymmddHHMMSS.txt";
                    a = Name.toLocal8Bit();
                    while(a.size()<61)
                    {
                        a.append('0');
                    }
                    Data.append(a);
                    size = Data.size();

                    FrameHead.clear();
                    FrameHead.append(SourceAdd+DestinaAdd);
                    //信息类别
                    FrameHead[8] = 0x01;
                    FrameHead[9] = 0xF1;
                    FrameHead[10] = 0x00;
                    FrameHead[11] = 0x00;
                    //保留
                    FrameHead[12] = 0x00;
                    FrameHead[13] = 0x00;
                    FrameHead[14] = 0x00;
                    FrameHead[15] = 0x00;
                    //日期
                    FrameHead[16] = 0x00;
                    FrameHead[17] = 0x00;
                    FrameHead[18]= 0x00;
                    FrameHead[19]= 0x00;
                    //时间
                    FrameHead[20]= 0x00;
                    FrameHead[21]= 0x00;
                    FrameHead[22]= 0x00;
                    FrameHead[23]= 0x00;
                    //长度
                    aa = size;
                    ee.clear();
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    FrameHead.append(ee);

                    Record.clear();//总数据
                    Record = FrameHead+Data;
                    m_Remotesocket->writeDatagram(Record.data(),Record.size(),m_settings.m_RemoteConIP,m_settings.m_RemoteConPort);
                }
                else if(tem==65&&tem1==9)
                {
                    Type=9;
                    emit Romotedata("收到远控指向标校控制指令");
                    //指向标校控制
                    /***************************响应***************************/
                    Number = datagram.mid(30,2);
                    QByteArray Data;
                    //接收到的信息类别
                    Data[0]= 0x01;
                    Data[1]= 0xF0;
                    Data[2]= 0x00;
                    Data[3]= 0x00;
                    //标识
                    Data[4]= 0x09;
                    Data[5]= 0x41;
                    Data[6]= 0x00;
                    Data[7]= 0x00;
                    //顺序号
                    Data.append(Number);
                    //控制结果
                    if(flage==1)
                    {
                       Data[10]= 0x00;
                    }
                    else if(flage==2)
                    {
                        Data[10]= 0x01;
                    }
                    //数据域长度
                    size = Data.size();


                    QByteArray FrameHead;
                    FrameHead.append(SourceAdd+DestinaAdd);
                    //信息类别
                    FrameHead[8] = 0x00;
                    FrameHead[9] = 0xF1;
                    FrameHead[10] = 0x00;
                    FrameHead[11] = 0x00;
                    //保留
                    FrameHead[12] = 0x00;
                    FrameHead[13] = 0x00;
                    FrameHead[14] = 0x00;
                    FrameHead[15] = 0x00;
                    //日期
                    FrameHead[16] = 0x00;
                    FrameHead[17] = 0x00;
                    FrameHead[18]= 0x00;
                    FrameHead[19]= 0x00;
                    //时间
                    FrameHead[20]= 0x00;
                    FrameHead[21]= 0x00;
                    FrameHead[22]= 0x00;
                    FrameHead[23]= 0x00;
                    //长度
                    int aa = size;
                    QByteArray ee;
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    FrameHead.append(ee);

                    QByteArray  Record;//总数据
                    Record = FrameHead+Data;
                    m_Remotesocket->writeDatagram(Record.data(),Record.size(),m_settings.m_RemoteConIP,m_settings.m_RemoteConPort);
                    /***************************上报***************************/
                    Data.clear();
                    //接收到的信息类别
//                        Data[0]= 0x01;
//                        Data[1]= 0xF0;
//                        Data[2]= 0x00;
//                        Data[3]= 0x00;
                    //标识
                    Data[0]= 0x09;
                    Data[1]= 0x41;
                    //标识状态
                    Data[2]=0x01;
                    //标校射电源个数
                    Data[3]=0x00;
                    //标校点个数
                    Data[4]=0x00;
                    //当前标校点标识
                    Data[5]=0x01;
                    //当前目标名称
                    QByteArray a;
                    QString Name="test";
                    a = Name.toLocal8Bit();
                    while(a.size()<10)
                    {
                        a.append('\x00');
                    }
                    Data.append(a);
                    //当前标校点开始日期
                    Data.append(2);
                    Data.append('\x00');
                    Data.append(2);
                    Data.append(1);
                    Data.append(1);
                    Data.append('\x00');
                    Data.append(2);
                    Data.append(5);
                    //当前标校点开始时间
                    Data.append(1);
                    Data.append(5);
                    Data.append(2);
                    Data.append(6);
                    Data.append(4);
                    Data.append(3);
                    Data.append(2);
                    Data.append(5);
                    //当前标校点结束日期
                    Data.append(2);
                    Data.append('\x00');
                    Data.append(2);
                    Data.append(1);
                    Data.append(1);
                    Data.append('\x00');
                    Data.append(2);
                    Data.append(5);
                    //当前标校点结束时间
                    Data.append(1);
                    Data.append(5);
                    Data.append(2);
                    Data.append(6);
                    Data.append(4);
                    Data.append(8);
                    Data.append(2);
                    Data.append(2);
                    //数据域长度
                    size = Data.size();


                    FrameHead.clear();
                    FrameHead.append(SourceAdd+DestinaAdd);
                    //信息类别
                    FrameHead[0] = 0x01;
                    FrameHead[1] = 0xF1;
                    FrameHead[2] = 0x00;
                    FrameHead[3] = 0x00;
                    //保留
                    FrameHead[4] = 0x00;
                    FrameHead[5] = 0x00;
                    FrameHead[6] = 0x00;
                    FrameHead[7] = 0x00;
                    //日期
                    FrameHead[8] = 0x00;
                    FrameHead[9] = 0x00;
                    FrameHead[10]= 0x00;
                    FrameHead[11]= 0x00;
                    //时间
                    FrameHead[12]= 0x00;
                    FrameHead[13]= 0x00;
                    FrameHead[14]= 0x00;
                    FrameHead[15]= 0x00;
                    //长度
                    aa = size;
                    ee.clear();
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    FrameHead.append(ee);

                    Record.clear();//总数据
                    Record = FrameHead+Data;
                    m_Remotesocket->writeDatagram(Record.data(),Record.size(),m_settings.m_RemoteConIP,m_settings.m_RemoteConPort);
                }
                else if(tem==65&&tem1==10)
                {
                    Type=10;
                    emit Romotedata("收到远控角度上报控制指令");
                    //角度上报控制
                    if(QString::number(datagram[32])=="1")
                    {
                        AngleReportFlag = 1;
                    }
                    else if(QString::number(datagram[32])=="2")
                    {
                        AngleReportFlag = 2;
                    }
                    /***************************响应***************************/
                    Number = datagram.mid(30,2);
                    QByteArray Data;
                    //接收到的信息类别
                    Data[0]= 0x01;
                    Data[1]= 0xF0;
                    Data[2]= 0x00;
                    Data[3]= 0x00;
                    //标识
                    Data[4]= 0x0A;
                    Data[5]= 0x41;
                    Data[6]= 0x00;
                    Data[7]= 0x00;
                    //顺序号
                    Data.append(Number);
                    //控制结果
                    if(flage==1)
                    {
                       Data[10]= 0x00;
                    }
                    else if(flage==2)
                    {
                        Data[10]= 0x01;
                    }
                    size = Data.size();


                    QByteArray FrameHead;
                    FrameHead.append(SourceAdd+DestinaAdd);
                    //信息类别
                    FrameHead[8] = 0x00;
                    FrameHead[9] = 0xF1;
                    FrameHead[10] = 0x00;
                    FrameHead[11] = 0x00;
                    //保留
                    FrameHead[12] = 0x00;
                    FrameHead[13] = 0x00;
                    FrameHead[14] = 0x00;
                    FrameHead[15] = 0x00;
                    //日期
                    FrameHead[16] = 0x00;
                    FrameHead[17] = 0x00;
                    FrameHead[18]= 0x00;
                    FrameHead[19]= 0x00;
                    //时间
                    FrameHead[20]= 0x00;
                    FrameHead[21]= 0x00;
                    FrameHead[22]= 0x00;
                    FrameHead[23]= 0x00;
                    //长度
                    int aa = size;
                    QByteArray ee;
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    FrameHead.append(ee);


                    QByteArray  Record;//总数据
                    Record = FrameHead+Data;
                    m_Remotesocket->writeDatagram(Record.data(),Record.size(),m_settings.m_RemoteConIP,m_settings.m_RemoteConPort);
                    /***************************上报***************************/
                    Data.clear();
                    //接收到的信息类别
//                        Data[0]= 0x01;
//                        Data[1]= 0xF0;
//                        Data[2]= 0x00;
//                        Data[3]= 0x00;
                    //标识
                    Data[0]= 0x0A;
                    Data[1]= 0x41;
                    //方位
                    aa = (int)(m_ACU.m_NowAz*1000);
                    ee.clear();
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    Data.append(ee);
                    //俯仰
                    aa = (int)(m_ACU.m_NowEl*1000);
                    ee.clear();
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    Data.append(ee);
                    //相心_X
                    aa = 0;
                    ee.clear();
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    Data.append(ee);
                    //相心_Y
                    aa = 0;
                    ee.clear();
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    Data.append(ee);
                    //相心_Z
                    aa = 0;
                    ee.clear();
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    Data.append(ee);
                    //数据域长度
                    size = Data.size();


                    FrameHead.clear();
                    FrameHead.append(SourceAdd+DestinaAdd);
                    //信息类别
                    FrameHead[8] = 0x01;
                    FrameHead[9] = 0xF1;
                    FrameHead[10] = 0x00;
                    FrameHead[11] = 0x00;
                    //保留
                    FrameHead[12] = 0x00;
                    FrameHead[13] = 0x00;
                    FrameHead[14] = 0x00;
                    FrameHead[15] = 0x00;
                    //日期
                    FrameHead[16] = 0x00;
                    FrameHead[17] = 0x00;
                    FrameHead[18]= 0x00;
                    FrameHead[19]= 0x00;
                    //时间
                    FrameHead[20]= 0x00;
                    FrameHead[21]= 0x00;
                    FrameHead[22]= 0x00;
                    FrameHead[23]= 0x00;
                    //长度
                    aa = size;
                    ee.clear();
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    FrameHead.append(ee);

                    Record.clear();//总数据
                    Record = FrameHead+Data;
                    m_Remotesocket->writeDatagram(Record.data(),Record.size(),m_settings.m_RemoteConIP,m_settings.m_RemoteConPort);
                }
                else if(tem==65&&tem1==11)
                {
                    Type=11;
                    emit Romotedata("收到远控气象数据广播指令");
                    //气象数据广播
                }
                else if(tem==65&&tem1==12)
                {
                    Type=12;
                    emit Romotedata("收到远控远程上/去电控制指令");
                    if(QString::number(datagram[32])=="1")
                    {
                        QString data="<000/01\r";
                        myCom->write(data.toUtf8(),data.size());
                    }
                    else if(QString::number(datagram[32])=='2')
                    {
                        QString data="<000/02\r";
                        myCom->write(data.toUtf8(),data.size());
                    }
                    //远程上电控制命令
                    /***************************响应***************************/
                    Number = datagram.mid(30,2);
                    QByteArray Data;
                    //接收到的信息类别
                    Data[0]= 0x01;
                    Data[1]= 0xF0;
                    Data[2]= 0x00;
                    Data[3]= 0x00;
                    //标识
                    Data[4]= 0x0C;
                    Data[5]= 0x41;
                    Data[6]= 0x00;
                    Data[7]= 0x00;
                    //顺序号
                    Data.append(Number);
                    //控制结果
                    if(flage==1)
                    {
                       Data[10]= 0x00;
                    }
                    else if(flage==2)
                    {
                        Data[10]= 0x01;
                    }
                    //数据域长度
                    size = Data.size();


                    QByteArray FrameHead;
                    FrameHead.append(SourceAdd+DestinaAdd);
                    //信息类别
                    FrameHead[8] = 0x00;
                    FrameHead[9] = 0xF1;
                    FrameHead[10] = 0x00;
                    FrameHead[11] = 0x00;
                    //保留
                    FrameHead[12] = 0x00;
                    FrameHead[13] = 0x00;
                    FrameHead[14] = 0x00;
                    FrameHead[15] = 0x00;
                    //日期
                    FrameHead[16] = 0x00;
                    FrameHead[17] = 0x00;
                    FrameHead[18]= 0x00;
                    FrameHead[19]= 0x00;
                    //时间
                    FrameHead[20]= 0x00;
                    FrameHead[21]= 0x00;
                    FrameHead[22]= 0x00;
                    FrameHead[23]= 0x00;
                    //长度
                    int aa = size;
                    QByteArray ee;
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    FrameHead.append(ee);


                    QByteArray  Record;//总数据
                    Record = FrameHead+Data;
                    m_Remotesocket->writeDatagram(Record.data(),Record.size(),m_settings.m_RemoteConIP,m_settings.m_RemoteConPort);
                }
            }
            else if(QString::number(datagram[8])=="4"&&QString::number(datagram[9])=="-16"&&QString::number(datagram[10])=="0"&&QString::number(datagram[11])=="0")
            {

                //状态查询
                QByteArray Data;//数据域
                //顺序号
                Number = datagram.mid(10,2);
                Data.append(Number);

                Data.append(0xFB);//单元1开始标识
                Data.append(0x01);
                if(m_Cal.m_SubsystemStatus==0)//分系统状态
                {
                    Data.append(0x01);
                }
                else if(m_Cal.m_SubsystemStatus==1)
                {
                    Data.append(0x02);
                }
                //Data.append(m_Cal.m_SubsystemStatus);//分系统状态

                int aa = m_Cal.m_NowAz*1000;
                QByteArray ee;
                ee.resize(sizeof(int));
                memcpy(ee.data(),&aa,sizeof(int));
                Data.append(ee,4);//当前方位角

                aa = m_Cal.m_NowEl*1000;
                ee.clear();
                ee.resize(sizeof(int));
                memcpy(ee.data(),&aa,sizeof(int));
                Data.append(ee,4);//当前俯仰角

                aa = m_Prodata.AZ*1000;
                ee.clear();
                ee.resize(sizeof(int));
                memcpy(ee.data(),&aa,sizeof(int));
                Data.append(ee,4);//程引方位角

                aa = m_Prodata.EL*1000;
                ee.clear();
                ee.resize(sizeof(int));
                memcpy(ee.data(),&aa,sizeof(int));
                Data.append(ee,4);//程引俯仰角

                Data.append(0xFE);
                Data.append(0x01);//单元1结束标识
                Data.append(0xFB);//单元2开始标识
                Data.append(0x02);
                if(flage==2)
                {
                    Data.append(0x01);//监控方式
                }
                else
                {
                   Data.append(0x02);//监控方式
                }
                Data.append(0x01);//天线工作方式
                Data.append(0x02);//与ADU通信状态
                Data.append(0x02);//与标校软件通信状态
                Data.append(0x01);//远程上电状态
                Data.append(0x01);//声音告警状态
                Data.append(0x01);//灯光告警状态

                aa = 0;
                ee.clear();
                ee.resize(sizeof(int));
                memcpy(ee.data(),&aa,sizeof(int));
                Data.append(ee,4);//功率探头接收信号强度

                Data.append(0xFE);
                Data.append(0x02);//单元2结束标识
                Data.append(0xFB);//单元3开始标识
                Data.append(0x03);
                Data.append(0x01);//驱动监控方式
                if(m_ACU.m_AzFirstDriver==0)
                {
                  Data.append(0x01);//方位驱动1状态(正常)
                }
                else
                {
                  Data.append(0x02);//方位驱动1状态(警告)
                }

                //Data.append(m_ACU.m_AzFirstDriver);//方位驱动1状态
                if(m_ACU.m_AzSecondDriver==0)
                {
                  Data.append(0x01);//方位驱动2状态(正常)
                }
                else
                {
                  Data.append(0x02);//方位驱动2状态(警告)
                }
                //Data.append(m_ACU.m_AzSecondDriver);//方位驱动2状态
                Data.append(0x01);//方位驱动器1加电
                Data.append(0x01);//方位驱动器2加电

                aa = 0;
                ee.clear();
                ee.resize(sizeof(int));
                memcpy(ee.data(),&aa,sizeof(int));
                Data.append(ee,4);//方位电机1电流


                aa = 0;
                ee.clear();
                ee.resize(sizeof(int));
                memcpy(ee.data(),&aa,sizeof(int));
                Data.append(ee,4);//方位电机2电流
                if(m_ACU.m_AzCW==0)
                {
                  Data.append(0x01);//方位顺限位(限位)
                }
                else if(m_ACU.m_AzCW==1)
                {
                  Data.append(0x02);//方位顺限位(未限位)
                }
                //Data.append(m_ACU.m_AzCW);//方位顺限位
                if(m_ACU.m_AzCCW==0)
                {
                  Data.append(0x01);//方位逆限位(限位)
                }
                else if(m_ACU.m_AzCCW==1)
                {
                  Data.append(0x02);//方位逆限位(未限位)
                }
                //Data.append(m_ACU.m_AzCCW);//方位逆限位
                if(m_ACU.m_ElFirstDriver==0)
                {
                  Data.append(0x01);//俯仰驱动状态(正常)
                }
                else
                {
                  Data.append(0x02);//俯仰驱动状态(警告)
                }
                //Data.append(m_ACU.m_ElFirstDriver);//俯仰驱动状态
                Data.append(0x01);//俯仰驱动器加电

                aa = 0;
                ee.clear();
                ee.resize(sizeof(int));
                memcpy(ee.data(),&aa,sizeof(int));
                Data.append(ee);//俯仰电机电流
                if(m_ACU.m_ElCW==0)
                {
                  Data.append(0x01);//俯仰上限位(限位)
                }
                else if(m_ACU.m_ElCW==1)
                {
                  Data.append(0x02);//俯仰上位(未限位)
                }
                //Data.append(m_ACU.m_ElCW);//俯仰上限位
                if(m_ACU.m_ElCCW==0)
                {
                  Data.append(0x01);//俯仰下限位(限位)
                }
                else if(m_ACU.m_ElCCW==1)
                {
                  Data.append(0x02);//俯仰下位(未限位)
                }
                //Data.append(m_ACU.m_ElCCW);//俯仰下限位
                Data.append(0x02);//天线收藏状态
                if(m_ACU.m_AzCodeDisk==0)
                {
                  Data.append(0x01);//方位码盘状态(正常)
                }
                else if(m_ACU.m_AzCodeDisk==1)
                {
                  Data.append(0x02);//方位码盘状态(警告)
                }
                //Data.append(m_ACU.m_AzCodeDisk);//方位码盘状态
                if(m_ACU.m_ElCodeDisk==0)
                {
                  Data.append(0x01);//俯仰码盘状态(正常)
                }
                else if(m_ACU.m_ElCodeDisk==1)
                {
                  Data.append(0x02);//俯仰码盘状态(警告)
                }
                //Data.append(m_ACU.m_ElCodeDisk);//俯仰码盘故障
                Data.append(0xFE);//单元3结束标识
                Data.append(0x03);
                size=Data.size();


                QByteArray FrameHead;//帧头
                FrameHead.append(SourceAdd+DestinaAdd);
                //信息类别
                FrameHead[8] = 0x04;
                FrameHead[9] = 0xF1;
                FrameHead[10] = 0x00;
                FrameHead[11] = 0x00;
                //保留
                FrameHead[12] = 0x00;
                FrameHead[13] = 0x00;
                FrameHead[14] = 0x00;
                FrameHead[15] = 0x00;
                //日期
                FrameHead[16] = 0x00;
                FrameHead[17] = 0x00;
                FrameHead[18]= 0x00;
                FrameHead[19]= 0x00;
                //时间
                FrameHead[20]= 0x00;
                FrameHead[21]= 0x00;
                FrameHead[22]= 0x00;
                FrameHead[23]= 0x00;
                //长度
//                    FrameHead[16]= 0x186;
//                    FrameHead[17]= 0x00;
//                    FrameHead[18]= 0x00;
//                    FrameHead[19]= 0x00;
                aa = size;
                ee.clear();
                ee.resize(sizeof(int));
                memcpy(ee.data(),&aa,sizeof(int));
                FrameHead.append(ee);

                QByteArray Record;//总数据
                Record = FrameHead+Data;
                m_Remotesocket->writeDatagram(Record.data(),Record.size(),m_settings.m_RemoteConIP,m_settings.m_RemoteConPort);

                //将QString AB转换为0Xab
//                    QString aa="AB";
//                    int v0;
//                    sscanf(aa.toUtf8(),"%X",&v0);
            }
        }
    }

}

/********************************************************
* acu状态解析
********************************************************/
void UdpThread::ACU_Analysis(QByteArray byte)
{
    bool bl;
    int tem;
    QString str = byte.mid(10,1);
    m_ACU.m_ACUState = str.toInt(&bl,16);

    str = byte.mid(9,1);
    tem=str.toInt(&bl,16);
    t = tem &0x1;
    m_ACU.m_AzVelState=t;
    t = int(tem &0x2)>>1;
    m_ACU.m_AzPosState=t;
    t = int(tem &0x4)>>2;
    m_ACU.m_ElVelState=t;
    t = int(tem &0x8)>>3;
    m_ACU.m_ElPosState=t;
    str = byte.mid(8,1);
    tem = str.toInt(&bl,16);
    t=tem &0x1;
    m_ACU.m_Tracksignal=t;
    t = int(tem &0x2)>>1;
    m_ACU.m_InstructionErr=t;
    t = int(tem &0x4)>>2;
    m_ACU.m_NotDo=t;
    t = int(tem &0x8)>>3;
    m_ACU.m_handset=t;

    str = byte.mid(7,1);
    tem = str.toInt(&bl,16);
    t=tem &0x1;
    m_ACU.m_Clockwise=t;
    t = int(tem &0x2)>>1;
    m_ACU.m_Anticlockwise=t;
    t = int(tem &0x4)>>2;
    m_ACU.m_TurnUp=t;
    t = int(tem &0x8)>>3;
    m_ACU.m_TurnDown=t;

///////////////////////////////////////////////////////////////////
    str=byte.mid(16,1);
    tem = str.toInt(&bl,16);

    t=tem&0x1;
    m_ACU.m_AzCW=t;
    t = int(tem &0x2)>>1;
    m_ACU.m_AzCCW=t;
    t = int(tem &0x4)>>2;
    m_ACU.m_AzPerCW=t;
    t = int(tem &0x8)>>3;
    m_ACU.m_AzPerCCW=t;

    str=byte.mid(15,1);
    tem = str.toInt(&bl,16);
    t=tem&0x1;
    m_ACU.m_AzEndLimit=t;
    t = int(tem &0x2)>>1;
    m_ACU.m_AzFirstDriver=t;
    t = int(tem &0x4)>>2;
    m_ACU.m_AzSecondDriver=t;
    t = int(tem &0x8)>>3;
    m_ACU.m_AzCodeDisk=t;

    str=byte.mid(14,1);
    tem = str.toInt(&bl,16);
    t=tem&0x1;
    m_ACU.m_AzStop=t;
    t = int(tem &0x2)>>1;
    t = int(tem &0x4)>>2;
    t = int(tem &0x8)>>3;
    m_ACU.m_AzPower=t;
/////////////////////////////////////////////////////////////
    str=byte.mid(22,1);
    tem = str.toInt(&bl,16);

    t=tem&0x1;
    m_ACU.m_ElCW=t;
    t = int(tem &0x2)>>1;
    m_ACU.m_ElCCW=t;
    t = int(tem &0x4)>>2;
    m_ACU.m_ElPerCW=t;
    t = int(tem &0x8)>>3;
    m_ACU.m_ElPerCCW=t;

    str=byte.mid(21,1);
    tem = str.toInt(&bl,16);
    t=tem&0x1;
    m_ACU.m_ElEndLimit=t;
    t = int(tem &0x2)>>1;
    m_ACU.m_ElFirstDriver=t;
    t = int(tem &0x4)>>2;
    m_ACU.m_ElSecondDriver=t;
    t = int(tem &0x8)>>3;
    m_ACU.m_ElCodeDisk=t;

    str=byte.mid(20,1);
    tem = str.toInt(&bl,16);
    t=tem&0x1;
    m_ACU.m_ElLock=t;
    t = int(tem &0x2)>>1;
    m_ACU.m_ElUnlock=t;
    t = int(tem &0x4)>>2;
    m_ACU.m_ElStow=t;
    t = int(tem &0x8)>>3;
    m_ACU.m_ElPower=t;

    str=byte.mid(24,8);
    m_ACU.m_NowAz=str.toDouble();

    m_ACU.m_NowAzVel = qAbs((m_ACU.m_NowAz -lastAZ ))/0.1;
    lastAZ = m_ACU.m_NowAz;
    str=byte.mid(33,8);
    m_ACU.m_NowEl=str.toDouble();
    m_ACU.m_NowElVel = qAbs((m_ACU.m_NowEl -lastEL ))/0.1;
    lastEL = m_ACU.m_NowEl;
    str=byte.mid(42,5);
    m_ACU.m_RXLEV =str.toDouble();


}

/********************************************************
* 驱动状态解析
********************************************************/
void UdpThread::Drive_Analysis(QByteArray byte)
{
/***********************************
 * 方位驱动 1 故障
 ***********************************/
    m_ACU.m_AzFirstDriverList.clear();
    bool bl;
    int tem;
    int data=0;
    QString str = byte.mid(11,1);
    tem=str.toInt(&bl,16);
    data+=tem;
    int t = tem &0x1;
    if(t==1)
    {
        m_ACU.m_AzFirstDriverList.append("方位码盘故障");
    }
    t = int(tem &0x2)>>1;
    if(t==1)
    {
        m_ACU.m_AzFirstDriverList.append("通信故障");
    }

    t = int(tem &0x4)>>2;
    if(t==1)
    {
        m_ACU.m_AzFirstDriverList.append("过电压");
    }
    t = int(tem &0x8)>>3;
    if(t==1)
    {
        m_ACU.m_AzFirstDriverList.append("速度反馈故障");
    }
    str = byte.mid(10,1);
    tem=str.toInt(&bl,16);
    data+=int(tem<<4);
    t = tem &0x1;
    if(t==1)
    {
        m_ACU.m_AzFirstDriverList.append("散热片跳闸");
    }
    t = int(tem &0x2)>>1;
    if(t==1)
    {
        m_ACU.m_AzFirstDriverList.append("过速");
    }

    t = int(tem &0x4)>>2;
    if(t==1)
    {
        m_ACU.m_AzFirstDriverList.append("丢失脉冲");
    }
    t = int(tem &0x8)>>3;
    if(t==1)
    {
        m_ACU.m_AzFirstDriverList.append("丢失脉冲");
    }
    str = byte.mid(9,1);
    tem=str.toInt(&bl,16);
    data+=int(tem<<8);
    t = tem &0x1;
    if(t==1)
    {
        m_ACU.m_AzFirstDriverList.append("过流跳闸");
    }
    t = int(tem &0x2)>>1;
    if(t==1)
    {
        m_ACU.m_AzFirstDriverList.append("其它故障");
    }

    t = int(tem &0x4)>>2;
    if(t==1)
    {
        m_ACU.m_AzFirstDriverList.append("交流互感器故障");
    }
    t = int(tem &0x8)>>3;
    if(t==1)
    {
        m_ACU.m_AzFirstDriverList.append("三相故障");
    }
    str = byte.mid(8,1);
    tem=str.toInt(&bl,16);
    data+=int(tem<<12);
    t = tem &0x1;
    if(t==1)
    {
        m_ACU.m_AzFirstDriverList.append("相位所定");
    }
    m_ACU.m_AzFirstDriver=data;
    //qDebug()<<m_ACU.m_AzFirstDriverList;

/***********************************
 * 方位驱动 2 故障
 ***********************************/
    m_ACU.m_AzSecondDriverList.clear();
    data=0;
    str = byte.mid(17,1);
    tem=str.toInt(&bl,16);
    data+=tem;
    t = tem &0x1;
    if(t==1)
    {
        m_ACU.m_AzSecondDriverList.append("方位码盘故障");
    }
    t = int(tem &0x2)>>1;
    if(t==1)
    {
        m_ACU.m_AzSecondDriverList.append("通信故障");
    }

    t = int(tem &0x4)>>2;
    if(t==1)
    {
        m_ACU.m_AzSecondDriverList.append("过电压");
    }
    t = int(tem &0x8)>>3;
    if(t==1)
    {
        m_ACU.m_AzSecondDriverList.append("速度反馈故障");
    }
    str = byte.mid(16,1);
    tem=str.toInt(&bl,16);
    data+=int(tem<<4);
    t = tem &0x1;
    if(t==1)
    {
        m_ACU.m_AzSecondDriverList.append("散热片跳闸");
    }
    t = int(tem &0x2)>>1;
    if(t==1)
    {
        m_ACU.m_AzSecondDriverList.append("过速");
    }

    t = int(tem &0x4)>>2;
    if(t==1)
    {
        m_ACU.m_AzSecondDriverList.append("丢失脉冲");
    }
    t = int(tem &0x8)>>3;
    if(t==1)
    {
        m_ACU.m_AzSecondDriverList.append("丢失脉冲");
    }
    str = byte.mid(15,1);
    tem=str.toInt(&bl,16);
    data+=int(tem<<8);
    t = tem &0x1;
    if(t==1)
    {
        m_ACU.m_AzSecondDriverList.append("过流跳闸");
    }
    t = int(tem &0x2)>>1;
    if(t==1)
    {
        m_ACU.m_AzSecondDriverList.append("其它故障");
    }

    t = int(tem &0x4)>>2;
    if(t==1)
    {
        m_ACU.m_AzSecondDriverList.append("交流互感器故障");
    }
    t = int(tem &0x8)>>3;
    if(t==1)
    {
        m_ACU.m_AzSecondDriverList.append("三相故障");
    }
    str = byte.mid(14,1);
    tem=str.toInt(&bl,16);
    data+=int(tem<<12);
    t = tem &0x1;
    if(t==1)
    {
        m_ACU.m_AzSecondDriverList.append("相位所定");
    }
    m_ACU.m_AzSecondDriver=data;
    //qDebug()<<m_ACU.m_AzSecondDriverList;


/***********************************
 * 俯仰驱动 1 故障
 ***********************************/
    m_ACU.m_ElFirstDriverList.clear();
    data=0;
    str = byte.mid(23,1);
    tem=str.toInt(&bl,16);
    data+=tem;
    t = tem &0x1;
    if(t==1)
    {
        m_ACU.m_ElFirstDriverList.append("方位码盘故障");
    }
    t = int(tem &0x2)>>1;
    if(t==1)
    {
        m_ACU.m_ElFirstDriverList.append("通信故障");
    }

    t = int(tem &0x4)>>2;
    if(t==1)
    {
        m_ACU.m_ElFirstDriverList.append("过电压");
    }
    t = int(tem &0x8)>>3;
    if(t==1)
    {
        m_ACU.m_ElFirstDriverList.append("速度反馈故障");
    }
    str = byte.mid(22,1);
    tem=str.toInt(&bl,16);
    data+=int(tem<<4);
    t = tem &0x1;
    if(t==1)
    {
        m_ACU.m_ElFirstDriverList.append("散热片跳闸");
    }
    t = int(tem &0x2)>>1;
    if(t==1)
    {
        m_ACU.m_ElFirstDriverList.append("过速");
    }

    t = int(tem &0x4)>>2;
    if(t==1)
    {
        m_ACU.m_ElFirstDriverList.append("丢失脉冲");
    }
    t = int(tem &0x8)>>3;
    if(t==1)
    {
        m_ACU.m_ElFirstDriverList.append("丢失脉冲");
    }
    str = byte.mid(21,1);
    tem=str.toInt(&bl,16);
    data+=int(tem<<8);
    t = tem &0x1;
    if(t==1)
    {
        m_ACU.m_ElFirstDriverList.append("过流跳闸");
    }
    t = int(tem &0x2)>>1;
    if(t==1)
    {
        m_ACU.m_ElFirstDriverList.append("其它故障");
    }

    t = int(tem &0x4)>>2;
    if(t==1)
    {
        m_ACU.m_ElFirstDriverList.append("交流互感器故障");
    }
    t = int(tem &0x8)>>3;
    if(t==1)
    {
        m_ACU.m_ElFirstDriverList.append("三相故障");
    }
    str = byte.mid(20,1);
    tem=str.toInt(&bl,16);
    data+=int(tem<<12);
    t = tem &0x1;
    if(t==1)
    {
        m_ACU.m_ElFirstDriverList.append("相位所定");
    }
    m_ACU.m_ElFirstDriver=data;
    //qDebug()<<m_ACU.m_ElFirstDriverList;

/***********************************
 * 俯仰驱动 2 故障
 ***********************************/
    m_ACU.m_ElSecondDriverList.clear();
    data=0;
    str = byte.mid(29,1);
    tem=str.toInt(&bl,16);
    data+=tem;
    t = tem &0x1;
    if(t==1)
    {
        m_ACU.m_ElSecondDriverList.append("方位码盘故障");
    }
    t = int(tem &0x2)>>1;
    if(t==1)
    {
        m_ACU.m_ElSecondDriverList.append("通信故障");
    }

    t = int(tem &0x4)>>2;
    if(t==1)
    {
        m_ACU.m_ElSecondDriverList.append("过电压");
    }
    t = int(tem &0x8)>>3;
    if(t==1)
    {
        m_ACU.m_ElSecondDriverList.append("速度反馈故障");
    }
    str = byte.mid(28,1);
    tem=str.toInt(&bl,16);
    data+=int(tem<<4);
    t = tem &0x1;
    if(t==1)
    {
        m_ACU.m_ElSecondDriverList.append("散热片跳闸");
    }
    t = int(tem &0x2)>>1;
    if(t==1)
    {
        m_ACU.m_ElSecondDriverList.append("过速");
    }

    t = int(tem &0x4)>>2;
    if(t==1)
    {
        m_ACU.m_ElSecondDriverList.append("丢失脉冲");
    }
    t = int(tem &0x8)>>3;
    if(t==1)
    {
        m_ACU.m_ElSecondDriverList.append("丢失脉冲");
    }
    str = byte.mid(27,1);
    tem=str.toInt(&bl,16);
    data+=int(tem<<8);
    t = tem &0x1;
    if(t==1)
    {
        m_ACU.m_ElSecondDriverList.append("过流跳闸");
    }
    t = int(tem &0x2)>>1;
    if(t==1)
    {
        m_ACU.m_ElSecondDriverList.append("其它故障");
    }

    t = int(tem &0x4)>>2;
    if(t==1)
    {
        m_ACU.m_ElSecondDriverList.append("交流互感器故障");
    }
    t = int(tem &0x8)>>3;
    if(t==1)
    {
        m_ACU.m_ElSecondDriverList.append("三相故障");
    }
    str = byte.mid(26,1);
    tem=str.toInt(&bl,16);
    data+=int(tem<<12);
    t = tem &0x1;
    if(t==1)
    {
        m_ACU.m_ElSecondDriverList.append("相位所定");
    }
    m_ACU.m_ElSecondDriver=data;
    //qDebug()<<m_ACU.m_ElSecondDriverList;

}
/********************************************************
* 极化状态解析
********************************************************/
void UdpThread::Pal_Analysis(QByteArray byte)
{
    bool bl;
    int tem;
    QString str = byte.mid(8,1);
    tem=str.toInt(&bl,16);
    int t = tem &0x1;
    m_ACU.m_CommunicationState=t;
    t = int(tem &0x2)>1;
    m_ACU.m_InstructionErr=t;
    t = int(tem &0x4)>2;
    m_ACU.m_NotDo=t;
    t = int(tem &0x8)>3;
    m_ACU.m_handset=t;

    str = byte.mid(7,1);
    m_ACU.m_Pol=str.toInt();

    //极化一位置
    if(m_ACU.m_Pol==1)
    {
        str = byte.mid(10,6);
        m_ACU.m_PolPos=str.toDouble();
    }
    //极化二位置
    if(m_ACU.m_Pol==2)
    {
       str = byte.mid(17,6);
       m_ACU.m_PolPos=str.toDouble();
    }
    //极化三位置
    if(m_ACU.m_Pol==3)
    {
       str = byte.mid(24,6);
       m_ACU.m_PolPos=str.toDouble();
    }
    //极化四位置
    if(m_ACU.m_Pol==4)
    {
       str = byte.mid(31,6);
       m_ACU.m_PolPos=str.toDouble();
    }
    if(m_ACU.m_Pol==1)
    {
        str = byte.mid(39,1);
        tem=str.toInt(&bl,16);
        t = tem &0x1;
        m_ACU.m_PolSwitchCW=t;
        t = int(tem &0x2)>1;
        m_ACU.m_PolSwitchCCW=t;
        t = int(tem &0x4)>2;
        m_ACU.m_PolSoftwareCW=t;
        t = int(tem &0x8)>3;
        m_ACU.m_PolSoftwareCCW=t;

        str = byte.mid(38,1);
        tem=str.toInt(&bl,16);
        t = tem &0x1;
        m_ACU.m_PolPosState=t;
        t = int(tem &0x2)>1;
        m_ACU.m_PolControlState=t;
        t = int(tem &0x4)>2;
        m_ACU.m_PolClockwise=t;
        t = int(tem &0x8)>3;
        m_ACU.m_PolAnticlockwise=t;
    }
    if(m_ACU.m_Pol==2)
    {
        str = byte.mid(41,1);
        tem=str.toInt(&bl,16);
        t = tem &0x1;
        m_ACU.m_PolSwitchCW=t;
        t = int(tem &0x2)>1;
        m_ACU.m_PolSwitchCCW=t;
        t = int(tem &0x4)>2;
        m_ACU.m_PolSoftwareCW=t;
        t = int(tem &0x8)>3;
        m_ACU.m_PolSoftwareCCW=t;

        str = byte.mid(40,1);
        tem=str.toInt(&bl,16);
        t = tem &0x1;
        m_ACU.m_PolPosState=t;
        t = int(tem &0x2)>1;
        m_ACU.m_PolControlState=t;
        t = int(tem &0x4)>2;
        m_ACU.m_PolClockwise=t;
        t = int(tem &0x8)>3;
        m_ACU.m_PolAnticlockwise=t;
    }
    if(m_ACU.m_Pol==3)
    {
        str = byte.mid(43,1);
        tem=str.toInt(&bl,16);
        t = tem &0x1;
        m_ACU.m_PolSwitchCW=t;
        t = int(tem &0x2)>1;
        m_ACU.m_PolSwitchCCW=t;
        t = int(tem &0x4)>2;
        m_ACU.m_PolSoftwareCW=t;
        t = int(tem &0x8)>3;
        m_ACU.m_PolSoftwareCCW=t;

        str = byte.mid(42,1);
        tem=str.toInt(&bl,16);
        t = tem &0x1;
        m_ACU.m_PolPosState=t;
        t = int(tem &0x2)>1;
        m_ACU.m_PolControlState=t;
        t = int(tem &0x4)>2;
        m_ACU.m_PolClockwise=t;
        t = int(tem &0x8)>3;
        m_ACU.m_PolAnticlockwise=t;
    }
    if(m_ACU.m_Pol==4)
    {
        str = byte.mid(44,1);
        tem=str.toInt(&bl,16);
        t = tem &0x1;
        m_ACU.m_PolSwitchCW=t;
        t = int(tem &0x2)>1;
        m_ACU.m_PolSwitchCCW=t;
        t = int(tem &0x4)>2;
        m_ACU.m_PolSoftwareCW=t;
        t = int(tem &0x8)>3;
        m_ACU.m_PolSoftwareCCW=t;

        str = byte.mid(43,1);
        tem=str.toInt(&bl,16);
        t = tem &0x1;
        m_ACU.m_PolPosState=t;
        t = int(tem &0x2)>1;
        m_ACU.m_PolControlState=t;
        t = int(tem &0x4)>2;
        m_ACU.m_PolClockwise=t;
        t = int(tem &0x8)>3;
        m_ACU.m_PolAnticlockwise=t;
    }

}
/********************************************************
* 接收机状态解析
********************************************************/
void UdpThread::Rec_Analysis(QByteArray byte)
{
    bool bl;
    int tem;
    QString str = byte.mid(8,1);
    tem=str.toInt(&bl,16);
    int t = tem &0x1;
    m_ACU.m_CommunicationState=t;
    t = int(tem &0x2)>1;
    m_ACU.m_signalLock=t;
    t = int(tem &0x4)>2;
    m_ACU.m_CodeType=t;
    t = int(tem &0x8)>3;
    m_ACU.m_ControlType=t;

    str= byte.mid(10,6);
    m_ACU.m_Signal=str.toDouble();
    str=byte.mid(17,6);
    m_ACU.m_AzErrorSignal=str.toDouble();
    str= byte.mid(24,6);
    m_ACU.m_ElErrorSignal=str.toDouble();

}
/********************************************************
* 标校状态解析(上报)
********************************************************/
void UdpThread::Cal_State(QByteArray byte)
 {

    bool bl;
    int tem;
    QByteArray Arr = byte.mid(12,4);
    std::reverse(Arr.begin(), Arr.end());//高低字节互换
    tem = Arr.toHex().toInt(&bl,16);
    m_Cal.m_DataSize=tem;

    Arr = byte.mid(16,4);
    std::reverse(Arr.begin(), Arr.end());
    tem=Arr.toHex().toInt(&bl,16);
    m_Cal.m_SerialNumber=tem;

    Arr = byte.mid(20,1);
    tem = Arr.toHex().toInt(&bl,16);
    m_Cal.m_SubsystemStatus=tem;
    Arr = byte.mid(21,1);
    tem = Arr.toHex().toInt(&bl,16);
    m_Cal.m_ControlMode=tem;
    Arr = byte.mid(22,1);
    tem = Arr.toHex().toInt(&bl,16);
    m_Cal.m_WorkMode=tem;

}
/********************************************************
* 标校轨道预报解析(响应)
********************************************************/
void UdpThread::Cal_Track(QByteArray byte)
{
    bool bl;
    int tem=0;
    QByteArray Arr = byte.mid(16,1);
    tem = Arr.toInt(&bl,16);
    m_Cal.m_TrackPositionResult=tem;
    tem=0;
    double result=0;
    for(int i=17;i<817;i=i+8)
    {
        Arr = byte.mid(i,8);
        memcpy(&result,Arr.data(),8);
        m_Cal.m_TrackAZ[tem]=result;
        tem++;
    }
    tem=0;
    for(int i = 817;i<1617;i=i+8)
    {
        Arr = byte.mid(i,8);
        memcpy(&result,Arr.data(),8);
        m_Cal.m_TrackEL[tem]=Arr.toHex().toInt(&bl,16);
        tem++;
    }
//    Arr = byte.mid(18,8);
//    m_Cal.m_TrackAZ=Arr.toDouble();
//    Arr = byte.mid(27,8);
//    m_Cal.m_TrackEL=Arr.toDouble();

}
/********************************************************
* 标校星位引导解析
********************************************************/
void UdpThread::Cal_Asgui(QByteArray byte)
{

    bool bl;
    int tem;
    QString str = byte.mid(12,4);
    QString time="";
    tem = str.toInt(&bl,16);
    m_Cal.m_DataSize=tem;

    QByteArray Arr = byte.mid(16,4);
    std::reverse(Arr.begin(), Arr.end());
    tem = Arr.toHex().toInt(&bl,16);
    QString year = QString::number(tem);

    Arr = byte.mid(20,4);
    std::reverse(Arr.begin(), Arr.end());
    tem = Arr.toHex().toInt(&bl,16);
    QString month = QString::number(tem);

    Arr = byte.mid(24,4);
    std::reverse(Arr.begin(), Arr.end());
    tem = Arr.toHex().toInt(&bl,16);
    QString day = QString::number(tem);

    Arr = byte.mid(28,4);
    std::reverse(Arr.begin(), Arr.end());
    tem = Arr.toHex().toInt(&bl,16);
    QString hour = QString::number(tem);

    Arr = byte.mid(32,4);
    std::reverse(Arr.begin(), Arr.end());
    tem = Arr.toHex().toInt(&bl,16);
    QString min = QString::number(tem);

    Arr = byte.mid(36,4);
    std::reverse(Arr.begin(), Arr.end());
    tem = Arr.toHex().toInt(&bl,16);
    QString sec = QString::number(tem);

    Arr = byte.mid(40,4);
    std::reverse(Arr.begin(), Arr.end());
    tem = Arr.toHex().toInt(&bl,16);
    QString msec = QString::number(tem);
    QString Time = year+"-"+month+"-"+day+" "+hour+":"+min+":"+sec+"."+msec;
    m_Cal.m_AstralTime=QDateTime::fromString(Time,"yyyy-MM-dd hh:mm:ss.zzz");

    Arr = byte.mid(44,8);
    //std::reverse(Arr.begin(), Arr.end());
    double result=0;
    memcpy(&result,Arr.data(),8);
    m_Cal.m_AstralNowAz=result;
    QString num= QString("%1").arg(m_Cal.m_AstralNowAz,7,'f',3,QChar('0'));
    QString data = QString("<000/04_0,+%0\r").arg(num);
    myCom->write(data.toUtf8(),data.size());

    Arr = byte.mid(52,8);
    //std::reverse(Arr.begin(), Arr.end());
    result=0;
    memcpy(&result,Arr.data(),8);
    m_Cal.m_AstralNowEl=result;
    num= QString("%1").arg(m_Cal.m_AstralNowEl,7,'f',3,QChar('0'));
    data = QString("<000/04_1,+%0\r").arg(num);

}
/********************************************************
* 标校射电源解析(响应)
********************************************************/
void UdpThread::Cal_RadioSource(QByteArray byte)
{
    int i=18;
    while(i<=byte.size())
    {
        std::string str = byte.mid(i,60).toStdString();
        m_Cal.m_FileName.append(QString::fromStdString(str));
        i+=60;
    }
}
/********************************************************
* 标校指向控制解析(响应)
********************************************************/
void UdpThread::Cal_sweep(QByteArray byte)
{
    bool bl;
    int tem;
    QByteArray Arr = byte.mid(16,1);
    tem = Arr.toHex().toInt(&bl,16);
    m_Cal.m_PositionResult = tem;
}
/********************************************************
* 标校指向控制解析(上报)
********************************************************/
void UdpThread::Cal_sweepReport(QByteArray byte)
{
    bool bl;
    int tem;
    QByteArray Arr = byte.mid(16,1);
    tem = Arr.toHex().toInt(&bl,16);
    m_Cal.m_SweepState = tem;
    Arr = byte.mid(17,1);
    tem = Arr.toHex().toInt(&bl,16);
    m_Cal.m_SweepNumber=tem;
    Arr = byte.mid(18,1);
    tem = Arr.toHex().toInt(&bl,16);
    m_Cal.m_SweepCalNumber=tem;
    Arr = byte.mid(19,1);
    tem = Arr.toHex().toInt(&bl,16);
    m_Cal.m_SweepNowNumber=tem;

    QString Name = byte.mid(20,10);
    m_Cal.m_NowName = Name;
    //标校状态为正在标校时，才会返回时间
    if(m_Cal.m_SweepState ==4)
    {
       //当前标校点开始日期
        QByteArray Arr = byte.mid(30,4);
        tem = Arr.toHex().toInt(&bl,16);
        m_Cal.m_NowSpotBeginDate=QString::number(tem);
       //当前标校点开始时间
        Arr = byte.mid(34,4);
        std::reverse(Arr.begin(), Arr.end());
        tem = Arr.toHex().toInt(&bl,16);
        int hour = tem/(60*60*10000);
        int min = (tem-(hour*60*60*10000))/(60*10000);
        int sec = (tem-(hour*60*60*10000)-(min*60*10000))/10000;
        int msec  = (tem-(hour*60*60*10000)-(min*60*10000)-(sec*10000))/10;
        m_Cal.m_NowSpotBeginTime=QString::number(hour)+QString::number(min)+QString::number(sec)+QString::number(msec);
        QString Time = m_Cal.m_NowSpotBeginDate+m_Cal.m_NowSpotBeginTime;
        m_Cal.m_PositionBeginTime = QDateTime::fromString(Time,"yyyyMMddhhmmsszzz");
       //当前标校点结束日期
        Arr = byte.mid(38,4);
        tem = Arr.toHex().toInt(&bl,16);
        m_Cal.m_NowSpotEndDate=QString::number(tem);

       //当前标校点结束日期
        Arr = byte.mid(42,4);
        std::reverse(Arr.begin(), Arr.end());
        tem = Arr.toHex().toInt(&bl,16);
        hour = tem/(60*60*10000);
        min = (tem-(hour*60*60*10000))/(60*10000);
        sec = (tem-(hour*60*60*10000)-(min*60*10000))/10000;
        msec  = (tem-(hour*60*60*10000)-(min*60*10000)-(sec*10000))/10;
        m_Cal.m_NowSpotEndTime=QString::number(hour)+QString::number(min)+QString::number(sec)+QString::number(msec);
        Time = m_Cal.m_NowSpotEndDate+m_Cal.m_NowSpotEndTime;
        m_Cal.m_PositionEndTime = QDateTime::fromString(Time,"yyyyMMddhhmmsszzz");
    }
}

void UdpThread::initSerial()
{
    myCom = new Win_QextSerialPort("COM9", QextSerialBase::EventDriven);

    // 以读写方式打开串口
    bool a =myCom->open(QIODevice::ReadWrite);
    // 波特率设置
    myCom->setBaudRate(BAUD19200);

    // 数据位设置，我们设置为8位数据位
    myCom->setDataBits(DATA_8);

    // 奇偶校验设置，我们设置为无校验
    myCom->setParity(PAR_NONE);

    // 停止位设置，我们设置为1位停止位
    myCom->setStopBits(STOP_1);

    // 数据流控制设置，我们设置为无数据流控制
    myCom->setFlowControl(FLOW_OFF);

    // 延时设置，我们设置为延时500ms,这个在Windows下好像不起作用
//    myCom->setTimeout(500);

    // 信号和槽函数关联，当串口缓冲区有数据时，进行读串口操作

    connect(myCom, SIGNAL(readyRead()), this, SLOT(RecvData()));
}

QByteArray UdpThread::intToByte(int i, int size, int type)
{
    QString A = QString::number(i);
    int v0;
    const char AA='0xab';
    sscanf(&AA,"%d",&v0);
    QByteArray abyte0;
    if(type==0)
    {
        if(size==4)
        {
            abyte0.resize(size);
            abyte0[0] = (uchar)((v0>>12 )& 0x0f);
            abyte0[1] = (uchar)((v0>>8) & 0x0f);
            abyte0[2] = (uchar)((v0>>4) & 0x0f);
            abyte0[3] = (uchar)((v0) & 0x0f);
        }
        else if(size == 2)
        {
            abyte0.resize(size);
            abyte0[0] = (uchar)((v0>>12 )& 0x0f);
            abyte0[1] = (uchar)((v0>>8) & 0x0f);
        }
        else if(size == 8)
        {
            abyte0.resize(size);
            abyte0[0] = (uchar)((v0>>28)& 0x0f);
            abyte0[1] = (uchar)((v0>>24) & 0x0f);
            abyte0[2] = (uchar)((v0>>20) & 0x0f);
            abyte0[3] = (uchar)((v0>>16) & 0x0f);
            abyte0[4] = (uchar)((v0>>12 )& 0x0f);
            abyte0[5] = (uchar)((v0>>8) & 0x0f);
            abyte0[6] = (uchar)((v0>>4) & 0x0f);
            abyte0[7] = (uchar)((v0) & 0x0f);
        }
        return abyte0;
    }
    else if(type==1)
    {
        if(size==4)
        {
            abyte0.resize(size);
            abyte0[0] =(uchar)((v0) & 0x0f);
            abyte0[1] =(uchar)((v0>>4) & 0x0f);
            abyte0[2] =(uchar)((v0>>8) & 0x0f);
            abyte0[3] =(uchar)((v0>>12 )& 0x0f);
        }
        else if(size == 2)
        {
            abyte0.resize(size);
            abyte0[0] = (uchar)((v0>>8) & 0x0f);
            abyte0[1] = (uchar)((v0>>12 )& 0x0f);
        }
        else if(size == 8)
        {
            abyte0.resize(size);
            abyte0[0] = (uchar)((v0)& 0x0f);
            abyte0[1] = (uchar)((v0>>4) & 0x0f);
            abyte0[2] = (uchar)((v0>>8) & 0x0f);
            abyte0[3] = (uchar)((v0>>12) & 0x0f);
            abyte0[4] = (uchar)((v0>>16 )& 0x0f);
            abyte0[5] = (uchar)((v0>>20) & 0x0f);
            abyte0[6] = (uchar)((v0>>24) & 0x0f);
            abyte0[7] = (uchar)((v0>>28) & 0x0f);
        }
        return abyte0;
    }
    else if(type==2)
    {
        if(size==2)
        {
            abyte0.resize(size);
            abyte0[0]=(uchar)((v0>>8) & 0xff);
            abyte0[1]=(uchar)((v0) & 0x00ff);
        }
        else if(size ==4)
        {
            abyte0.resize(size);
            abyte0[0]=(uchar)((v0>>24) & 0xff);
            abyte0[1]=(uchar)((v0>>16) & 0xff);
            abyte0[2]=(uchar)((v0>>8) & 0xff);
            abyte0[3]=(uchar)((v0) & 0x00ff);
        }
    }

}

void UdpThread::initSend()
{
    timer = new QTimer;
    timer->start(1000);
    connect(timer,&QTimer::timeout,[=](){
        if(Type==2&&flage==1)
        {
            if(az-m_ACU.m_NowAz<=0.002&&az-m_ACU.m_NowAz>=-0.002&&el-m_ACU.m_NowEl<=0.002&&el-m_ACU.m_NowEl>=-0.002)
            {
                QByteArray Data;
                QByteArray FrameHead;
                QByteArray Record;
                Data[0]= 0x02;
                Data[1]= 0x41;
                //指向状态
                Data[2]=0x01;
                size=Data.size();

                FrameHead.clear();
                FrameHead.append(SourceAdd+DestinaAdd);
                //信息类别
                FrameHead[8] = 0x01;
                FrameHead[9] = 0xF1;
                FrameHead[10] = 0x00;
                FrameHead[11] = 0x00;
                //保留
                FrameHead[12] = 0x00;
                FrameHead[13] = 0x00;
                FrameHead[14] = 0x00;
                FrameHead[15] = 0x00;
                //日期
                FrameHead[16] = 0x00;
                FrameHead[17] = 0x00;
                FrameHead[18]= 0x00;
                FrameHead[19]= 0x00;
                //时间
                FrameHead[20]= 0x00;
                FrameHead[21]= 0x00;
                FrameHead[22]= 0x00;
                FrameHead[23]= 0x00;
                //长度
                int aa = size;
                QByteArray ee;
                ee.resize(sizeof(int));
                memcpy(ee.data(),&aa,sizeof(int));
                FrameHead.append(ee);

                Record.clear();//总数据
                Record = FrameHead+Data;
                qDebug()<<"send is "<<Record;
                m_Remotesocket->writeDatagram(Record.data(),Record.size(),m_settings.m_RemoteConIP,m_settings.m_RemoteConPort);
                Type=0;
            }
        }
        else if(Type==3&&flage==2)
        {
            QString ProFileName=datagram.mid(32,60);
            qDebug()<<ProFileName;
            QDir dir(m_settings.m_FtpPath);
            QStringList Files = dir.entryList(QStringList()<<ProFileName,QDir::Files|QDir::Readable,QDir::Name);
            foreach (QString temp, Files) {
                if(temp==ProFileName)
                {
                    qDebug()<<"远控程引";
                    QFile file(m_settings.m_FtpPath+"/"+ProFileName);
                    qDebug()<<m_settings.m_FtpPath+"/"+ProFileName;
                    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                        proType=1;
                        return;
                    }

                    QDateTime temp,Protime;
                    QDateTime now = QDateTime::currentDateTime();
                    double  max=0,tem=0,min=0,tem1=0;
                    int i = 0;
                    int j =0;
                    m_Prodatanode.clear();
                    while(!file.atEnd()) {

                        QString Pro = file.readLine();
                        if(i>=1)//第一次为数据头 不截取
                        {
                             QStringList aa= Pro.split('\t');
                             m_Prodata.Time = UTCTOLocalTime(QDateTime::fromString(aa[1],"yyyy-MM-dd'T'hh:mm:ss"));
                            if(m_Prodata.Time>now)
                            {
                                PAZ= aa[2].toDouble();
                                m_Prodata.AZ=PAZ;
                                //ui->label_PAZ->setText(QString::number(PAZ));
                                //每一次的俯仰角度
                                PEL= aa[3].toDouble();
                                m_Prodata.EL=PEL;
                                if(j==0)//第一次的数据直接发送
                                {
                                    m_Prodata.AZAngle=0;
                                    m_Prodata.ELAngle=0;
                                    m_Prodata.FirstAZ=PAZ;
                                    m_Prodata.FirstEL=PEL;
                                    tem=PAZ;
                                    tem1=PEL;
                                    temp=m_Prodata.Time;
                                    max=PEL;
                                    min=PEL;
                                    m_Prodatanode.push_back(m_Prodata);
                                }
                                else if(j>0)//后面数据先解析再发送
                                {
                                    int bb=temp.secsTo(m_Prodata.Time);
                                    m_Prodata.number=(bb*1000)/20;
                                    for(int i = 0;i<m_Prodata.number;i++)
                                    {
                                        temp=temp.addMSecs(20);
                                        m_Prodata.Time=temp;
                                        m_Prodata.AZAngle+=((PAZ-tem)/(bb*1000))*20;
                                        m_Prodata.ELAngle+=((PEL-tem1)/(bb*1000))*20;
                                        m_Prodatanode.push_back(m_Prodata);
                                    }
                                    tem=PAZ;
                                    tem1=PEL;
                                }
                                j++;
                            }
                        }
                        i++;
                    }
                    if(m_Prodatanode.size()==0)
                    {
                        proType=2;
                        Type=0;
                        ProsendState=true;
                    }
                    else
                    {
                        proType=1;
                        ProsendState=true;
                        ProThread->start();
                    }
                }
            }
            QByteArray Data;
            //标识
            Data[0]= 0x03;
            Data[1]= 0x41;
            //程引状态
            Data[2]=proType;
            //偏置状态
            Data[3]=0x01;
            //方位偏置
            int aa = 0;
            QByteArray ee;
            ee.resize(sizeof(int));
            memcpy(ee.data(),&aa,sizeof(int));
            Data.append(ee);
            //俯仰偏置
            aa = 0;
            ee.clear();
            ee.resize(sizeof(int));
            memcpy(ee.data(),&aa,sizeof(int));
            Data.append(ee);
            //时间偏置
            aa = 0;
            ee.clear();
            ee.resize(sizeof(int));
            memcpy(ee.data(),&aa,sizeof(int));
            Data.append(ee);
            size = Data.size();

            QByteArray FrameHead;
            FrameHead.append(SourceAdd+DestinaAdd);
            //信息类别
            FrameHead[8] = 0x01;
            FrameHead[9] = 0xF1;
            FrameHead[10] = 0x00;
            FrameHead[11] = 0x00;
            //保留
            FrameHead[12] = 0x00;
            FrameHead[13] = 0x00;
            FrameHead[14] = 0x00;
            FrameHead[15] = 0x00;
            //日期
            FrameHead[16] = 0x00;
            FrameHead[17] = 0x00;
            FrameHead[18]= 0x00;
            FrameHead[19]= 0x00;
            //时间
            FrameHead[20]= 0x00;
            FrameHead[21]= 0x00;
            FrameHead[22]= 0x00;
            FrameHead[23]= 0x00;
            //长度
            aa = size;
            ee.clear();
            ee.resize(sizeof(int));
            memcpy(ee.data(),&aa,sizeof(int));
            FrameHead.append(ee);


            QByteArray Record;//总数据
            Record = FrameHead+Data;
            m_Remotesocket->writeDatagram(Record.data(),Record.size(),m_settings.m_RemoteConIP,m_settings.m_RemoteConPort);
            if(m_Prodatanode.size()==0)
            {
                Type=0;
            }
        }
        else if(Type==4&&flage==1)
        {
             if(170-m_ACU.m_NowAz<0.001&&170-m_ACU.m_NowAz>-0.001&&88-m_ACU.m_NowEl<0.001&&88-m_ACU.m_NowEl>-0.001)
             {
                 QByteArray Data;
                 //接收到的信息类别
//                        Data[0]= 0x01;
//                        Data[1]= 0xF0;
//                        Data[2]= 0x00;
//                        Data[3]= 0x00;
                 //标识
                 Data[0]= 0x04;
                 Data[1]= 0x41;
                 //收藏状态
                 Data[2]=0x02;
                 size = Data.size();

                 QByteArray FrameHead;
                 FrameHead.clear();
                 FrameHead.append(SourceAdd+DestinaAdd);
                 //信息类别
                 FrameHead[8] = 0x01;
                 FrameHead[9] = 0xF1;
                 FrameHead[10] = 0x00;
                 FrameHead[11] = 0x00;
                 //保留
                 FrameHead[12] = 0x00;
                 FrameHead[13] = 0x00;
                 FrameHead[14] = 0x00;
                 FrameHead[15] = 0x00;
                 //日期
                 FrameHead[16] = 0x00;
                 FrameHead[17] = 0x00;
                 FrameHead[18]= 0x00;
                 FrameHead[19]= 0x00;
                 //时间
                 FrameHead[20]= 0x00;
                 FrameHead[21]= 0x00;
                 FrameHead[22]= 0x00;
                 FrameHead[23]= 0x00;
                 //长度
                 int aa = size;
                 QByteArray  ee;
                 ee.resize(sizeof(int));
                 memcpy(ee.data(),&aa,sizeof(int));
                 FrameHead.append(ee);


                 QByteArray Record;//总数据
                 Record = FrameHead+Data;
                 m_Remotesocket->writeDatagram(Record.data(),Record.size(),m_settings.m_RemoteConIP,m_settings.m_RemoteConPort);
                 Type=0;
             }
        }
        else if(Type==5&&flage==1)
        {
            if(m_ACU.m_NowAzVel>=-1&&m_ACU.m_NowAzVel<=1&&m_ACU.m_NowElVel>=-1&&m_ACU.m_NowElVel<=1)
            {
                if(m_ACU.m_NowAzVel*1000>=AZmax)
                {
                    AZmax=m_ACU.m_NowAzVel*1000;
                    qDebug()<<"AZmax"<<AZmax;
                    AZmaxadd=m_ACU.m_NowAzVel*1000;
                }
                if(m_ACU.m_NowElVel*1000>=ELmax)
                {
                    ELmax=m_ACU.m_NowElVel*1000;
                    ELmaxadd=m_ACU.m_NowElVel*1000;
                }
            }
            if(PerTestFlage==true)
            {
                QString num ,data;
                num= QString("%1").arg(AZangle,7,'f',3,QChar('0'));
                data = QString("<000/04_0,+%0\r").arg(num);
                myCom->write(data.toUtf8(),data.size());
                num= QString("%1").arg(ELangle,7,'f',3,QChar('0'));
                data = QString("<000/04_1,+%0\r").arg(num);
                myCom->write(data.toUtf8(),data.size());
                QByteArray Data;
                //标识
                Data[0]= 0x05;
                Data[1]= 0x41;
                //测试状态
                Data[2]=0x02;
                //方位最大速度
                int aa = AZmax;
                QByteArray ee;
                ee.resize(sizeof(int));
                memcpy(ee.data(),&aa,sizeof(int));
                Data.append(ee);
                //俯仰最大速度
                aa = ELmax;
                ee.clear();
                ee.resize(sizeof(int));
                memcpy(ee.data(),&aa,sizeof(int));
                Data.append(ee);
                //方位最大加速度
                aa = AZmaxadd;
                ee.clear();
                ee.resize(sizeof(int));
                memcpy(ee.data(),&aa,sizeof(int));
                Data.append(ee);
                //俯仰最大加速度
                aa = ELmaxadd;
                ee.clear();
                ee.resize(sizeof(int));
                memcpy(ee.data(),&aa,sizeof(int));
                Data.append(ee);
                //数据域长度
                size = Data.size();

                QByteArray FrameHead;
                FrameHead.append(SourceAdd+DestinaAdd);
                //信息类别
                FrameHead[8] = 0x01;
                FrameHead[9] = 0xF1;
                FrameHead[10] = 0x00;
                FrameHead[11] = 0x00;
                //保留
                FrameHead[12] = 0x00;
                FrameHead[13] = 0x00;
                FrameHead[14] = 0x00;
                FrameHead[15] = 0x00;
                //日期
                FrameHead[16] = 0x00;
                FrameHead[17] = 0x00;
                FrameHead[18]= 0x00;
                FrameHead[19]= 0x00;
                //时间
                FrameHead[20]= 0x00;
                FrameHead[21]= 0x00;
                FrameHead[22]= 0x00;
                FrameHead[23]= 0x00;
                //长度
                aa = size;
                ee.clear();
                ee.resize(sizeof(int));
                memcpy(ee.data(),&aa,sizeof(int));
                FrameHead.append(ee);

                QByteArray Record;//总数据
                Record = FrameHead+Data;
                m_Remotesocket->writeDatagram(Record.data(),Record.size(),m_settings.m_RemoteConIP,m_settings.m_RemoteConPort);
                if(AZangle-m_ACU.m_NowAz>=-0.02&&AZangle-m_ACU.m_NowAz<=0.02&&ELangle-m_ACU.m_NowEl>=-0.02&&ELangle-m_ACU.m_NowElVel<=0.02)
                {
                    QByteArray Data;
                    //标识
                    Data[0]= 0x05;
                    Data[1]= 0x41;
                    //测试状态
                    Data[2]=0x03;
                    //方位最大速度
                    int aa = AZmax;
                    QByteArray ee;
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    Data.append(ee);
                    //俯仰最大速度
                    aa = ELmax;
                    ee.clear();
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    Data.append(ee);
                    //方位最大加速度
                    aa = AZmaxadd;
                    ee.clear();
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    Data.append(ee);
                    //俯仰最大加速度
                    aa = ELmaxadd;
                    ee.clear();
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    Data.append(ee);
                    //数据域长度
                    size = Data.size();

                    QByteArray FrameHead;
                    FrameHead.append(SourceAdd+DestinaAdd);
                    //信息类别
                    FrameHead[8] = 0x01;
                    FrameHead[9] = 0xF1;
                    FrameHead[10] = 0x00;
                    FrameHead[11] = 0x00;
                    //保留
                    FrameHead[12] = 0x00;
                    FrameHead[13] = 0x00;
                    FrameHead[14] = 0x00;
                    FrameHead[15] = 0x00;
                    //日期
                    FrameHead[16] = 0x00;
                    FrameHead[17] = 0x00;
                    FrameHead[18]= 0x00;
                    FrameHead[19]= 0x00;
                    //时间
                    FrameHead[20]= 0x00;
                    FrameHead[21]= 0x00;
                    FrameHead[22]= 0x00;
                    FrameHead[23]= 0x00;
                    //长度
                    aa = size;
                    ee.clear();
                    ee.resize(sizeof(int));
                    memcpy(ee.data(),&aa,sizeof(int));
                    FrameHead.append(ee);

                    QByteArray Record;//总数据
                    Record = FrameHead+Data;
                    m_Remotesocket->writeDatagram(Record.data(),Record.size(),m_settings.m_RemoteConIP,m_settings.m_RemoteConPort);
                    PerTestFlage=false;
                    Type=0;
                }
            }
            else
            {
                QByteArray Data;
                //标识
                Data[0]= 0x05;
                Data[1]= 0x41;
                //测试状态
                Data[2]=0x04;
                //方位最大速度
                int aa = 0;
                QByteArray ee;
                ee.resize(sizeof(int));
                memcpy(ee.data(),&aa,sizeof(int));
                Data.append(ee);
                //俯仰最大速度
                aa = 0;
                ee.clear();
                ee.resize(sizeof(int));
                memcpy(ee.data(),&aa,sizeof(int));
                Data.append(ee);
                //方位最大加速度
                aa = 0;
                ee.clear();
                ee.resize(sizeof(int));
                memcpy(ee.data(),&aa,sizeof(int));
                Data.append(ee);
                //俯仰最大加速度
                aa = 0;
                ee.clear();
                ee.resize(sizeof(int));
                memcpy(ee.data(),&aa,sizeof(int));
                Data.append(ee);
                //数据域长度
                size = Data.size();

                QByteArray FrameHead;
                FrameHead.append(SourceAdd+DestinaAdd);
                //信息类别
                FrameHead[8] = 0x01;
                FrameHead[9] = 0xF1;
                FrameHead[10] = 0x00;
                FrameHead[11] = 0x00;
                //保留
                FrameHead[12] = 0x00;
                FrameHead[13] = 0x00;
                FrameHead[14] = 0x00;
                FrameHead[15] = 0x00;
                //日期
                FrameHead[16] = 0x00;
                FrameHead[17] = 0x00;
                FrameHead[18]= 0x00;
                FrameHead[19]= 0x00;
                //时间
                FrameHead[20]= 0x00;
                FrameHead[21]= 0x00;
                FrameHead[22]= 0x00;
                FrameHead[23]= 0x00;
                //长度
                aa = size;
                ee.clear();
                ee.resize(sizeof(int));
                memcpy(ee.data(),&aa,sizeof(int));
                FrameHead.append(ee);

                QByteArray Record;//总数据
                Record = FrameHead+Data;
                m_Remotesocket->writeDatagram(Record.data(),Record.size(),m_settings.m_RemoteConIP,m_settings.m_RemoteConPort);
                Type=0;
                return ;
            }
        }
        else if(Type==8&&flage==1)
        {
            SendCal_RtimeAngle();
            QByteArray Data;
            Data[0]= 0x08;
            Data[1]= 0x41;
            //生成预报文件数量
            Data[2]=m_Cal.m_FileName.size();
            //文件名(字节数不够就在后面添0)
            QByteArray a;
            QString Name;
            for(int i =0 ;i<m_Cal.m_FileName.size();i++)
            {
                QString Name=m_Cal.m_FileName[i];
                a = Name.toLocal8Bit();
                while(a.size()<61)
                {
                    a.append('0');
                }
                Data.append(a);
            }
            size = Data.size();

            QByteArray FrameHead;
            FrameHead.append(SourceAdd+DestinaAdd);
            //信息类别
            FrameHead[8] = 0x01;
            FrameHead[9] = 0xF1;
            FrameHead[10] = 0x00;
            FrameHead[11] = 0x00;
            //保留
            FrameHead[12] = 0x00;
            FrameHead[13] = 0x00;
            FrameHead[14] = 0x00;
            FrameHead[15] = 0x00;
            //日期
            FrameHead[16] = 0x00;
            FrameHead[17] = 0x00;
            FrameHead[18]= 0x00;
            FrameHead[19]= 0x00;
            //时间
            FrameHead[20]= 0x00;
            FrameHead[21]= 0x00;
            FrameHead[22]= 0x00;
            FrameHead[23]= 0x00;
            //长度
            int aa = size;
            QByteArray ee;
            ee.resize(sizeof(int));
            memcpy(ee.data(),&aa,sizeof(int));
            FrameHead.append(ee);

            QByteArray Record;//总数据
            Record = FrameHead+Data;
            m_Remotesocket->writeDatagram(Record.data(),Record.size(),m_settings.m_RemoteConIP,m_settings.m_RemoteConPort);
            Type=0;
        }
        else if(Type==9&&flage==1)
        {
            SendCal_sweep();
            QByteArray Data;
            Data[0]= 0x09;
            Data[1]= 0x41;
            //标识状态
            Data[2]=m_Cal.m_SweepState;
            //标校射电源个数
            Data[3]=m_Cal.m_SweepNumber;
            //标校点个数
            Data[4]=m_Cal.m_SweepCalNumber;
            //当前标校点标识
            Data[5]=m_Cal.m_SweepNowNumber;
            //当前目标名称
            QByteArray a;
            QString Name=m_Cal.m_NowName;
            a = Name.toLocal8Bit();
            while(a.size()<10)
            {
                a.append('\x00');
            }
            Data.append(a);
            if(m_Cal.m_SweepState==4)
            {
                int Time =m_Cal.m_NowSpotBeginDate.toInt();
                //当前标校点开始日期
                Data.append(intToByte(Time,8,1));
//                Data.append(2);
//                Data.append('\x00');
//                Data.append(2);
//                Data.append(1);
//                Data.append(1);
//                Data.append('\x00');
//                Data.append(2);
//                Data.append(5);
                //当前标校点开始时间
                Time =m_Cal.m_NowSpotBeginTime.toInt();
                Data.append(intToByte(Time,8,1));
//                Data.append(1);
//                Data.append(5);
//                Data.append(2);
//                Data.append(6);
//                Data.append(4);
//                Data.append(3);
//                Data.append(2);
//                Data.append(5);
                //当前标校点结束日期
                Time =m_Cal.m_NowSpotEndDate.toInt();
                Data.append(intToByte(Time,8,1));
//                Data.append(2);
//                Data.append('\x00');
//                Data.append(2);
//                Data.append(1);
//                Data.append(1);
//                Data.append('\x00');
//                Data.append(2);
//                Data.append(5);
                //当前标校点结束时间
                Time =m_Cal.m_NowSpotEndTime.toInt();
                Data.append(intToByte(Time,8,1));
//                Data.append(1);
//                Data.append(5);
//                Data.append(2);
//                Data.append(6);
//                Data.append(4);
//                Data.append(8);
//                Data.append(2);
//                Data.append(2);
            }
            else
            {
                Data.append('\x00');
                Data.append('\x00');
                Data.append('\x00');
                Data.append('\x00');
                Data.append('\x00');
                Data.append('\x00');
                Data.append('\x00');
                Data.append('\x00');
                //当前标校点开始时间
                Data.append('\x00');
                Data.append('\x00');
                Data.append('\x00');
                Data.append('\x00');
                Data.append('\x00');
                Data.append('\x00');
                Data.append('\x00');
                Data.append('\x00');
                //当前标校点结束日期
                Data.append('\x00');
                Data.append('\x00');
                Data.append('\x00');
                Data.append('\x00');
                Data.append('\x00');
                Data.append('\x00');
                Data.append('\x00');
                Data.append('\x00');
                //当前标校点结束时间
                Data.append('\x00');
                Data.append('\x00');
                Data.append('\x00');
                Data.append('\x00');
                Data.append('\x00');
                Data.append('\x00');
                Data.append('\x00');
                Data.append('\x00');
            }
            //当前标校点开始日期

            //数据域长度
            size = Data.size();


            QByteArray FrameHead;
            FrameHead.append(SourceAdd+DestinaAdd);
            //信息类别
            FrameHead[0] = 0x01;
            FrameHead[1] = 0xF1;
            FrameHead[2] = 0x00;
            FrameHead[3] = 0x00;
            //保留
            FrameHead[4] = 0x00;
            FrameHead[5] = 0x00;
            FrameHead[6] = 0x00;
            FrameHead[7] = 0x00;
            //日期
            FrameHead[8] = 0x00;
            FrameHead[9] = 0x00;
            FrameHead[10]= 0x00;
            FrameHead[11]= 0x00;
            //时间
            FrameHead[12]= 0x00;
            FrameHead[13]= 0x00;
            FrameHead[14]= 0x00;
            FrameHead[15]= 0x00;
            //长度
            int aa = size;
            QByteArray ee;
            ee.resize(sizeof(int));
            memcpy(ee.data(),&aa,sizeof(int));
            FrameHead.append(ee);

            QByteArray Record;//总数据
            Record = FrameHead+Data;
            m_Remotesocket->writeDatagram(Record.data(),Record.size(),m_settings.m_RemoteConIP,m_settings.m_RemoteConPort);
            Type=0;

        }
        else if(Type==10&&flage==1)
        {
            if(AngleReportFlag==1)
            {
                QByteArray Data;
                Data[0]= 0x0A;
                Data[1]= 0x41;
                //方位
                int aa = (int)(m_ACU.m_NowAz*1000);
                QByteArray ee;
                ee.resize(sizeof(int));
                memcpy(ee.data(),&aa,sizeof(int));
                Data.append(ee);
                //俯仰
                aa = (int)(m_ACU.m_NowEl*1000);
                ee.clear();
                ee.resize(sizeof(int));
                memcpy(ee.data(),&aa,sizeof(int));
                Data.append(ee);
                //相心_X
                aa = 0;
                ee.clear();
                ee.resize(sizeof(int));
                memcpy(ee.data(),&aa,sizeof(int));
                Data.append(ee);
                //相心_Y
                aa = 0;
                ee.clear();
                ee.resize(sizeof(int));
                memcpy(ee.data(),&aa,sizeof(int));
                Data.append(ee);
                //相心_Z
                aa = 0;
                ee.clear();
                ee.resize(sizeof(int));
                memcpy(ee.data(),&aa,sizeof(int));
                Data.append(ee);
                //数据域长度
                size = Data.size();
                QByteArray FrameHead;
                FrameHead.append(SourceAdd+DestinaAdd);
                //信息类别
                FrameHead[8] = 0x01;
                FrameHead[9] = 0xF1;
                FrameHead[10] = 0x00;
                FrameHead[11] = 0x00;
                //保留
                FrameHead[12] = 0x00;
                FrameHead[13] = 0x00;
                FrameHead[14] = 0x00;
                FrameHead[15] = 0x00;
                //日期
                FrameHead[16] = 0x00;
                FrameHead[17] = 0x00;
                FrameHead[18]= 0x00;
                FrameHead[19]= 0x00;
                //时间
                FrameHead[20]= 0x00;
                FrameHead[21]= 0x00;
                FrameHead[22]= 0x00;
                FrameHead[23]= 0x00;
                //长度
                aa = size;
                ee.clear();
                ee.resize(sizeof(int));
                memcpy(ee.data(),&aa,sizeof(int));
                FrameHead.append(ee);

                QByteArray Record;//总数据
                Record = FrameHead+Data;
                m_Remotesocket->writeDatagram(Record.data(),Record.size(),m_settings.m_RemoteConIP,m_settings.m_RemoteConPort);
            }
            else if(AngleReportFlag ==2)
            {
                AngleReportFlag=0;
                Type = 0;
            }
        }
    });
}

