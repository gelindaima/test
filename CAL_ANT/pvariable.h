#ifndef PVARIABLE_H
#define PVARIABLE_H

#include<QCoreApplication>
#include<QThread>
#include "udpthread.h"
#include<QUdpSocket>
#include<QUrl>
#include<QTimer>
#include<QTime>
#include<QList>
#include<QStringList>
#include<QMutex>
#include<QMessageBox>
#define Query_State "<000/00_2'\r'" //伺服查询命令解析不到就将cr替换成\r,其它命令相同
#define Query_DevState "<000/00_3'\r'" //驱动器详细状态查询命令
#define Query_PolState "<000/00_1'\r'" //极化状态查询命令
#define Query_RecState "<001/00_4'\r'" //接收机状态查询命令

struct systemsetdata
{
    int m_LocalPort = 5999;
    int m_LocalConPort;//本地组播端口
    QHostAddress m_localConIp;//本地组播IP
    QHostAddress m_AntennaIp;//acuIP
    int m_AntennaPort;
    QHostAddress m_CalibrationIp;//标效Ip
    int m_CalibrationPort;
    QHostAddress m_RemoteConIP;//远控组播IP
    int m_RemoteConPort;
    QHostAddress m_FtpIp;//FTPip
    int m_FtpPort;

    QString UserName;
    QString Password;
    QString m_FtpPath;//FTP保存文件目录
    QString m_DownPath;//本地保存目录
    QString m_DefPath;//程序所在目录
    QString m_SysSettingsFile;//系统配置文件
    systemsetdata()
    {
        m_DefPath = QCoreApplication::applicationDirPath();//获取程序所在目录
        m_SysSettingsFile = QString("%1/cal_ant.ini").arg(m_DefPath);
    }
};
//ACU状态
struct ACUState
{
   bool state=false;//是否执行标校 true - 执行 false - 不执行
   int m_ACUState=0;//ACU工作状态
   int m_AzVelState=1;//方位速度控制
   int m_AzPosState=1;//方位位置控制
   int m_AzCW=1;//方位顺限位
   int m_AzCCW=1;//方位逆限位
   int m_AzPerCW=1;//方位预顺限位
   int m_AzPerCCW=1;//方位预逆限位
   int m_AzEndLimit=1;//方位终限位
   int m_AzFirstDriver=1;//方位驱动1
   QStringList m_AzFirstDriverList;//方位1故障错误码
   int m_AzSecondDriver=1;//方位驱动2
   QStringList m_AzSecondDriverList;//方位2故障错误码
   int m_AzCodeDisk=1;//方位码盘
   int m_AzStop=1;//方位急停
   int m_AzPower=1;//方位上电
   int m_AzVoltage=1;//方位电压，0正常，1过电压
   double m_AzErrorSignal;//方位差信号
   int m_ElVelState=1;//俯仰速度控制
   int m_ElPosState=1;//俯仰位置控制
   int m_ElCW=1;//俯仰顺限位
   int m_ElCCW=1;//俯仰逆限位
   int m_ElPerCW=1;//俯仰预顺限位
   int m_ElPerCCW=1;//俯仰预逆限位
   int m_ElEndLimit=1;//俯仰终限位
   int m_ElFirstDriver=1;//俯仰驱动1
   QStringList m_ElFirstDriverList;//俯仰1故障错误码
   int m_ElSecondDriver=1;//俯仰驱动2
   QStringList m_ElSecondDriverList;//俯仰2故障错误码
   int m_ElLock=1;//俯仰入锁
   int m_ElUnlock=1;//俯仰退锁
   int m_ElStow=NULL;//俯仰收藏
   int m_ElCodeDisk=NULL;//俯仰码盘故障
   int m_ElPower=NULL;//俯仰上电
   int m_ElVoltage=NULL;//俯仰电压，0正常，1过电压
   double m_ElErrorSignal=NULL;//俯仰差信号
   int m_Tracksignal=NULL;//跟踪信号
   int m_InstructionErr=NULL;//下发命令格式错误
   int m_CommunicationState=NULL;//通信状态
   int m_signalLock=NULL;//信号锁定
   int m_AzState=NULL;
   int m_ElState=NULL;
   double m_RXLEV=NULL;//信号电平
   int m_NotDo=NULL;//拒绝执行
   int m_handset=NULL;//遥控方式
   int m_Clockwise=NULL;//顺时针
   int m_Anticlockwise=NULL;//逆时针
   int m_TurnUp=NULL;//天线正在向上转动
   int m_TurnDown=NULL;//天线正在向下转动
   int m_Pol=NULL;//极化; 1，极化1 2，极化2 3，极化3， 4，极化4
   double m_PolPos=NULL;//极化位置
   int m_PolSwitchCW=NULL;//极化开关顺时针限位
   int m_PolSwitchCCW=NULL;//极化开关逆时针限位
   int m_PolSoftwareCW=NULL;//极化软件顺时针限位
   int m_PolSoftwareCCW=NULL;//极化软件逆时针限位
   int m_PolPosState=NULL;//极化位置检测
   int m_PolControlState=NULL;//极化控制回路
   int m_PolClockwise=NULL;//极化顺转
   int m_PolAnticlockwise=NULL;//极化逆转
   int m_PolControlNotSet=NULL;//极化控制未设置
   int m_CodeType=NULL;//为 1 时为军码 P（0 为民码 C）
   int m_ControlType=NULL;//为 1 时遥控(0 为本控)
   double m_Signal=NULL;//信号
   double m_NowAz=NULL;//当前方位
   double m_NowEl=NULL;//当前俯仰
   double m_NowAzVel;
   double m_NowElVel;
};
//标校查询参数
struct CalQueryPsrameter
{
    int ForecastMode;//预报方式
    int ConvolutionFlage;//卷积标识
    int RadioStarNum;//射电星编号
    int ScanningModee;//扫描方式
    int ScanningRange;//扫描范围
    int ScanningSpeed;//扫描速度
    int SpecifyAZ;//指定方位
    int SpecifyEL;//指定俯仰
    int Temperature;//温度
    int Humidity;//湿度
    int Pressure;//气压
    int Frequency;//频率
    int Startyear,Startmonth,Startday,Starthour,Startmin,Startsec,Startmsec;//开始年月日时分秒毫秒
    int Endyear,Endmonth,Endday,Endhour,Endmin,Endsec,Endmsec;//结束年月日时分秒毫秒
    double  AZ;//方位
    double  EL;//俯仰
};
//标校状态
struct CalibrationState
{
//    1：生成标校规划文件正常
//    2：生成标校规划文件异常
//    3：等待标校开始时间到
//    4：正在标校
//    5：标校完成，结束
//    6：收到指令，结束标校
//    7：标校未完成，异常终止
    bool State=false;//是否执行标校 true - 执行 false - 不执行
    int m_CalibrationState=NULL;//标效状态
    int m_FileNumber=NULL;//射电源文件数量
    int m_RadioSourceNumber=NULL; //标效射电源个数
    int m_SpotNumber=NULL;//标效点个数
    int m_NowSport=NULL;//当前标效点
    int m_PositionState=NULL;//当前状态；0 - 未开始 1，开始 2，结束
    int m_PositionResult=NULL;//指向命令执行结果0：正常接收并执行；1：分控拒收；2：帧格式错误(未知命令)；3：被控对象不存在；4：参数错误；5：条件不具备； 6：未知原因失败
    QString m_RadioSourceName="";//当前射电源标识

    QString   m_NowSpotBeginDate;//当前标效点开始日期
    QString   m_NowSpotBeginTime="";//当前标效点开始时间
    QString   m_NowSpotEndDate="";//当前标效点结束日期
    QString   m_NowSpotEndTime="";//当前标效点结束时间

    QDateTime m_PositionBeginTime;//指向开始时间
    QDateTime m_PositionEndTime;//指向结束时间

    double m_NowAz=NULL;//星位引导方位
    double m_NowEl=NULL;//星位引导俯仰
    double m_AstralNowAz=NULL;//当前方位
    double m_AstralNowEl=NULL;//当前俯仰
    double m_UpTimeNowAz=NULL;//实时方位
    double m_UpTimeNowEl=NULL;//实时俯仰

    QDateTime m_AstralTime;//星位引导时间
    QDateTime m_UpTime;//实时角度中的当前时间

    int m_DataSize=NULL;//数据长度
    int m_SerialNumber=NULL;//流水号
    int m_SubsystemStatus=NULL;//分系统状态 0：无此参数 1：正常 2：故障 3：维护
    int m_ControlMode=NULL;//控制方式 0 ：本空；1：分控 2 ：无此参数
    int m_WorkMode=NULL;//工作状态 0：空闲 1：标校 2：无此参数

    int m_TrackPositionResult=NULL;//轨道预报控制结果
    double  m_TrackAZ[100];//轨道预报方位
    double  m_TrackEL[100];//轨道预报俯仰

    int m_Sweepident=NULL;//指向标识
    int m_SweepState=0;
    //指向状态1：生成标校规划文件正常
    //2：生成标校规划文件异常
    //3：等待标校开始时间到
    //4：正在标校
    //5：标校完成，结束
    //6：收到指令，结束标校
    //7：标校未完成，异常终止
    int m_SweepNumber=0;//指向个数
    int m_SweepCalNumber=0;//标校点个数
    int m_SweepNowNumber=0;//当前标校点标识
    QString m_NowName="";//当前目标名称

    QStringList m_FileName;//射电源文件名称
    QString m_StartTime="";//开始时间

};

struct ClickedSend
{
    bool m_init=false;//判定是否有初始值
    double m_Azinit=NULL;//AZ初始位子
    double m_Elinit=NULL;//EL初始位子
    double m_AzVelinit=NULL;
    double m_ElVelinit=NULL;
    double m_Azpos=NULL;//AZ现在的位子
    double m_Elpos=NULL;//EL现在的位子
    double m_AzVel=NULL;//AZ现在的速度
    double m_ElVel=NULL;//EL现在的速度

};
struct Prodata
{
    double AZ=NULL;
    double EL=NULL;
    double FirstAZ=NULL;
    double FirstEL=NULL;
    double AZAngle=NULL;
    double ELAngle=NULL;
    QDateTime Time;
    int number=NULL;
};
//远控状态
struct RemoteState
{
  int   m_ItemNumber;//顺序号
  int   m_RemoteResult;//控制结果
  QString m_remoteCalFilename;//远控标校文件名

};
void QueryState(int target);
void readXML();
double Cal_MJD(QDateTime UTC);
void Message(int type,int num,QString text);
QDateTime UTCTOLocalTime(QDateTime UTC);
class PVariable
{
public:
    PVariable();
};

#endif // PVARIABLE_H
