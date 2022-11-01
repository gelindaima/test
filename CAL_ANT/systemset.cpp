#include "systemset.h"
#include "ui_systemset.h"
#include "pvariable.h"
#include <QFile>
#include <QTextCodec>
#include <QDomDocument>
#include <QTextStream>
extern systemsetdata m_settings;
#pragma execution_character_set("utf-8")
systemset::systemset(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::systemset)
{
    ui->setupUi(this);
    ui->Password->setEchoMode(QLineEdit::Password);
    initwidget();
    readXML();
}

systemset::~systemset()
{
    delete ui;
}
/*********************************************************************************************
 * updataXML() 创建XML文件
**********************************************************************************************/
void systemset::updataXML()
{

    //QTextCodec::setCodecForLocale(QTextCodec::codecForLocale());
     // QTextCodec::setCodecForLocale(QTextCodec::codecForName("gb18030"));
      //filepath="./debug/1.xml";
    m_settings.m_DefPath = QCoreApplication::applicationDirPath();//获取程序所在目录
    m_settings.m_SysSettingsFile = QString("%1/cal_ant.xml").arg(m_settings.m_DefPath);
    //m_settings.m_SysSettingsFile = QString("%1/cal_ant.xml").arg(m_DefPath);
      QFile file(m_settings.m_SysSettingsFile);//关联文件名字
  //       if(true==file.exists()){//如果存在不创建
  //           qDebug()<<QString::fromLocal8Bit( "文件已经存在");
  //           return;
  //       }
  //       else {
             bool isok = file.open(QIODevice::WriteOnly| QIODevice::Truncate);
             if(true == isok)
             {
                 QTextCodec::setCodecForLocale(QTextCodec::codecForLocale());
                 //创建xml文档对象
                 QDomDocument doc;

                 //创建xml头部格式
                 QDomProcessingInstruction ins;

                 ins = doc.createProcessingInstruction("xml","version=\"1.0\" encoding=\"UTF-8\"");
                 //创建xml文档根元素
                 QDomElement systemroot = doc.createElement("SystemSettings");
                 doc.appendChild(systemroot);
                 QDomElement root = doc.createElement("本地设置");
                 systemroot.appendChild(root);
                 QDomElement root2 = doc.createElement("FTP设置");
                 systemroot.appendChild(root2);

                 //创建root的子元素
                 QDomElement LoadPort = doc.createElement("本地端口");
                 root.appendChild(LoadPort);

                 QDomElement LocalConIp = doc.createElement("本地组播Ip");
                 root.appendChild(LocalConIp);

                 QDomElement LocalConPort = doc.createElement("本地组播端口");
                 root.appendChild(LocalConPort);

                 QDomElement ACUIp = doc.createElement("伺服IP");
                 root.appendChild(ACUIp);

                 QDomElement ACUPort = doc.createElement("伺服端口");
                 root.appendChild(ACUPort);

                 QDomElement RemoteConIP = doc.createElement("远控组播IP");
                 root.appendChild(RemoteConIP);

                 QDomElement RemoteConPort = doc.createElement("远控组播端口");
                 root.appendChild(RemoteConPort);

                 QDomElement CalIp = doc.createElement("标校IP");
                 root.appendChild(CalIp);

                 QDomElement CalPort = doc.createElement("标校端口");
                 root.appendChild(CalPort);
                 //设置root中元素的值和属性值
                 QDomText text;
                 text = doc.createTextNode(ui->LoadPort->text());
                 LoadPort.appendChild(text);
                 m_settings.m_LocalPort=ui->LoadPort->text().toInt();

                 text = doc.createTextNode(ui->LocalConIp->text());
                 LocalConIp.appendChild(text);
                 m_settings.m_localConIp=ui->LocalConIp->text();

                 text = doc.createTextNode(ui->LocalConPort->text());
                 LocalConPort.appendChild(text);
                 m_settings.m_LocalConPort=ui->LocalConPort->text().toInt();
                 text = doc.createTextNode(ui->ACUIp->text());
                 ACUIp.appendChild(text);
                 m_settings.m_AntennaIp.setAddress(ui->ACUIp->text());
                 text = doc.createTextNode(ui->ACUPort->text());
                 ACUPort.appendChild(text);
                 m_settings.m_AntennaPort=ui->ACUPort->text().toInt();
                 text = doc.createTextNode(ui->RemoteConIP->text());
                 RemoteConIP.appendChild(text);
                 m_settings.m_RemoteConIP=ui->RemoteConIP->text();
                 text = doc.createTextNode(ui->RemoteConPort->text());
                 RemoteConPort.appendChild(text);
                 m_settings.m_RemoteConPort=ui->RemoteConPort->text().toInt();
                 text = doc.createTextNode(ui->CalIp->text());
                 CalIp.appendChild(text);
                 m_settings.m_CalibrationIp=ui->CalIp->text();
                 text = doc.createTextNode(ui->CalPort->text());
                 CalPort.appendChild(text);
                 m_settings.m_CalibrationPort=ui->CalPort->text().toInt();

                 //创建root2的子元素
                 QDomElement FtpIp = doc.createElement("FTPIP");
                 root2.appendChild(FtpIp);
                 QDomElement FtpPort = doc.createElement("FTP端口");
                 root2.appendChild(FtpPort);
                 QDomElement UserName = doc.createElement("用户名");
                 root2.appendChild(UserName);
                 QDomElement Password = doc.createElement("密码");
                 root2.appendChild(Password);
                 QDomElement FTPPath = doc.createElement("FTP路径");
                 root2.appendChild(FTPPath);
                 QDomElement LocadPath = doc.createElement("本地路径");
                 root2.appendChild(LocadPath);


                 //设置root2中元素的值和属性值
                 text = doc.createTextNode(ui->FtpIp->text());
                 FtpIp.appendChild(text);
                 m_settings.m_FtpIp=ui->FtpIp->text();
                 text = doc.createTextNode(ui->FtpPort->text());
                 FtpPort.appendChild(text);
                 m_settings.m_FtpPort=ui->FtpPort->text().toInt();
                 text = doc.createTextNode(ui->UserName->text());
                 UserName.appendChild(text);
                 m_settings.UserName=ui->UserName->text();
                 text = doc.createTextNode(ui->Password->text());
                 Password.appendChild(text);
                 m_settings.Password=ui->Password->text();
                 text = doc.createTextNode(ui->FTPPath->text());
                 FTPPath.appendChild(text);
                 m_settings.m_FtpPath=ui->FTPPath->text();
                 text = doc.createTextNode(ui->LocadPath->text());
                 LocadPath.appendChild(text);
                 m_settings.m_DownPath=ui->LocadPath->text();

                 //保存
                 QTextStream stream(&file);//文本流关联文件
                 //QTextCodec *codec = QTextCodec::codecForName("GBK");
                // stream.setCodec(codec);
                 doc.save(stream,4,QDomNode::EncodingFromTextStream);
                 file.close();
                 this->close();
                 Message(2,1,"系统参数保存成功!");
             }

}
/*********************************************************************************************
 * initwidget() 初始化窗口控件
**********************************************************************************************/
void systemset::initwidget()
{
    ui->LoadPort->setText(QString::number(m_settings.m_LocalPort));
    ui->LocalConIp->setText(m_settings.m_localConIp.toString());
    ui->LocalConPort->setText(QString::number(m_settings.m_LocalConPort));
    ui->ACUIp->setText(m_settings.m_AntennaIp.toString());
    ui->ACUPort->setText(QString::number(m_settings.m_AntennaPort));
    ui->RemoteConIP->setText(m_settings.m_RemoteConIP.toString());
    ui->RemoteConPort->setText(QString::number(m_settings.m_RemoteConPort));
    ui->CalIp->setText(m_settings.m_CalibrationIp.toString());
    ui->CalPort->setText(QString::number(m_settings.m_CalibrationPort));
    ui->FtpIp->setText(m_settings.m_FtpIp.toString());
    ui->FtpPort->setText(QString::number(m_settings.m_FtpPort));
    ui->UserName->setText(m_settings.UserName);
    ui->Password->setText(m_settings.Password);
    ui->FTPPath->setText(m_settings.m_FtpPath);
    ui->LocadPath->setText(m_settings.m_DownPath);
}

//void systemset::readXML()
//{
//    m_settings.m_DefPath = QCoreApplication::applicationDirPath();//获取程序所在目录
//    m_settings.m_SysSettingsFile = QString("%1/cal_ant.xml").arg(m_settings.m_DefPath);
//    QFile file(m_settings.m_SysSettingsFile);
//   if(!file.open(QIODevice::ReadOnly))
//   {
//        file.close();
//        return;
//   }
//   //QTextStream floStream(&file);
//   //QString xmlDataStr = floStream.readAll();
//   //QString errorStr;
//  // int errorLine;
//  // int errorCol;
//     QDomDocument doc;
//       if(!doc.setContent(&file))
//       {
//           // qDebug() << errorStr << "line: " << errorLine << "col: " << errorCol;
//           qDebug()<<QString::fromLocal8Bit("打开失败");
//           file.close();
//           return;
//       }
//       file.close();
//       //系统设置界面初始化数据之电源通道设置
//       QString LoadPort = doc.elementsByTagName(QString::fromLocal8Bit("本地端口")).at(0).toElement().text();
//       ui->LoadPort->setText(LoadPort);
//       QString LocalConPort = doc.elementsByTagName(QString::fromLocal8Bit("本地组播端口")).at(0).toElement().text();
//       ui->LocalConPort->setText(LocalConPort);

//       QString ACUIp = doc.elementsByTagName(QString::fromLocal8Bit("伺服IP")).at(0).toElement().text();
//       ui->ACUIp->setText(ACUIp);
//       QString ACUPort = doc.elementsByTagName(QString::fromLocal8Bit("伺服端口")).at(0).toElement().text();
//       ui->ACUPort->setText(ACUPort);
//       QString RemoteConIP = doc.elementsByTagName(QString::fromLocal8Bit("远控组播IP")).at(0).toElement().text();
//       ui->RemoteConIP->setText(RemoteConIP);
//       QString RemoteConPort = doc.elementsByTagName(QString::fromLocal8Bit("远控组播端口")).at(0).toElement().text();
//       ui->RemoteConPort->setText(RemoteConPort);
//       QString CalIp = doc.elementsByTagName(QString::fromLocal8Bit("标校IP")).at(0).toElement().text();
//       ui->CalIp->setText(CalIp);
//       QString CalPort = doc.elementsByTagName(QString::fromLocal8Bit("标校端口")).at(0).toElement().text();
//       ui->CalPort->setText(CalPort);

//       //系统设置界面初始化数据之系统设置
//       QString FtpIp = doc.elementsByTagName(QString::fromLocal8Bit("FTPIP")).at(0).toElement().text();
//       ui->FtpIp->setText(FtpIp);
//       QString FtpPort = doc.elementsByTagName(QString::fromLocal8Bit("FTP端口")).at(0).toElement().text();
//       ui->FtpPort->setText(FtpPort);
//       QString UserName = doc.elementsByTagName(QString::fromLocal8Bit("用户名")).at(0).toElement().text();
//       ui->UserName->setText(UserName);
//       //qDebug()<<ui->lineEdit_soureIP->text();
//       QString Password = doc.elementsByTagName(QString::fromLocal8Bit("密码")).at(0).toElement().text();
//       ui->Password->setText(Password);
//       QString FTPPath = doc.elementsByTagName(QString::fromLocal8Bit("FTP路径")).at(0).toElement().text();
//       ui->FTPPath->setText(FTPPath);
//       QString LocadPath = doc.elementsByTagName(QString::fromLocal8Bit("本地路径")).at(0).toElement().text();
//       ui->LocadPath->setText(LocadPath);


//}
/****************************************************************
 *                                                              *
 * on_ButtonCancel_clicked() 保存设置
 *                                                              *
*****************************************************************/
void systemset::on_ButtonSave_clicked()
{
    updataXML();
}
/****************************************************************
 *                                                              *
 * on_ButtonCancel_clicked() 取消不保存设置并退出界面
 *                                                              *
*****************************************************************/
void systemset::on_ButtonCancel_clicked()
{
    this->close();
}
