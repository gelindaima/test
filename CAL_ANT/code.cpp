#include "code.h"
#include "ui_code.h"
extern std::list<QStringList>m_SourceCode;
#pragma execution_character_set("utf-8")
Code::Code(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Code)
{
    ui->setupUi(this);
    ui->textEdit->setReadOnly(true);
    upstate = new QTimer(this);
    upstate->setTimerType(Qt::PreciseTimer);
    upstate->start(1);
    connect(upstate,&QTimer::timeout,this,&Code::initwidget);
}

Code::~Code()
{
    delete ui;
}
/**************************************************************
 * initwidget() 初始化窗口控件
**************************************************************/
void Code::initwidget()
{
    std::list<QStringList>::iterator it = m_SourceCode.begin();
    if(it!=m_SourceCode.end())
    {
        for(int i =0;i<it->size();i++)
        {
           QStringList A = *it;
           if(A[i].contains(Title)==true)
           {
               ui->textEdit->append(A[i]);
           }
        }
        m_SourceCode.remove(it->at(0).split(""));
    }
    m_SourceCode.clear();
}

void Code::on_pushButton_ACU_clicked()
{
    Title = "本控单元发送";
}

void Code::on_pushButton_Remote_clicked()
{
    Title = "远控单元发送";
}

void Code::on_pushButton_Cal_clicked()
{
    Title = "标校单元发送";
}
