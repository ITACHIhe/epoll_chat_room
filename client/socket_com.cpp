#include "socket_com.h"
#include<QDebug>
#include"msgemiter.h"
#include<QMessageBox>
#include<QDir>
#include<QFile>

int socket_com::number=0;
socket_com *socket_com::com=socket_com::getInstance();
socket_com::socket_com()
{
    sockfd=0;
    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(8889);
    strptr="127.0.0.1";
    if(inet_pton(AF_INET, strptr, &servaddr.sin_addr) <= 0)
    {
        qDebug()<<"inet_pton()error";

    }
    iscon=false;
    QString dir_pic="picture";
    QDir dir1;
    if (!dir1.exists(dir_pic))
    {
        bool res = dir1.mkpath(dir_pic);
    }

}

int socket_com::socket_start()
{
    if(sockfd==0)
   {
        if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        qDebug()<<"create socket error";
        return 0;
    }
   }
    if(iscon==false)
  {
        if( connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
    {
         qDebug()<<"connect error()";
         return 0;
    }
  }
     iscon=true;
     const char buf[]="i need ID";
     qDebug()<<"socked1"<<sockfd;
     write(sockfd,buf,strlen(buf));
     return 0;
}

int socket_com::socket_login(const char*mix)
{
    if(sockfd==0)
   {
        if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
         qDebug()<<"create socket error";
        return 0;
    }
   }
    if(iscon==false)
  {
        if( connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
    {
         qDebug()<<"connect error()";
         return 0;
    }
  }
     iscon=true;
    //
    //发送数据到服务器
     qDebug()<<"socked"<<sockfd;
     write(sockfd,mix,strlen(mix));
     return 0;
}

int socket_com::socket_enter(const char *mix)
{
     write(this->sockfd,mix,strlen(mix));
     qDebug()<<"success to enter chat room";

     return 0;
}
void socket_com::socket_send_file()
{
    FILE *xml_fq;
    if((xml_fq=fopen("data.xml","rb"))==NULL)
    {
        qDebug()<<"文件打开失败";
        exit(0);
    }
    qDebug()<<"open file success";
    memset(buffer,0,sizeof(buffer));
    while (!feof(xml_fq))
    {
       len = fread(buffer, 1, sizeof(buffer), xml_fq);
       if(len != write(sockfd, buffer, len))
       {
           qDebug()<<"写完了";
           break;
       }
    }
    fclose(xml_fq);
}

void socket_com::socket_recv()//这是受到xml回包的做法
{
    char tmpBuffer[20480];
    r1=MsgEmiter::getInstance();
    while(1)
 {

    int recv_num=recv(sockfd,tmpBuffer,20480,0);
    if(tmpBuffer[0]=='P')//这里补充一个收到文件的场景
    {
        QString tmp_picture=QString(QLatin1String(tmpBuffer));
        tmp_picture=tmp_picture.mid(1);
        QFile *F_picture=new QFile;
        F_picture->setFileName("./picture/"+tmp_picture);
        F_picture->open(QFile::Text|QFile::ReadWrite);
        if(F_picture->exists())
        {
            qDebug()<<"create file successs in  picture";
        }
        else
        {
            qDebug()<<"can't create file in  picture";
        }
        char tmp_stream_buffer[20480];
        memset(tmp_stream_buffer,0,sizeof(tmp_stream_buffer));
       int tmp_re=recv(sockfd,tmp_stream_buffer,20480,0);
       tmp_stream_buffer[tmp_re]='\0';

       QString tmp_pic=QString(QLatin1String(tmp_stream_buffer));
       tmp_pic=tmp_pic.mid(3);
       qDebug()<<"tmp_pic:"<<tmp_pic;
       int t;
       for(int i=0;i<tmp_pic.size();i++)
       {
           if(tmp_pic[i]==" ")
           {
               t=i;
               break;
           }
       }
       QString  tmp_pic_1=tmp_pic.mid(0,t);
       qDebug()<<"tmp_name:"<<tmp_pic_1;

       QString tmp_stream=tmp_pic.mid(t+1);
       QFile *stream_file=new QFile;
       stream_file->setFileName("./picture/"+tmp_pic_1);
       stream_file->open(QFile::Text|QFile::ReadWrite);
       if(stream_file->exists())
       {
           qDebug()<<"load in stream_file";
       }
       else
       {
           qDebug()<<"load in stream_file is NOT";
       }
       QByteArray tmp_stream1=tmp_stream.toLatin1();
       const char * tmp_stream2=tmp_stream1.data();
        QTextStream in(stream_file);
        in<<tmp_stream2;
         stream_file->close();


    }
    if(recv_num==0)
    {
        qDebug()<<"recv is duankai";
       // r1->sendmsg_close();  it will dead all_process
        QMessageBox::warning(0,"警告","服务器已经断开连接");
        return;
    }  
    tmp_xml = QString(QLatin1String(tmpBuffer));

    r1->send_msg();
    }
}


