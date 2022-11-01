#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "systemset.h"
extern systemsetdata m_settings;
extern ACUState m_ACU;
extern CalibrationState m_Cal;
extern ClickedSend m_CliSend;
extern QMutex m_ACULock;
extern QMutex m_CalLock;
extern Prodata m_Prodata;
extern int flage;
extern bool ProsendState;
extern QList<Prodata> m_Prodatanode;
extern Win_QextSerialPort* myCom;
extern bool CalFlag;
#define re0 6378140
#define PI 3.141592653
#define f_l 1/298.257
//extern std::list<QString>m_SourceCode;
#pragma execution_character_set("utf-8")
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    flage=2;
    ProsendState=false;
    CalFlag=false;
    ui->starstop->setFlat(true);//设置边框不显示
    readXML();//读取配置文件
    init_UdpSocket();
    initwidget();
    init_Calparameter();
    WriteLog();
    Insertlog("软件已启动");
    SetUpFtp();
    QElapsedTimer time;
    upstate = new QTimer(this);
    upstate->setTimerType(Qt::PreciseTimer);
    upstate->start(200);
    Prostate = new QTimer(this);
    Prostate->setTimerType(Qt::PreciseTimer);
    //Prostate->start(200);
    connect(upstate,&QTimer::timeout,this,&MainWindow::UdpRcev);
    connect(Prostate,&QTimer::timeout,this,&MainWindow::SendProData);
    connect(udpsocket,&UdpThread::Romotedata,this,&MainWindow::Insertlog);
    sendsocket = new QUdpSocket(this);
    Prodatasend = new ProdataSend;
    mycomthread = new QThread(this);
    Prodatasend->moveToThread(mycomthread);
    connect(mycomthread,&QThread::started,Prodatasend,&ProdataSend::Startrun);//读取程引文件
    connect(mycomthread, &QThread::finished, Prodatasend, &QObject::deleteLater);

}

MainWindow::~MainWindow()
{
    delete ui;
}
/**************************************************************
 * Insertlog(QString content) 写入日志
**************************************************************/
void MainWindow::Insertlog(QString content)
{
    QDateTime ftime = QDateTime::currentDateTime();
    QString init=QString("%1    "+content).arg(ftime.toString("yyyy-MM-dd HH:mm:ss"));
    ui->textEdit->append(init);
    QString Path;
    Path=LogFolderpath+LogPath;
    QFile file(Path);
    QTextStream in(&file);
    if(file.open(QIODevice::ReadWrite|QIODevice::Append|QIODevice::Text))
    {
        in<<init<<endl;
        file.close();
    }
    else
    {
        Message(2,1,"日志文件未打开，请重试");
    }

}

void MainWindow::chekstate()
{
    if(ui->RadioPosition->isChecked())
    {
        ui->radioFirst->setText("0.06°");
        ui->radioSecond->setText("0.12°");
    }
    else
    {
        ui->radioFirst->setText("0.06°/sec");
        ui->radioSecond->setText("0.12°/sec");
    }


}
/**************************************************************
 * initwidget() 初始化主窗口
**************************************************************/
void MainWindow::initwidget()
{
    chart = new QChart;
    seriesAZ = new QLineSeries();
    seriesEL = new QLineSeries();
    QTimer *timer = new QTimer;
    timer->start(100);
    connect(timer,&QTimer::timeout,[=](){
         QDateTime uptime= QDateTime::currentDateTime();
         ui->lineEdit_UTC->setText(uptime.toString("yyyy-MM-dd hh:mm:ss.zzz"));
         QString YMD = uptime.toString("yyyy-MM-dd");
         if(YMD!=LogTime)
         {
             LogPath=QString("/"+YMD+".log");
             QFile Logfile(LogFolderpath+LogPath);
             Logfile.open(QIODevice::ReadWrite);
         }
         double MJD = Cal_MJD(uptime);
         ui->lineEdit_localMJD->setText(QString::number(MJD,'f',5));
    });
    QButtonGroup *block1=new QButtonGroup(this);
    block1->addButton(ui->radioFirst,0);//一个值为0
    block1->addButton(ui->radioSecond,1);//一个值为1
    block1->addButton(ui->RadioCustomsize,2);
    ui->radioFirst->setChecked(1);

    ui->label_AZ1_POWER->setPixmap(QPixmap(":/icon/red.png"));
    ui->label_AZ1_POWER->setScaledContents(true);
    ui->label_AZ1_ENABLE->setPixmap(QPixmap(":/icon/red.png"));
    ui->label_AZ1_ENABLE->setScaledContents(true);
    ui->label_F1_POWER->setPixmap(QPixmap(":/icon/red.png"));
    ui->label_F1_POWER->setScaledContents(true);
    ui->label_F1_ENABLE->setPixmap(QPixmap(":/icon/red.png"));
    ui->label_F1_ENABLE->setScaledContents(true);
    ui->label_AZ1_STA->setPixmap(QPixmap(":/icon/red.png"));
    ui->label_AZ1_STA->setScaledContents(true);
    ui->label_F1_STA->setPixmap(QPixmap(":/icon/red.png"));
    ui->label_F1_STA->setScaledContents(true);
    ui->label_AZ1_CW->setPixmap(QPixmap(":/icon/red.png"));
    ui->label_AZ1_CW->setScaledContents(true);
    ui->label_F1_CW->setPixmap(QPixmap(":/icon/red.png"));
    ui->label_F1_CW->setScaledContents(true);
    ui->label_AZ1_CCW->setPixmap(QPixmap(":/icon/red.png"));
    ui->label_AZ1_CCW->setScaledContents(true);
    ui->label_F1_CCW->setPixmap(QPixmap(":/icon/red.png"));
    ui->label_F1_CCW->setScaledContents(true);

    ui->lineEditStep->setEnabled(false);
    ui->textEdit->setFocusPolicy(Qt::NoFocus);
    ui->textEdit->setCursor(Qt::ArrowCursor);
}
/**************************************************************
 * initQChart() 初始化曲线图
**************************************************************/
void MainWindow::initQChart()
{
    seriesAZ->clear();
    seriesEL->clear();

    QList<Prodata>::const_iterator it = m_Prodatanode.begin();
    int i =0;
    while(it !=m_Prodatanode.end())
    {

        *seriesAZ<<QPointF(i,it->AZ);
        *seriesEL<<QPointF(i,it->EL);
        it++;
        i++;

    }
//    for (int i = 0;i<m_Prodatanode.size() ;i++ )
//    {
//        QString aa= m_Prodatanode.;
//        *seriesAZ<<QPointF(i,aa.toDouble());
//    }
//    for (int i = 0;i<Prodata.size() ;i++ )
//    {
//        QString aa= Prodata.at(i).mid(35,8);
//        *seriesEL<<QPointF(i,aa.toDouble());
//    }

         seriesAZ->setName("程引方位");
         seriesAZ->setColor(QColor(Qt::blue));
         seriesEL->setName("程引俯仰");
         seriesEL->setColor(QColor(Qt::green));


         QValueAxis * Axisy = new QValueAxis;
         Axisy->setTickCount(9);
         Axisy->setRange(-720,720);
         Axisy->setGridLinePen(QPen(QColor(Qt::black), 1, Qt::DashLine));//颜色为黑色虚线
         chart->addSeries(seriesAZ);
         chart->addSeries(seriesEL);
         chart->createDefaultAxes();
         chart->setAxisY(Axisy,seriesAZ);
         chart->setAxisY(Axisy,seriesEL);

         chart->setContentsMargins(0, 0, 0, 0);//设置外边界全部为0
         chart->setMargins(QMargins(0, 0, 0, 0));//设置内边界全部为0
         chart->setBackgroundRoundness(0);

         ui->widget->setChart(chart);

}
/**************************************************************
 * ReadProfile() 读取程引文件，解析程引文件
**************************************************************/
void MainWindow::ReadProfile()
 {
//qDebug()<<m_settings.m_FtpPath<<"文件目录";
    //Prodata.clear();//每次读取文件的时候清空之前的记录
    QFile file(ProFileName);
    qDebug()<<ProFileName;
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Message(1,1,"文件打开失败，请重试");
        return;
    }

    QDateTime temp,Protime;
    QDateTime now = QDateTime::currentDateTime();
    double  max=0,tem=0,min=0,tem1=0;
    int i = 0;
    int j =0;
    double X;//接收文件的坐标
    double Y;
    double Z;
    m_Prodatanode.clear();
    while(!file.atEnd()) {

        QString Pro = file.readLine();
     //   qDebug()<<Pro;
        if(i>=1)//第一次为数据头 不截取
        {
             QStringList aa= Pro.split('\t');
             m_Prodata.Time = UTCTOLocalTime(QDateTime::fromString(aa[0],"yyyy-MM-dd'T'hh:mm:ss"));
           //  qDebug()<<m_Prodata.Time<<"++++++++++++++";
           //  m_Prodata.Time = UTCTOLocalTime(QDateTime::fromString(aa[0],"hh:mm:ss")) ;

            if(m_Prodata.Time>now)
            {
                X=aa[1].toDouble();
                Y=aa[2].toDouble();
                Z=aa[3].toDouble();

                PAZ= GetAE(X,Y,Z,true);
             //     PAZ= aa[2].toDouble();
                m_Prodata.AZ=PAZ;
                //ui->label_PAZ->setText(QString::number(PAZ));
                //每一次的俯仰角度
                PEL= GetAE(X,Y,Z,false);
                //PEL=aa[3].toDouble();
                m_Prodata.EL=PEL;
                if(j==0)//第一次的数据直接发送
                {
                    ui->label_starttime->setText(m_Prodata.Time.toString("yyyy-MM-dd hh:mm:ss"));
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
                if(max<=PEL)
                {
                    max=PEL;
                }
                if(min>=PEL)
                {
                    min=PEL;
                }
                j++;
            }
        }
        i++;
    }
    QString num = QString("%1").arg(max,7,'f',3,'0');
    ui->label_MAXEL->setText(num);
    num = QString("%1").arg(min,7,'f',3,'0');
    ui->label_MINEL->setText(num);
    if(m_Prodatanode.size()==0)
    {
        Message(2,1,"没有可执行的数据！");
    }
    else
    {
        ui->label_endtime->setText(m_Prodata.Time.toString("yyyy-MM-dd hh:mm:ss"));
    }
   file.close();
}
/**************************************************************
 * WriteLog() 创建文件写日志文档
**************************************************************/
void MainWindow::WriteLog()
{

    QDateTime now = QDateTime::currentDateTime();
    LogTime = now.toString("yyyy-MM-dd");
    LogFolderpath =QCoreApplication::applicationDirPath();
    LogFolderpath.append("/log");

    QDir tempDir;
    //临时保存程序当前路径
    QString currentDir = tempDir.currentPath();
    //如果filePath路径不存在，创建它
    if(!tempDir.exists(LogFolderpath))
    {
        qDebug()<<"不存在该路径"<<endl;
        tempDir.mkpath(LogFolderpath);
    }

    LogPath=QString("/"+LogTime+".log");

    QFile Logfile(LogFolderpath+LogPath);
    Logfile.open(QIODevice::ReadWrite);
}
/**************************************************************
 * SendProData() 解析程引文件
**************************************************************/
void MainWindow::SendProData()
{
//    QList<Prodata>::const_iterator it = m_Prodatanode.begin();
//    if(it != m_Prodatanode.end())
//    {
//        if(number==0)
//        {
//            FirstAZ=it->AZ;
//            FirstEL=it->EL;
//            QString deta =QString("<101/04_0,%1").arg(QString::number(FirstAZ));
//            sendsocket->writeDatagram(deta.toUtf8(),deta.size(),m_settings.m_AntennaIp,m_settings.m_AntennaPort);
//            deta =QString("<101/04_1,%1").arg(QString::number(FirstEL));
//            sendsocket->writeDatagram(deta.toUtf8(),deta.size(),m_settings.m_AntennaIp,m_settings.m_AntennaPort);
//            m_Prodatanode.erase(it);//删除头节点
//            number++;
//        }
//        else
//        {
//            if(i<it->number)
//            {
//                aa+=it->AZAngle;
//                QString data = QString("<101/04_0,%1").arg(QString::number(aa+FirstAZ));
//                sendsocket->writeDatagram(data.toUtf8(),data.size(),m_settings.m_AntennaIp,m_settings.m_AntennaPort);

//                bb+=it->ELAngle;
//                data =QString("<101/04_1,%1").arg(QString::number(bb+FirstEL));
//                sendsocket->writeDatagram(data.toUtf8(),data.size(),m_settings.m_AntennaIp,m_settings.m_AntennaPort);
//                i++;
//            }
//            else
//            {
//                FirstAZ=it->AZ;
//                FirstEL=it->EL;
//                aa=0;
//                bb=0;
//                i=0;
//                m_Prodatanode.erase(it);//删除头节点
//            }
//        }

//    }
//    else
//    {
//        number=0;
//        Prostate->stop();
//    }
//    while(it !=m_Prodatanode.end())
//    {
//        if(i==0)
//        {
//            FirstAZ=it->AZ;
//            FirstEL=it->EL;
//            QString deta =QString("<101/04_0,%1").arg(QString::number(FirstAZ));
//            sendsocket->writeDatagram(deta.toUtf8(),deta.size(),m_settings.m_AntennaIp,m_settings.m_AntennaPort);
//            deta =QString("<101/04_1,%1").arg(QString::number(FirstEL));
//            sendsocket->writeDatagram(deta.toUtf8(),deta.size(),m_settings.m_AntennaIp,m_settings.m_AntennaPort);
//        }
//        else if(i>0)
//        {
//            while(j<it->number)
//            {
//                Sleep(0.02);
//                aa+=it->AZAngle;
//                QString data = QString("<101/04_0,%1").arg(QString::number(aa+FirstAZ));
//                sendsocket->writeDatagram(data.toUtf8(),data.size(),m_settings.m_AntennaIp,m_settings.m_AntennaPort);

//                bb+=it->ELAngle;
//                data =QString("<101/04_1,%1").arg(QString::number(bb+FirstEL));
//                sendsocket->writeDatagram(data.toUtf8(),data.size(),m_settings.m_AntennaIp,m_settings.m_AntennaPort);
//                j++;
//            }
//            FirstAZ=it->AZ;
//            FirstEL=it->EL;
//            aa=0;
//            bb=0;

//        }
//        ui->label_PAZ->setText(QString::number(FirstAZ));
//        ui->label_PEL->setText(QString::number(FirstEL));
//        Insertlog(QString("正在执行AZ：%0").arg(QString::number(it->AZ)));
//        Insertlog(QString("正在执行EL：%0").arg(QString::number(it->EL)));
//        it++;
//        i++;
//        j=0;
//    }
//    if(it==m_Prodatanode.end())
//    {
//        Prostate->stop();
//        m_Prodatanode.clear();
    //    }
}
/**************************************************************
 * SetUpFtp() 创建FTP文件夹
**************************************************************/
void MainWindow::SetUpFtp()
{
    //FTPserver.setWelcomeMessage("Hi,\nWelcome to debao's Ftp server.");
    //FTPserver.addAccount("anonymous");  //匿名访问

    FTPserver.addAccount(m_settings.UserName,     //用户名
                         m_settings.Password,     //密码
                         m_settings.m_FtpPath,    //路劲
                         FtpServer::Read | FtpServer::Write | FtpServer::Exec);
    if (FTPserver.listen(m_settings.m_FtpIp, m_settings.m_FtpPort)) {
        qDebug()<<QString("Listening at port %1").arg(m_settings.m_FtpPort);
    } else {
        qDebug()<<"Failed.";
        return;
    }
}

/*****************************************************************************************************************************
 * 状态字为一个 16 位二进制数，发送时以 4 位十六进制数表示，高位在前，低位在后，例如十六进制数 0xA5，转换为‘A’‘5’，最后形如 D15D14...D1D0，
 * 可表示 16*4=64 个状态。
 * 二进制位 含义
 * D3-D0 工作模式代码，共有 12 种模式，定义如下：
 * 0000---系统未就绪
 * 0001---俯仰收藏
 * 0011---手控速度
 * 0100---手控位置
 * 0101---预置位置
 * 0110---预置卫星
 * 0111---自动跟踪
 * 1001---自动搜索
 * 1010---入锁
 * 1011---退锁
 * 1110---系统就绪
 * 1111---正在准备
其他未定义
D4 为 1 表示方位速度控制不允许
D5 为 1 表示方位位置控制不允许
D6 为 1 表示俯仰速度控制不允许
D7 为 1 表示俯仰位置控制不允许
D8 为 1 表示跟踪信号满足跟踪条件
D9 为 1 表示刚接收到的命令格式错误
D10 为 1 表示拒绝执行刚接收的命令
D11 为 1 表示该设备处于遥控方式
D12 为 1 表示天线正在顺时针转动
D13 为 1 表示天线正在逆时针转动
D14 为 1 表示天线正在向上转动
D15 为 1 表示天线正在向下转
**************************************************************************************************************************************/
/**************************************************************
 * UdpRcev() 接收信号， 更新界面状态
**************************************************************/
void MainWindow::UdpRcev()
{
    if(m_ACU.state==true)
    {
        m_ACULock.lock();
        ACUState temacu=m_ACU;
        m_ACULock.unlock();
        //更新方位俯仰差
        QString num = QString("%1").arg(ui->label_PAZ->text().toDouble()-temacu.m_NowAz,7,'f',3,'0');
        ui->label_PAZ_Offset->setText(num);
        num = QString("%1").arg(ui->label_PEL->text().toDouble()-temacu.m_NowEl,7,'f',3,'0');
        ui->label_PEL_Offset->setText(num);
        //更新方位角， 俯仰角度数
        num = QString("%1").arg(temacu.m_NowAz,7,'f',3,'0');
        ui->label_AZ_angle->setText(num);
        num = QString("%1").arg(temacu.m_NowEl,7,'f',3,'0');
        ui->label_EL_angle->setText(num);
        num = QString("%1").arg(temacu.m_NowAzVel,7,'f',3,'0');
        ui->label_AZ_Speed->setText(num);
        num = QString("%1").arg(temacu.m_NowElVel,7,'f',3,'0');
        ui->label_EL_Speed->setText(num);
        num = QString("%1").arg(temacu.m_NowAz,7,'f',3,'0');
        ui->label_NAZ->setText(num);
        num = QString("%1").arg(temacu.m_NowEl,7,'f',3,'0');
        ui->label_NEL->setText(num);
        //更新方位角，俯仰角状态
        if(temacu.m_AzFirstDriver!=0)
        {

            //ui->label_AZ_speed->setText("0x"+QByteArray::number(temacu.m_AzFirstDriver, 2));
            if(temacu.m_AzFirstDriverList.size()>1)
            {
                for (int i =0;i<temacu.m_AzFirstDriverList.size();i++) {
                    Insertlog("方位1:"+temacu.m_AzFirstDriverList.at(i));
                    ui->label_AZ_speed->setText(temacu.m_AzFirstDriverList.at(i));
                    m_ACULock.lock();
                    m_ACU.m_AzFirstDriverList.removeAt(i);
                    m_ACULock.unlock();
                }
            }

        }
        if(m_ACU.m_AzSecondDriver!=0)
        {
            //ui->label_AZ1_Temperature->setText("0x"+QByteArray::number(temacu.m_AzSecondDriver,2));
            if(temacu.m_AzSecondDriverList.size()>1)
            {
                for (int i=0;i<temacu.m_AzSecondDriverList.size();i++) {
                    Insertlog("方位2:"+temacu.m_AzSecondDriverList.at(i));
                    ui->label_AZ1_Temperature->setText(temacu.m_AzSecondDriverList.at(i));
                    m_ACULock.lock();
                    m_ACU.m_AzSecondDriverList.removeAt(i);
                    m_ACULock.unlock();
                }
            }
        }
        if(m_ACU.m_ElFirstDriver!=0)
        {
            //ui->label_EL_speed->setText("0x"+QByteArray::number(temacu.m_ElFirstDriver,2));
            if(temacu.m_ElFirstDriverList.size()>0)
            {
                for(int i =0;i<temacu.m_ElFirstDriverList.size();i++)
                {
                    Insertlog("俯仰1:"+temacu.m_ElFirstDriverList.at(i));
                    ui->label_EL_speed->setText(temacu.m_ElFirstDriverList.at(i));
                    m_ACULock.lock();
                    m_ACU.m_ElFirstDriverList.removeAt(i);
                    m_ACULock.unlock();
                }
            }
        }
        if(m_ACU.m_ElSecondDriver!=0)
        {
            //ui->label_EL_Temperature->setText("0x"+QByteArray::number(temacu.m_ElSecondDriver,2));
            if(temacu.m_ElSecondDriverList.size()>1)
            {
                for(int i =0;i<temacu.m_ElSecondDriverList.size();i++)
                {
                    Insertlog("俯仰2:"+temacu.m_ElSecondDriverList.at(i));
                    ui->label_EL_Temperature->setText(temacu.m_ElSecondDriverList.at(i));
                    m_ACULock.lock();
                    m_ACU.m_ElSecondDriverList.removeAt(i);
                    m_ACULock.unlock();
                }
            }
        }
        //更新AZ EL状态
        if(temacu.m_AzFirstDriverList.length()!=1&&temacu.m_AzSecondDriverList.length()!=1)
        {
            ui->label_AZ1_POWER->setPixmap(QPixmap(":/icon/red.png"));
            ui->label_AZ1_POWER->setScaledContents(true);
        }
        else
        {
            ui->label_AZ1_POWER->setPixmap(QPixmap(":/icon/green.png"));
            ui->label_AZ1_POWER->setScaledContents(true);
        }
        if(temacu.m_ElFirstDriverList.length()!=1&&temacu.m_ElSecondDriverList.length()!=1)
        {
            ui->label_AZ1_STA->setPixmap(QPixmap(":/icon/red.png"));
            ui->label_AZ1_STA->setScaledContents(true);
        }
        else
        {
            ui->label_AZ1_STA->setPixmap(QPixmap(":/icon/green.png"));
            ui->label_AZ1_STA->setScaledContents(true);
        }
        if(temacu.m_AzCW==0)
        {
            ui->label_AZ1_ENABLE->setPixmap(QPixmap(":/icon/green.png"));
            ui->label_AZ1_ENABLE->setScaledContents(true);
        }
        else
        {
            ui->label_AZ1_ENABLE->setPixmap(QPixmap(":/icon/red.png"));
            ui->label_AZ1_ENABLE->setScaledContents(true);
        }

        if(temacu.m_AzCCW==0)
        {
            ui->label_F1_ENABLE->setPixmap(QPixmap(":/icon/green.png"));
            ui->label_F1_ENABLE->setScaledContents(true);
        }
        else
        {
            ui->label_F1_ENABLE->setPixmap(QPixmap(":/icon/red.png"));
            ui->label_F1_ENABLE->setScaledContents(true);
        }
        if(temacu.m_ElCW==0)
        {
            ui->label_AZ1_CW->setPixmap(QPixmap(":/icon/green.png"));
            ui->label_AZ1_CW->setScaledContents(true);
        }
        else
        {
            ui->label_AZ1_CW->setPixmap(QPixmap(":/icon/red.png"));
            ui->label_AZ1_CW->setScaledContents(true);
        }
        if(temacu.m_ElCCW==0)
        {
            ui->label_F1_CW->setPixmap(QPixmap(":/icon/green.png"));
            ui->label_F1_CW->setScaledContents(true);
        }
        else
        {
            ui->label_F1_CW->setPixmap(QPixmap(":/icon/red.png"));
            ui->label_F1_CW->setScaledContents(true);
        }

        if(temacu.m_ElLock==0)
        {
            ui->label_AZ1_CCW->setPixmap(QPixmap(":/icon/green.png"));
            ui->label_AZ1_CCW->setScaledContents(true);
        }
        else
        {
            ui->label_AZ1_CCW->setPixmap(QPixmap(":/icon/red.png"));
            ui->label_AZ1_CCW->setScaledContents(true);
        }
        if(temacu.m_ElUnlock==0)
        {
            ui->label_F1_CCW->setPixmap(QPixmap(":/icon/green.png"));
            ui->label_F1_CCW->setScaledContents(true);
        }
        else
        {
            ui->label_F1_CCW->setPixmap(QPixmap(":/icon/red.png"));
            ui->label_F1_CCW->setScaledContents(true);
        }

        //更新系统状态
        if(temacu.m_ACUState==0)
        {
            ui->label_ACUState->setText("系统未就绪");
        }
        else if(temacu.m_ACUState==1)
        {
            ui->label_ACUState->setText("俯仰收藏");
        }
        else if(temacu.m_ACUState==3)
        {
            ui->label_ACUState->setText("手控速度");
        }
        else if(temacu.m_ACUState==4)
        {
            ui->label_ACUState->setText("手控位置");
        }
        else if(temacu.m_ACUState==5)
        {
            ui->label_ACUState->setText("预置位置");
        }
        else if(temacu.m_ACUState==6)
        {
            ui->label_ACUState->setText("预置卫星");
        }
        else if(temacu.m_ACUState==7)
        {
            ui->label_ACUState->setText("自动跟踪");
        }
        else if(temacu.m_ACUState==11)
        {
            ui->label_ACUState->setText("自动搜索");
        }
        else if(temacu.m_ACUState==12)
        {
            ui->label_ACUState->setText("入锁");
        }
        else if(temacu.m_ACUState==13)
        {
            ui->label_ACUState->setText("退锁");
        }
        else if(temacu.m_ACUState==14)
        {
            ui->label_ACUState->setText("系统就绪");
        }
        else if(temacu.m_ACUState==15)
        {
            ui->label_ACUState->setText("正在准备");
        }

    }

    if(m_Cal.State==true)
    {

        m_CalLock.lock();
        CalibrationState temcal =m_Cal;
        m_CalLock.unlock();
        if(temcal.m_SubsystemStatus==0)
        {
            ui->label_SubsystemStatus->setText("无此参数");
        }
        else if(temcal.m_SubsystemStatus==1)
        {
            ui->label_SubsystemStatus->setText("正常");
        }
        else if(temcal.m_SubsystemStatus==2)
        {
            ui->label_SubsystemStatus->setText("故障");
        }
        else if(temcal.m_SubsystemStatus==3)
        {
            ui->label_SubsystemStatus->setText("维护");
        }
        if(temcal.m_ControlMode==0)
        {
            ui->label_ControlMode->setText("本控");
        }
        else if(temcal.m_ControlMode==1)
        {
            ui->label_ControlMode->setText("分控");
        }
        else if(temcal.m_ControlMode==2)
        {
            ui->label_ControlMode->setText("无此参数");
        }
        if(temcal.m_WorkMode==0)
        {
            ui->label_WorkMode->setText("空闲");
        }
        else if(temcal.m_WorkMode==1)
        {
            ui->label_WorkMode->setText("标校");
        }
        else if(temcal.m_WorkMode==2)
        {
            ui->label_WorkMode->setText("无此参数");
        }
        //更新星位引导信息
        ui->label_AstralTime->setText(temcal.m_AstralTime.toString("yyyy-MM-dd hh:mm:ss.zzz"));
        QString num = QString("%1").arg(temcal.m_AstralNowAz,7,'f',3,'0');
        ui->label_AstralAZ->setText(num);
        num = QString("%1").arg(temcal.m_AstralNowEl,7,'f',3,'0');
        ui->label_AstralEL->setText(num);
        //更新实时角度信息
        ui->label_UpTime->setText(temcal.m_UpTime.toString("yyyy-MM-dd hh:mm:ss.zzz"));
        num = QString("%1").arg(temcal.m_UpTimeNowAz,7,'f',3,'0');
        ui->label_UpTimeAZ->setText(num);
        num = QString("%1").arg(temcal.m_UpTimeNowEl,7,'f',3,'0');
        ui->label_UpTimeEL->setText(num);
        //更新轨道预报界面信息
        if(temcal.m_TrackPositionResult==0)
        {
            ui->lineEdit_TrackPositionResult->setText("正常接收并执行");
        }
        else if(temcal.m_TrackPositionResult==1)
        {
            ui->lineEdit_TrackPositionResult->setText("分控拒收");
        }
        else if(temcal.m_TrackPositionResult==2)
        {
            ui->lineEdit_TrackPositionResult->setText("帧格式错误");
        }
        else if(temcal.m_TrackPositionResult==3)
        {
            ui->lineEdit_TrackPositionResult->setText("被控对象不存在");
        }
        else if(temcal.m_TrackPositionResult==4)
        {
            ui->lineEdit_TrackPositionResult->setText("参数错误");
        }
        else if(temcal.m_TrackPositionResult==5)
        {
            ui->lineEdit_TrackPositionResult->setText("条件不具备");
        }
        else if(temcal.m_TrackPositionResult==6)
        {
            ui->lineEdit_TrackPositionResult->setText("未知原因失败");
        }

        ui->lineEdit_Sweepident->setText(QString::number(temcal.m_Sweepident));
        if(temcal.m_SweepState==1)
        {
            ui->lineEdit_SweepState->setText("生成标校规划文件正常");
        }
        else if(temcal.m_SweepState==2)
        {
            ui->lineEdit_SweepState->setText("生成标校规划文件异常");
        }
        else if(temcal.m_SweepState==3)
        {
            ui->lineEdit_SweepState->setText("等待标校开始时间到");
        }
        else if(temcal.m_SweepState==4)
        {
            ui->lineEdit_SweepState->setText("正在标校");
        }
        else if(temcal.m_SweepState==5)
        {
            ui->lineEdit_SweepState->setText("标校完成，结束");
        }
        else if(temcal.m_SweepState==6)
        {
            ui->lineEdit_SweepState->setText("收到指令，结束标校");
        }
        else if(temcal.m_SweepState==7)
        {
            ui->lineEdit_SweepState->setText("标校未完成，异常终止");
        }

        //更新指向标校界面参数信息
        ui->lineEdit_SweepNumber->setText(QString::number(temcal.m_SweepNumber));
        ui->lineEdit_SweepNowNumber->setText(QString::number(temcal.m_SweepNowNumber));
        ui->lineEdit_SweepCalNumber->setText(QString::number(temcal.m_SweepCalNumber));
        ui->lineEdit_NowName->setText(temcal.m_NowName);
        ui->lineEdit_NowSpotBeginDate->setText(temcal.m_NowSpotBeginDate);
        ui->lineEdit_NowSpotBeginTime->setText(temcal.m_NowSpotBeginTime);
        ui->lineEdit_NowSpotEndDate->setText(temcal.m_NowSpotEndDate);
        ui->lineEdit_NowSpotEndTime->setText(temcal.m_NowSpotEndTime);
        //更新射电源信息参数
        //ui->lineEdit_filename->setText(temcal.m_FileName);
        ui->lineEdit_StartTime->setText(temcal.m_StartTime);
    }
}
/**************************************************************
 * init_UdpSocket() UDP初始化
**************************************************************/
void MainWindow::init_UdpSocket()
{
    udpsocket=new UdpThread();
    udpthread=new QThread();
    udpsocket->moveToThread(udpthread);
    connect(udpthread,&QThread::finished,udpsocket,&QObject::deleteLater);
    //connect(udpsocket,&UdpThread::analysis_singal,this,&MainWindow::Analysis_slots);
    connect(udpthread,&QThread::started,udpsocket,&UdpThread::RecvData);

    udpthread->start();

}

void MainWindow::init_Calparameter()
{
    QString path =QCoreApplication::applicationDirPath();
    path.append("/Calparameter.txt");
    QFile File(path);
    if(File.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        QByteArray byte = File.readAll();
        QString data = QString::fromLocal8Bit(byte);
        int startIndex = data.indexOf('\n',0);
        int endIndex = data.indexOf('\n',startIndex);
        QString Caldata = data.mid(startIndex,endIndex-startIndex);
        qDebug()<<data;
        //QStringList aa= data.split('\t');
    }

}


/**************************************************************
 * on_starstop_clicked() 启动伺服或者关闭
**************************************************************/
void MainWindow::on_starstop_clicked()
{
    if(Openflage==false)
    {
       QString data = "<000/01\r";
       myCom->write(data.toUtf8(),data.size());
       Openflage=true;
    }
    else if(Openflage==true)
    {
       QString data = "<000/02\r";
       myCom->write(data.toUtf8(),data.size());
       Openflage=false;
    }

}

/**************************************************************
 * on_change_clicked() 本远控切换
**************************************************************/
void MainWindow::on_change_clicked()
{
    if (flage == 2)
    {
         ui->label_antstate->setText("远控");
         Insertlog("控制模式切换为远控");
         ui->groupBox_WorkMode->setEnabled(false);
         ui->groupBox_Pro->setEnabled(false);
         ui->stackedWidget_2->setEnabled(false);
         ui->starstop->setEnabled(false);
         ui->ACU->setEnabled(false);
         ui->record->setEnabled(false);
         ui->Cal->setEnabled(false);
         ui->stow->setEnabled(false);
         ui->read->setEnabled(false);
         ui->SourceCode->setEnabled(false);
         flage=1;
    }
    else if(flage==1)
    {
        ui->label_antstate->setText("本控");
        Insertlog("控制模式切换为本控");
        ui->groupBox_WorkMode->setEnabled(true);
        ui->groupBox_Pro->setEnabled(true);
        ui->stackedWidget_2->setEnabled(true);
        ui->starstop->setEnabled(true);
        ui->ACU->setEnabled(true);
        ui->record->setEnabled(true);
        ui->Cal->setEnabled(true);
        ui->stow->setEnabled(true);
        ui->read->setEnabled(true);
        ui->SourceCode->setEnabled(true);
        flage=2;
    }
}

/**************************************************************
 * on_Cal_clicked() 标校界面切换
**************************************************************/
void MainWindow::on_Cal_clicked()
{
//    flage=3;
    ui->stackedWidget->setCurrentIndex(1);
    ui->label_workmode->setText("标校");
    Insertlog("工作模式选择为标校");

}

/**************************************************************
 * on_home_clicked() 收藏
**************************************************************/
void MainWindow::on_stow_clicked()
{
    QString num,data;
    num= QString("%1").arg(170,7,'f',3,QChar('0'));
    data = QString("<000/04_0,+%0\r").arg(num);
    myCom->write(data.toUtf8(),data.size());
    num= QString("%1").arg(88,7,'f',3,QChar('0'));
    data = QString("<000/04_1,+%0\r").arg(num);
    myCom->write(data.toUtf8(),data.size());
}

/**************************************************************
 * on_record_clicked() 程引界面切换
**************************************************************/
void MainWindow::on_record_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
    ui->stackedWidget_3->setCurrentIndex(1);
    ui->label_workmode->setText("程引");
    Insertlog("工作模式选择为程引");
}

/**************************************************************
 * on_read_clicked() 读取日志文件
**************************************************************/
void MainWindow::on_read_clicked()
{
    QString path =QCoreApplication::applicationDirPath();
    path.append("/log");
    QDesktopServices::openUrl(QUrl(path));
}

/**************************************************************
 * on_exit_clicked() 退出
**************************************************************/
void MainWindow::on_exit_clicked()
{
    this->close();
}

/**************************************************************
 * on_ButtonSystemSettings_clicked() 系统设置
**************************************************************/
void MainWindow::on_ButtonSystemSettings_clicked()
{
    //m_settings.show();
    systemset sys(this);
    sys.exec();

}

/**************************************************************
 * on_ButtonELUp_clicked() 俯仰上移
**************************************************************/
void MainWindow::on_ButtonELUp_clicked()
{

    if(ui->RadioPosition->isChecked())
    {
        if(m_CliSend.m_init==false)
        {
            m_CliSend.m_Elinit=m_ACU.m_NowEl;//记录初始位置
            m_CliSend.m_Azinit=m_ACU.m_NowAz;
            m_CliSend.m_init=true;
        }

        if(ui->radioFirst->isChecked())
        {
            Insertlog("俯仰向上移动0.06");
            m_CliSend.m_Elpos +=0.06;
        }
        else if(ui->radioSecond->isChecked())
        {
            Insertlog("俯仰向上移动0.12");
            m_CliSend.m_Elpos +=0.12;
        }
        else if(ui->RadioCustomsize->isChecked())
        {  
            QString data = ui->lineEditStep->text();
            Insertlog("俯仰向上移动"+data);
            m_CliSend.m_Elpos +=data.toDouble();
        }
        QString num;
        QString data;
        if(m_CliSend.m_Elpos+m_CliSend.m_Elinit>=0)
        {
            num= QString("%1").arg(m_CliSend.m_Elpos+m_CliSend.m_Elinit,7,'f',3,QChar('0'));
            data = QString("<000/04_1,+%0\r").arg(num);
        }
        else
        {
            num= QString("%1").arg(m_CliSend.m_Elpos+m_CliSend.m_Elinit,8,'f',3,QChar('0'));
            data = QString("<000/04_1,%0\r").arg(num);
        }

        //QString data = QString("<000/04_1,%1\r").arg(m_CliSend.m_Elpos+m_CliSend.m_Elinit);
        if(m_CliSend.m_Elpos+m_CliSend.m_Elinit<0||m_CliSend.m_Elpos+m_CliSend.m_Elinit>=90)
        {
            Message(1,1,"超出范围,请重试" );
        }
        else
        {
            myCom->write(data.toUtf8(),data.size());
            QString num = QString("%1").arg(m_CliSend.m_Elpos,7,'f',3,'0');
            ui->lineEdit_ELOffsets->setText(num+"°");
//            m_CliSend.m_init=false;
        }
     }
    else if(ui->RadioVel->isChecked())
    {
        if(m_CliSend.m_init==false)
        {
            m_CliSend.m_ElVelinit=m_ACU.m_NowElVel;//记录初始位置
            m_CliSend.m_AzVelinit=m_ACU.m_NowAzVel;
            m_CliSend.m_init=true;
        }
        if(ui->radioFirst->isChecked())
        {
            Insertlog("俯仰向上移动0.06");
            m_CliSend.m_ElVel +=0.06;
        }
        else if(ui->radioSecond->isChecked())
        {
            Insertlog("俯仰向上移动0.12");
            m_CliSend.m_ElVel +=0.12;
        }
        else if(ui->RadioCustomsize->isChecked())
        {
            QString data = ui->lineEditStep->text();
            Insertlog("俯仰向上移动"+data);
            m_CliSend.m_ElVel +=data.toDouble();
        }
        QString num;
        QString data;
        if(m_CliSend.m_ElVel+m_CliSend.m_ElVelinit>=0)
        {
            num= QString("%1").arg(m_CliSend.m_ElVel+m_CliSend.m_ElVelinit,5,'f',3,QChar('0'));
            data = QString("<000/03_1,+%1\r").arg(num);
        }
        else
        {
            num= QString("%1").arg(m_CliSend.m_ElVel+m_CliSend.m_ElVelinit,6,'f',3,QChar('0'));
            data = QString("<000/03_1,%1\r").arg(num);
        }
        if(m_CliSend.m_ElVel+m_CliSend.m_ElVelinit<-1||m_CliSend.m_ElVel+m_CliSend.m_ElVelinit>1)
        {
            Message(1,1,"超出范围,请重试" );
        }
        else
        {
            myCom->write(data.toUtf8(),data.size());
            QString num = QString("%1").arg(m_CliSend.m_ElVel,6,'f',3,'0');
            ui->lineEdit_ELOffsets->setText(num+"°/sec");
//            m_CliSend.m_init=false;
        }
    }

}

/**************************************************************
 * on_ButtonELUp_clicked() 俯仰下移
**************************************************************/
void MainWindow::on_ButtonElDown_clicked()
{
    if(ui->RadioPosition->isChecked())
    {
        if(m_CliSend.m_init==false)
        {
            m_CliSend.m_Azinit=m_ACU.m_NowAz;
            m_CliSend.m_Elinit=m_ACU.m_NowEl;//记录初始位置
            m_CliSend.m_init=true;
        }

        if(ui->radioFirst->isChecked())
        {
            Insertlog("俯仰向下移动0.06");
            m_CliSend.m_Elpos -=0.06;
        }
        else if(ui->radioSecond->isChecked())
        {
            Insertlog("俯仰向下移动0.12");
            m_CliSend.m_Elpos -=0.12;
        }
        else if(ui->RadioCustomsize->isChecked())
        {
            QString data = ui->lineEditStep->text();
            Insertlog("俯仰向下移动"+data);
            m_CliSend.m_Elpos -=data.toDouble();
        }
        QString num;
        QString data;
        if(m_CliSend.m_Elpos+m_CliSend.m_Elinit>=0)
        {
            num= QString("%1").arg(m_CliSend.m_Elpos+m_CliSend.m_Elinit,7,'f',3,QChar('0'));
            data= QString("<000/04_1,+%0\r").arg(num);
        }
        else
        {
            num= QString("%1").arg(m_CliSend.m_Elpos+m_CliSend.m_Elinit,8,'f',3,QChar('0'));
            data= QString("<000/04_1,-%0\r").arg(num);
        }
        //QString data = QString("<000/04_1,%1\r").arg(m_CliSend.m_Elpos+m_CliSend.m_Elinit);
        if(m_CliSend.m_Elpos+m_CliSend.m_Elinit<0||m_CliSend.m_Elpos+m_CliSend.m_Elinit>=90)
        {
            Message(1,1,"超出范围,请重试" );
        }
        else
        {
            qDebug()<<data;
            myCom->write(data.toUtf8(),data.size());
            QString num = QString("%1").arg(m_CliSend.m_Elpos,7,'f',3,'0');
            ui->lineEdit_ELOffsets->setText(num+"°");
//            m_CliSend.m_init=false;
        }
     }
    else if(ui->RadioVel->isChecked())
    {
        if(m_CliSend.m_init==false)
        {
            m_CliSend.m_AzVelinit=m_ACU.m_NowAzVel;
            m_CliSend.m_ElVelinit=m_ACU.m_NowElVel;//记录初始位置
            m_CliSend.m_init=true;
        }

        if(ui->radioFirst->isChecked())
        {
            Insertlog("俯仰向下移动0.06");
            m_CliSend.m_ElVel -=0.06;
        }
        else if(ui->radioSecond->isChecked())
        {
            Insertlog("俯仰向下移动0.12");
            m_CliSend.m_ElVel -=0.12;
        }
        else if(ui->RadioCustomsize->isChecked())
        {
            QString data = ui->lineEditStep->text();
            Insertlog("俯仰向下移动"+data);
            m_CliSend.m_ElVel -=data.toDouble();
        }
        QString num;
        QString data;
        if(m_CliSend.m_ElVel+m_CliSend.m_ElVelinit>=0)
        {
            num= QString("%1").arg(m_CliSend.m_ElVel+m_CliSend.m_ElVelinit,5,'f',3,QChar('0'));
            data = QString("<000/03_1,+%1\r").arg(num);
        }
        else
        {
            num= QString("%1").arg(m_CliSend.m_ElVel+m_CliSend.m_ElVelinit,6,'f',3,QChar('0'));
            data = QString("<000/03_1,%1\r").arg(num);
        }

        //QString data = QString("<000/03_1,%1\r").arg(m_CliSend.m_ElVel+m_CliSend.m_ElVelinit);
        if(m_CliSend.m_ElVel+m_CliSend.m_ElVelinit<-1||m_CliSend.m_ElVel+m_CliSend.m_ElVelinit>1)
        {
            Message(1,1,"超出范围,请重试" );
        }
        else
        {
            myCom->write(data.toUtf8(),data.size());
            QString num = QString("%1").arg(m_CliSend.m_ElVel,6,'f',3,'0');
            ui->lineEdit_ELOffsets->setText(num+"°/sec");
//            m_CliSend.m_init=false;
        }

    }
}

/**************************************************************
 * on_ButtonAzLeft_clicked() 方位左移
**************************************************************/
void MainWindow::on_ButtonAzLeft_clicked()
{
    if(ui->RadioPosition->isChecked())
    {
        if(m_CliSend.m_init==false)
        {
            m_CliSend.m_Azinit=m_ACU.m_NowAz;//记录初始位置
            m_CliSend.m_Elinit=m_ACU.m_NowEl;
            m_CliSend.m_init=true;
        }

        if(ui->radioFirst->isChecked())
        {
            Insertlog("方位向左移动0.06");
            m_CliSend.m_Azpos -=0.06;
        }
        else if(ui->radioSecond->isChecked())
        {
            Insertlog("方位向左移动0.12");
            m_CliSend.m_Azpos -=0.12;
        }
        else if(ui->RadioCustomsize->isChecked())
        {

            QString data = ui->lineEditStep->text();
            Insertlog("方位向左移动"+data);
            m_CliSend.m_Azpos -=data.toDouble();
        }
        QString num;
        QString data;
        if(m_CliSend.m_Azpos+m_CliSend.m_Azinit>=0)
        {
            num= QString("%1").arg(m_CliSend.m_Azpos+m_CliSend.m_Azinit,7,'f',3,QChar('0'));
            data = QString("<000/04_0,+%1\r").arg(num);
        }
        else
        {
            num= QString("%1").arg(m_CliSend.m_Azpos+m_CliSend.m_Azinit,8,'f',3,QChar('0'));

            data = QString("<000/04_0,%1\r").arg(num);
        }
        //QString data = QString("<000/04_0,%1\r").arg(m_CliSend.m_Azpos+m_CliSend.m_Azinit);
        if(m_CliSend.m_Azpos+m_CliSend.m_Azinit<-180||m_CliSend.m_Azpos+m_CliSend.m_Azinit>180)
        {
            Message(1,1,"超出范围,请重试" );
        }
        else
        {
            myCom->write(data.toUtf8(),data.size());
            QString num = QString("%1").arg(m_CliSend.m_Azpos,7,'f',3,'0');
            ui->lineEdit_AZOffsets->setText(num+"°");
//            m_CliSend.m_init=false;
        }
     }
    else if(ui->RadioVel->isChecked())
    {
        if(m_CliSend.m_init==false)
        {
            m_CliSend.m_AzVelinit=m_ACU.m_NowAzVel;//记录初始位置
            m_CliSend.m_ElVelinit=m_ACU.m_NowElVel;
            m_CliSend.m_init=true;
        }
        if(ui->radioFirst->isChecked())
        {
            Insertlog("方位向左移动0.06");
            m_CliSend.m_AzVel -=0.06;
        }
        else if(ui->radioSecond->isChecked())
        {
            Insertlog("方位向左移动0.12");
            m_CliSend.m_AzVel -=0.12;
        }
        else if(ui->RadioCustomsize->isChecked())
        {

            QString data = ui->lineEditStep->text();
            Insertlog("方位向左移动"+data);
            m_CliSend.m_AzVel -=data.toDouble();
        }
        QString num;
        QString data;
        if(m_CliSend.m_AzVel+m_CliSend.m_AzVelinit>=0)
        {
            num= QString("%1").arg(m_CliSend.m_AzVel+m_CliSend.m_AzVelinit,5,'f',3,QChar('0'));
            data= QString("<000/03_0,+%1\r").arg(num);
        }
        else
        {
            num= QString("%1").arg(m_CliSend.m_AzVel+m_CliSend.m_AzVelinit,6,'f',3,QChar('0'));
            data= QString("<000/03_0,%1\r").arg(num);
        }
        //QString data = QString("<000/03_0,%1\r").arg(m_CliSend.m_AzVel+m_CliSend.m_AzVelinit);
        if(m_CliSend.m_AzVel+m_CliSend.m_AzVelinit<-1||m_CliSend.m_AzVel+m_CliSend.m_AzVelinit>1)
        {
            Message(1,1,"超出范围,请重试" );
        }
        else
        {
            myCom->write(data.toUtf8(),data.size());
            QString num = QString("%1").arg(m_CliSend.m_AzVel,6,'f',3,'0');
            ui->lineEdit_AZOffsets->setText(num+"°/sec");
//            m_CliSend.m_init=false;
        }
    }
}

/**************************************************************
 * on_ButtonAzRight_clicked() 方位右移
**************************************************************/
void MainWindow::on_ButtonAzRight_clicked()
{
    if(ui->RadioPosition->isChecked())
    {
        if(m_CliSend.m_init==false)
        {
            m_CliSend.m_Azinit=m_ACU.m_NowAz;//记录初始位置
            m_CliSend.m_Elinit=m_ACU.m_NowEl;
            m_CliSend.m_init=true;
        }

        if(ui->radioFirst->isChecked())
        {
            Insertlog("方位向右移动0.06");
            m_CliSend.m_Azpos +=0.06;
        }
        else if(ui->radioSecond->isChecked())
        {
            Insertlog("方位向右移动0.12");
            m_CliSend.m_Azpos +=0.12;
        }
        else if(ui->RadioCustomsize->isChecked())
        {
            QString data = ui->lineEditStep->text();
            Insertlog("方位向右移动"+data);
            m_CliSend.m_Azpos +=data.toDouble();
        }
        QString num;
        QString data ;
        if(m_CliSend.m_Azpos+m_CliSend.m_Azinit>=0)
        {
            num= QString("%1").arg(m_CliSend.m_Azpos+m_CliSend.m_Azinit,7,'f',3,QChar('0'));
            data = QString("<000/04_0,+%1\r").arg(num);
        }
        else
        {
            num= QString("%1").arg(m_CliSend.m_Azpos+m_CliSend.m_Azinit,8,'f',3,QChar('0'));
            data = QString("<000/04_0,%1\r").arg(num);
        }

        //QString data = QString("<000/04_0,%1\r").arg(m_CliSend.m_Azpos+m_CliSend.m_Azinit);
        if(m_CliSend.m_Azpos+m_CliSend.m_Azinit<-180||m_CliSend.m_Azpos+m_CliSend.m_Azinit>180)
        {
            Message(1,1,"超出范围,请重试" );
        }
        else
        {
            myCom->write(data.toUtf8(),data.size());
            QString num = QString("%1").arg(m_CliSend.m_Azpos,7,'f',3,'0');
            ui->lineEdit_AZOffsets->setText(num+"°");
//            m_CliSend.m_init=false;
        }
     }
    else if(ui->RadioVel->isChecked())
    {
        if(m_CliSend.m_init==false)
        {
            m_CliSend.m_AzVelinit=m_ACU.m_NowAzVel;//记录初始位置
            m_CliSend.m_ElVelinit=m_ACU.m_NowElVel;
            m_CliSend.m_init=true;
        }
        if(ui->radioFirst->isChecked())
        {
            Insertlog("方位向右移动0.06");
            m_CliSend.m_AzVel +=0.06;
        }
        else if(ui->radioSecond->isChecked())
        {
            Insertlog("方位向右移动0.12");
            m_CliSend.m_AzVel +=0.12;
        }
        else if(ui->RadioCustomsize->isChecked())
        {
            QString data = ui->lineEditStep->text();
            Insertlog("方位向右移动"+data);
            m_CliSend.m_AzVel +=data.toDouble();
        }
        QString num;
        QString data;
        if(m_CliSend.m_AzVel+m_CliSend.m_AzVelinit>=0)
        {
            num= QString("%1").arg(m_CliSend.m_AzVel+m_CliSend.m_AzVelinit,5,'f',3,QChar('0'));
            data = QString("<000/03_0,+%1\r").arg(num);
        }
        else
        {
            num= QString("%1").arg(m_CliSend.m_AzVel+m_CliSend.m_AzVelinit,6,'f',3,QChar('0'));
            data = QString("<000/03_0,%1\r").arg(num);
        }
        //QString data = QString("<000/03_0,%1\r").arg(m_CliSend.m_AzVel+m_CliSend.m_AzVelinit);
        if(m_CliSend.m_AzVel+m_CliSend.m_AzVelinit<-1||m_CliSend.m_AzVel+m_CliSend.m_AzVelinit>1)
        {
            Message(1,1,"超出范围,请重试" );
        }
        else
        {
            myCom->write(data.toUtf8(),data.size());
            QString num = QString("%1").arg(m_CliSend.m_AzVel,6,'f',3,'0');
            ui->lineEdit_AZOffsets->setText(num+"°/sec");
//            m_CliSend.m_init=false;
        }
    }
}

/**************************************************************
 * on_ButtonELUp_clicked() 功能按键区回初始位置
**************************************************************/
void MainWindow::on_ButtonZero_clicked()
{
    Insertlog("回零操作");
    if(ui->RadioPosition->isChecked())
    {
        if(m_CliSend.m_Azinit>=0)
        {
            QString num;
            num= QString("%1").arg(m_CliSend.m_Azinit,7,'f',3,QChar('0'));
            QString data = QString("<000/04_0,+%1\r").arg(num);
            myCom->write(data.toUtf8(),data.size());
        }
        else
        {
            QString num;
            num= QString("%1").arg(m_CliSend.m_Azinit,8,'f',3,QChar('0'));
            QString data = QString("<000/04_0,%1\r").arg(num);
            myCom->write(data.toUtf8(),data.size());
        }
        if(m_CliSend.m_Elinit>=0)
        {
            QString num;
            num= QString("%1").arg(m_CliSend.m_Elinit,7,'f',3,QChar('0'));
            QString data = QString("<000/04_1,+%1\r").arg(num);
            myCom->write(data.toUtf8(),data.size());
        }
        else
        {
            QString num;
            num= QString("%1").arg(m_CliSend.m_Elinit,8,'f',3,QChar('0'));
            QString data = QString("<000/04_1,%1\r").arg(num);
            myCom->write(data.toUtf8(),data.size());
        }
        ui->lineEdit_AZOffsets->setText(QString::number(000.000)+"°");
        ui->lineEdit_ELOffsets->setText(QString::number(000.000)+"°");
    }
    else if(ui->RadioVel->isChecked())
    {
        if(m_CliSend.m_AzVelinit>=0)
        {
            QString num;
            num= QString("%1").arg(m_CliSend.m_AzVelinit,5,'f',3,QChar('0'));
            QString data = QString("<000/03_0,+%1\r").arg(num);
            myCom->write(data.toUtf8(),data.size());
        }
        else
        {
            QString num;
            num= QString("%1").arg(m_CliSend.m_AzVelinit,6,'f',3,QChar('0'));
            QString data = QString("<000/03_0,%1\r").arg(num);
            myCom->write(data.toUtf8(),data.size());
        }
        if(m_CliSend.m_ElVelinit>=0)
        {
            QString num;
            num= QString("%1").arg(m_CliSend.m_ElVelinit,5,'f',3,QChar('0'));
            QString data = QString("<000/03_1,+%1\r").arg(num);
            myCom->write(data.toUtf8(),data.size());
        }
        else
        {
            QString num;
            num= QString("%1").arg(m_CliSend.m_ElVelinit,6,'f',3,QChar('0'));
            QString data = QString("<000/03_1,%1\r").arg(num);
            myCom->write(data.toUtf8(),data.size());
        }
        ui->lineEdit_AZOffsets->setText(QString::number(000.000)+"°/sec");
        ui->lineEdit_ELOffsets->setText(QString::number(000.000)+"°/sec");
    }
    m_CliSend.m_Azpos=0;
    m_CliSend.m_Elpos=0;
    m_CliSend.m_AzVel=0;
    m_CliSend.m_ElVel=0;
    m_CliSend.m_init=false;
}

/**************************************************************
 * on_ACU_clicked() ACU界面切换
**************************************************************/
void MainWindow::on_ACU_clicked()
{
    Insertlog("切换位ACU界面");
    ui->stackedWidget->setCurrentIndex(0);
    ui->stackedWidget_3->setCurrentIndex(0);
    if(ui->RadioPosition->isChecked())
    {
        ui->label_workmode->setText("指向");
    }
    else if(ui->RadioVel->isChecked())
    {
        ui->label_workmode->setText("速度");
    }
}
/**************************************************************
 * on_RadioPosition_clicked()跟新界面状态
**************************************************************/
void MainWindow::on_RadioPosition_clicked()
{
    chekstate();
    ui->label_workmode->setText("指向");
    Insertlog("工作模式选择为指向");
}

void MainWindow::on_RadioVel_clicked()
{
   chekstate();
   ui->label_workmode->setText("速度");
   Insertlog("工作模式选择为速度");
}
/**************************************************************
 * on_ButtonMove_clicked()开始移动天线到最终位置
**************************************************************/
void MainWindow::on_ButtonMove_clicked()
{
    Insertlog("开始移动到指定位置");
    if(ui->lineEdit_AZ->text().length()>0)
    {
        if(ui->lineEdit_AZ->text().toDouble()>=-360&&ui->lineEdit_AZ->text().toDouble()<=360)
        {
            QString data;
            QString num;
            if(ui->lineEdit_EL->text().toDouble()>=0)
            {
                num= QString("%1").arg(ui->lineEdit_AZ->text().toDouble(),7,'f',3,QChar('0'));
                data = QString("<000/04_0,+%0\r").arg(num);
            }
            else
            {
                num= QString("%1").arg(ui->lineEdit_AZ->text().toDouble(),8,'f',3,QChar('0'));
                data = QString("<000/04_0,-%0\r").arg(num);
            }
            myCom->write(data.toUtf8(),data.size());
        }
        else
        {
            Message(2,1,"方位范围超出，请重新设置!");
        }
    }
    if(ui->lineEdit_EL->text().length()>0)
    {
        if(ui->lineEdit_EL->text().toDouble()>=0&&ui->lineEdit_EL->text().toDouble()<=90)
        {
            QString data;
            if(ui->lineEdit_EL->text().toDouble()>=0)
            {

                 QString num;
                 num= QString("%1").arg(ui->lineEdit_EL->text().toDouble(),7,'f',3,QChar('0'));
                 data = QString("<000/04_1,+%0\r").arg(num);
            }
            else
            {
                QString num;
                num= QString("%1").arg(ui->lineEdit_EL->text().toDouble(),8,'f',3,QChar('0'));
                data = QString("<000/04_1,%0\r").arg(num);
            }
            myCom->write(data.toUtf8(),19);
        }
        else
        {
            Message(2,1,"俯仰范围超出，请重新设置!");
        }
    }
    if(ui->lineEdit_AZ->text().length()==0&&ui->lineEdit_EL->text().length()==0)
    {
         Message(2,1,"请设置方位俯仰!");
    }

}
/**************************************************************
 * on_ButtonStop_clicked()停止移动天线到最终位置
**************************************************************/
void MainWindow::on_ButtonStop_clicked()
{
    Insertlog("停止移动到指定位置");
    QString data = QString("<000/99'\r'").arg(ui->lineEdit_EL->text().toDouble());
    myCom->write(data.toUtf8(),data.size());
}

/**************************************************************
 * 点击自定义按钮， 开启文本框， 否则禁用
**************************************************************/
void MainWindow::on_RadioCustomsize_clicked()
{
    ui->lineEditStep->setEnabled(true);
}

void MainWindow::on_radioFirst_clicked()
{
    ui->lineEditStep->setEnabled(false);
}

void MainWindow::on_radioSecond_clicked()
{
    ui->lineEditStep->setEnabled(false);
}
/**************************************************************
 * on_ButtonChooseFile_clicked()打开程引文件
**************************************************************/
void MainWindow::on_ButtonChooseFile_clicked()
{
    Insertlog("打开程引文件");
    ProFileName = QFileDialog::getOpenFileName(nullptr,tr("文件"),"",tr("(*.txt)"));//获取整个文件名称
    ui->lineEditProFile->setText(ProFileName);
    ReadProfile();
}
/**************************************************************
 * on_ButtonProgramStart_clicked()开始执行程引文件
**************************************************************/
void MainWindow::on_ButtonProgramStart_clicked()
{
//   chart->removeSeries(seriesAZ);
//   chart->removeSeries(seriesEL);
    initQChart();
//    Prostate->setTimerType(Qt::PreciseTimer);
//    Prostate->start(20);
   ProsendState=true;
 //   Prodatasend->run();
    mycomthread->start();

    Insertlog("程引开始");
}
/**************************************************************
 * on_read_2_clicked()源码
**************************************************************/
void MainWindow::on_SourceCode_clicked()
{
    Insertlog("查看源码");
    Code *code = new Code(this);
    code->exec();
}
/**************************************************************
 * on_ButtonProgramStop_clicked()停止执行程引文件
**************************************************************/
void MainWindow::on_ButtonProgramStop_clicked()
{
    qDebug()<<"**************stop***********";
    Insertlog("程引停止");
    mycomthread->terminate();
    //mycomthread->wait();
  //  mycomthread->deleteLater();
 //   mycomthread->quit();
//    mycomthread->wait();
//    delete mycomthread;
}
/**************************************************************
 * on_Standby_clicked()待机
**************************************************************/
void MainWindow::on_Standby_clicked()
{
    QString data = QString("<000/99\r");
    myCom->write(data.toUtf8(),data.size());
}
/**************************************************************
 * on_pushButton_CalFlag_clicked()开始/停止标校
**************************************************************/
void MainWindow::on_pushButton_CalFlag_clicked()
{
    if(ui->pushButton_CalFlag->text()=="开始标校")
    {
        CalFlag=true;
        ui->pushButton_CalFlag->setText("停止标校");
    }
    else if(ui->pushButton_CalFlag->text()=="停止标校")
    {
        CalFlag=false;
        ui->pushButton_CalFlag->setText("开始标校");
    }

}
/**************************************************************
 * on_pushButton_clicked()保存标校参数
**************************************************************/
void MainWindow::on_pushButton_clicked()
{
    QString path =QCoreApplication::applicationDirPath();
    path.append("/Calparameter.txt");
    QFile File(path);
    QString data = QString("%1\t%2\t%3\t%4\t%5\t%6\t%7\t%8\t%9\t%10\t%11\t%12\t%13\t%14")
            .arg(ui->comboBox_Flag->currentText())
            .arg(ui->comboBox_ForecastMode->currentText())
            .arg(ui->comboBox_ConvoluFlag->currentText())
            .arg(ui->comboBox_ScanMode->currentText())
            .arg(ui->comboBox_RadioStar->currentText())
            .arg(ui->lineEdit_ScanRange->text())
            .arg(ui->lineEdit_ScanVel->text())
            .arg(ui->lineEdit_AppointAZ->text())
            .arg(ui->lineEdit_AppointEL->text())
            .arg(ui->lineEdit_temperature->text())
            .arg(ui->lineEdit_Humidity->text())
            .arg(ui->lineEdit_Pressure->text())
            .arg(ui->comboBox_Frequency->currentText())
            .arg(ui->lineEdit_CalTime->text());
    QTextStream parameter(&File);
    if(File.open(QIODevice::ReadWrite|QIODevice::Append|QIODevice::Text))
    {
        parameter<<data<<endl;
        File.close();
    }

}
double MainWindow::GetAE(double dX,double dY,double dZ,bool bAZ)
{
    //double pA,double pE;
    double a,e;								//
    double f_latitude;//当地经度
    double f_longitude;//当地纬度
    double f_height;//当地海拔
    double xh;
    double yh;
    double zh;
    double f_xg;
    double f_yg;
    double f_zg;
    double m_dlatitude,m_dlongitude,m_dheight;//本站经度维度
//   StructGPS gps_data = m_databus.GetGpsData();
    m_dlatitude =38.25518035888672;// 维度
    m_dlongitude = 114.36785455322264;//经度
    m_dheight = 121.7193603515625;//海拔

    f_latitude=m_dlatitude*3.1415926/180.0;
    f_longitude=m_dlongitude*3.1415926/180.0;
    f_height=m_dheight;

    double g1,g2;		//
    g1=re0/sqrt(1-(2*f_l-f_l*f_l)*sin(f_latitude)*sin(f_latitude))+f_height;
    g2=re0*(1-f_l)*(1-f_l)/sqrt(1-(2*f_l-f_l*f_l)*sin(f_latitude)*sin(f_latitude))+f_height;

    f_xg=g1*cos(f_latitude)*cos(f_longitude);
    f_yg=g1*cos(f_latitude)*sin(f_longitude);
    f_zg=g2*sin(f_latitude);
    double f_x=0.0,f_y=0.0,f_z=0.0;
    double m_dR=0;

    f_x=dX-f_xg;
    f_y=dY-f_yg;
    f_z=dZ-f_zg;

    m_dR=sqrt(f_x*f_x+f_y*f_y+f_z*f_z);

    xh=-(sin(f_latitude)*cos(f_longitude)*f_x+sin(f_latitude)*sin(f_longitude)*f_y-cos(f_latitude)*f_z);
    yh=-(sin(f_longitude)*f_x-cos(f_longitude)*f_y);
    zh= cos(f_latitude)*cos(f_longitude)*f_x+cos(f_latitude)*sin(f_longitude)*f_y+sin(f_latitude)*f_z;

    if(xh==0)								//,-
    {
        if(yh==0)							//=90,
        {
            a=90;
            e=90;
        }
        else if(yh>0)						//
        {
            a=90;
            e=atan(zh/yh);
            e=e*180.0/PI;
        }
        else								////A=270,
        {
            a=270;
            e=atan(-zh/yh);
            e=e*180.0/PI;
        }
    }
    else									//
    {
        //atanº¯ÊýÖ»ÄÜµÃ³ö(-90)--(+90)Ö®¼äµÄÖµ
        //-90£¨270)Óë+90µÄÇé¿öÉÏÒÑ·Ö±ðÌÖÂÛ
        //ÒÔÏÂ½«·Ö±ðÌÖÂÛa=0(180)¡¢a>0¡¢a<0µÄÇé¿ö

        a=atan(yh/xh);	//
        a=a*180.0/PI;
        if(a==0)
        {
            if(xh>0)
            {
                a=0;
                e=atan(zh/xh);
                e=e*180.0/PI;
            }
            else
            {
                a=180;
                e=atan(-zh/xh);
                e=e*180.0/PI;
            }
        }
        else if(a>0)
        {
            if(yh<0)			//ÈýÏóÏÞ
            {
                a=180+a;
                e=atan(sin(a*PI/180.0)*zh/yh);
                e=e*180.0/PI;
            }
            else if(yh>0)		//Ò»ÏóÏÞ
            {
                a=a;
                e=atan(sin(a*PI/180.0)*zh/yh);
                e=e*180.0/PI;
            }
        }
        else	//a<0Çé¿ö
        {
            if(xh>0)			//ËÄÏóÏÞ
            {
                a=360+a;
                e=atan(sin(a*PI/180.0)*zh/yh);
                e=e*180.0/PI;
            }
            else				//¶þÏóÏÞ
            {
                a=180+a;
                e=atan(sin(a*PI/180.0)*zh/yh);
                e=e*180.0/PI;
            }
        }//a<0Çé¿öend
    }//

    if (a < 0)
    {
        a += 360.0;
    }
    else if (a > 360)
    {
        a -= 360.0;
    }

    if (e < 0)
    {
        e = 0;
         // e=-e;
    }
    else if (e > 89.999)
    {
        e = 89.999;
    }

    if (bAZ)
    {
      //  qDebug()<<"AZAZAZAZAZAZA"<<a;
        return a;
    }
    else
    {
     //   qDebug()<<"ELELELLELEEL"<<e;

        return e;
    }
    return 0.0;
}
