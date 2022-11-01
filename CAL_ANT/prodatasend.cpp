#include "prodatasend.h"
#include "pvariable.h"
#include <QTimer>
extern Prodata m_Prodata;
extern QList<Prodata>m_Prodatanode;
extern systemsetdata m_settings;
extern bool ProsendState;
extern Win_QextSerialPort* myCom;
#pragma execution_character_set("utf-8")

ProdataSend::ProdataSend()
{

}

void ProdataSend::run()
{

}

void ProdataSend::Startrun()
{
    int i=0;
    while(ProsendState==true)
    {

        QList<Prodata>::iterator it = m_Prodatanode.begin();
        if(it!=m_Prodatanode.end())
        {
            QDateTime Dtime = QDateTime::currentDateTime();//获取当前时间
//            qDebug()<<it->Time;
//            qDebug()<<Dtime;
//            if(Dtime.msecsTo(it->Time)>20)
//            {
//                m_Prodatanode.erase(it++);//删除头节点
//            }
           // if(Dtime.msecsTo(it->Time)<=1&&Dtime.msecsTo(it->Time)>=-1)//时间间隔
     //       qDebug()<<"time is   " <<it->Time;
           if(Dtime.msecsTo(it->Time)<=1&&Dtime.msecsTo(it->Time)>=-1)//时间间隔

            {
                qDebug()<<"----------------";
                QString num= QString("%1").arg(it->FirstAZ+it->AZAngle,7,'f',3,QChar('0'));
                QString date =QString("<000/04_0,+%1\r").arg(num);
                myCom->write(date.toUtf8(),date.size());
                qDebug()<<"AZ:"<<date;
                num= QString("%1").arg(it->FirstEL+it->ELAngle,7,'f',3,QChar('0'));
               // date =QString("<000/04_1,+%1\r").arg(QString::number(it->FirstEL+it->ELAngle));
                date =QString("<000/04_1,+%1\r").arg(num);
                myCom->write(date.toUtf8(),date.size());
                qDebug()<<"el:"<<date;
                m_Prodatanode.erase(it);//删除头节点

            }
            i++;
        }
        else
        {

          ProsendState=false;
        }
    }
    return;
}
