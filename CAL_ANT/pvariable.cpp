#include "pvariable.h"
#include <QFile>
#include <QTextCodec>
#include <QDomDocument>
#include <QTextStream>

ACUState m_ACU;//ACU
CalibrationState m_Cal;//标效
systemsetdata m_settings;//系统参数
ClickedSend m_CliSend;//点击按钮发送指令
Prodata m_Prodata;//程引文件数据
bool ProsendState;//发送程引标识，false为不发送 true为发送
RemoteState m_Remote;//远控参数
QMutex m_ACULock;//用于ACU通信上锁
QMutex m_CalLock;//用于标效通信上锁
CalQueryPsrameter m_CalQuery;//标校查询参数
int  flage;//1 - 本控  2 - 远控  3 - 标校
std::list<QStringList>m_SourceCode;
//std::list<Prodata>m_Prodatanode;
QList<Prodata>m_Prodatanode;
Win_QextSerialPort* myCom;
bool CalFlag;
#pragma execution_character_set("utf-8")
PVariable::PVariable()
{

}
/**************************************************************
 * readXML() 读取XML文件配置参数并设置在全局变量中
**************************************************************/
void readXML()
{
    m_settings.m_DefPath = QCoreApplication::applicationDirPath();//获取程序所在目录
    m_settings.m_SysSettingsFile = QString("%1/cal_ant.xml").arg(m_settings.m_DefPath);
    QFile file(m_settings.m_SysSettingsFile);
   if(!file.open(QIODevice::ReadOnly))
   {
        file.close();
        return;
   }

     QDomDocument doc;
       if(!doc.setContent(&file))
       {
           qDebug()<<QString::fromLocal8Bit("打开失败");
           file.close();
           return;
       }
       //file.close();
       //系统设置界面初始化数据之电源通道设置
       int LoadPort = doc.elementsByTagName("本地端口").at(0).toElement().text().toInt();
       m_settings.m_LocalPort=LoadPort;
       QString LocalConIp = doc.elementsByTagName("本地组播Ip").at(0).toElement().text();
       m_settings.m_localConIp=LocalConIp;
       int LocalConPort = doc.elementsByTagName("本地组播端口").at(0).toElement().text().toInt();
       m_settings.m_LocalConPort=LocalConPort;
       QString ACUIp = doc.elementsByTagName("伺服IP").at(0).toElement().text();
       m_settings.m_AntennaIp=QHostAddress(ACUIp);
       int ACUPort = doc.elementsByTagName("伺服端口").at(0).toElement().text().toInt();
       m_settings.m_AntennaPort=ACUPort;
       QString RemoteConIP = doc.elementsByTagName("远控组播IP").at(0).toElement().text();
       m_settings.m_RemoteConIP=QHostAddress(RemoteConIP);
       int RemoteConPort = doc.elementsByTagName("远控组播端口").at(0).toElement().text().toInt();
       m_settings.m_RemoteConPort=RemoteConPort;
       QString CalIp = doc.elementsByTagName("标校IP").at(0).toElement().text();
       m_settings.m_CalibrationIp=QHostAddress(CalIp);
       int CalPort = doc.elementsByTagName("标校端口").at(0).toElement().text().toInt();
       m_settings.m_CalibrationPort=CalPort;
       //系统设置界面初始化数据之系统设置
       QString FtpIp = doc.elementsByTagName("FTPIP").at(0).toElement().text();
       m_settings.m_FtpIp=FtpIp;
       int FtpPort = doc.elementsByTagName("FTP端口").at(0).toElement().text().toInt();
       m_settings.m_FtpPort=FtpPort;
       QString UserName = doc.elementsByTagName("用户名").at(0).toElement().text();
       m_settings.UserName=UserName;
       QString Password = doc.elementsByTagName("密码").at(0).toElement().text();
       m_settings.Password=Password;
       QString FTPPath = doc.elementsByTagName("FTP路径").at(0).toElement().text();
       m_settings.m_FtpPath=FTPPath;
       QString LocadPath = doc.elementsByTagName("本地路径").at(0).toElement().text();
       m_settings.m_DownPath=LocadPath;
}
/**************************************************************
 * Cal_MJD() MJD时间换算
**************************************************************/
double Cal_MJD(QDateTime UTC)
{
    double year,month,day,hour,minute,second,minsecond;
    float temday;
    year = UTC.date().year();
    month = UTC.date().month();
    day = UTC.date().day();
    hour = UTC.time().hour();
    minute = UTC.time().minute();
    second = UTC.time().second();
    minsecond = UTC.time().msec();

    temday = (hour*3600 + minute*60 + second + minsecond/1000)/86400;

    if (month <= 2)
    {
        month +=  12;
        year-= 1901;
    }
    else
    {
        year-= 1900;
    }
    double MJD = 14956 + int(365.25*year) + int(30.6001*(month + 1)) + day + temday;
    return MJD;
}
/*********************************************************************
 *                   MessageBox提示信息对话框
 * -type   对话框显示的提示图标，取值如下：
 *         1 - 提示图标
 *         2 - 警告图标
 * -num    对话框显示按键个数，取值如下：
 *         1 - 一个按键
 *         2 - 两个按键
 * -text  需要显示的文字信息
**********************************************************************/
void Message(int type,int num,QString text)
{
    QMessageBox * box;

    if(type <= 1)
    {

        box = new QMessageBox(QMessageBox::Information,"Message",text);
    }
    else if(type >= 2)
    {
        box = new QMessageBox(QMessageBox::Warning,"Message",text);
    }

    if(num <= 1)
    {
        box->setStandardButtons(QMessageBox::Ok);

    }
    else if(num >= 2)
    {
        box->setStandardButtons(QMessageBox::Ok|QMessageBox::Cancel);

    }
    box->exec();
    delete box;
}

QDateTime UTCTOLocalTime(QDateTime UTC)
{
    UTC.setTimeSpec(Qt::UTC);
    return   UTC.toLocalTime();
}
