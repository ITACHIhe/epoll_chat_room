#include "choice.h"
#include "ui_choice.h"
#include<QDebug>
#include"socket_com.h"
#include<QMessageBox>
#include <unistd.h>
#include<string>
choice::choice(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::choice)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::Dialog| Qt::FramelessWindowHint);

    ui->password_line->setEchoMode(QLineEdit::Password);
    ui->room_line->setClearButtonEnabled(true);
    ui->name_line->setClearButtonEnabled(true);
    ui->password_line->setClearButtonEnabled(true);
    ui->password_line->setEchoMode(QLineEdit::Password);

    ui->room_line->setMaxLength(8);
    ui->name_line->setMaxLength(13);
    ui->password_line->setMaxLength(13);

    QIntValidator *validator = new QIntValidator(0, 1000000, this);
    ui->room_line->setValidator(validator);
}

choice::~choice()
{
    delete ui;
}

void choice::on_quit_Button_clicked()
{
    this->close();
}

void choice::mouseMoveEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);
    QPoint y=event->globalPos();
    QPoint x=y-this->z;
    this->move(x);
}

void choice::mousePressEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);
    QPoint y =event->globalPos();
    QPoint x=this->geometry().topLeft();
    this->z=y-x;
}

void choice::mouseReleaseEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);
    this->z=QPoint();
}

void choice::closeEvent(QCloseEvent *)
{
    socket_com *s1=socket_com::getInstance();
    char closebuff[]="00000";
    send(s1->sockfd,closebuff,strlen(closebuff),0);
    ::close(s1->sockfd);
}

void choice::on_enter_Button_clicked()
{
    QString name=ui->name_line->text();
    QString room=ui->room_line->text();
    QString mima=ui->password_line->text();
    if(name.isEmpty()||room.isEmpty())
    {
        QMessageBox::warning(0,"警告","密码或者账号为空");
    }
    else
    {

        QString message="R"+room+"!"+name+"!"+mima;
        QByteArray message1=message.toLatin1();
        const char * message2=message1.data();

        socket_com *s3=socket_com::getInstance();       
        s3->socket_enter(message2);

        char tmp_readbuffer1[2000];
        memset(tmp_readbuffer1,0,sizeof(tmp_readbuffer1));
        int lenrecv=-1;

            lenrecv=recv(s3->sockfd,tmp_readbuffer1,2000,0);
            while (1)
            {
                if(lenrecv>1)
                {
                tmp_readbuffer1[lenrecv]='\0';
                qDebug()<<"123"<<tmp_readbuffer1;
                QString str = QString(QLatin1String(tmp_readbuffer1));
                qDebug()<<"get str is"<<str;
                if(str=="room password is once")
                {
                    QMessageBox::information(0,"提示","房间被创建，你是房主");
                    m1=new chat_room(name,room);
                    m1->my_room_number=room;
                    m1->show();
                    this->hide();
                    return;
                }
                if(str=="room password is right")
                {
                    m1=new chat_room(name,room);
                    m1->my_room_number=room;
                    m1->show();
                    this->hide();
                    return;
                }
                if(str=="room password is error")
                {
                    QMessageBox::warning(0,"警告","房间密码已经已经被创建，或者密码错误");
                    return;
                }
            }
            }
    }
}

