#include "socket_server.h"
#include<QDebug>
#include<QFile>
#include<QTextStream>
#include<set>
#include<QDateTime>
#include"tinyxml.h"
#include<QDir>
#include<iostream>
socket_server *socket_server::server=NULL;//单例模式的写法

socket_server::socket_server()
{
    m_worker_connections=1024;//初始化epoll连接的最大项数
    sockfd=0;//初始化socket套接字的句柄
    memset(&servaddr,0,sizeof(servaddr));//结构体清零
    memset(&servaddr_c,0,sizeof(servaddr_c));//结构体清零

    servaddr.sin_family = PF_INET;//设置套接字的信息
    servaddr.sin_port = htons(SERVER_PORT);//设置套接字的信息
    servaddr.sin_addr.s_addr = inet_addr(SERVER_HOST);//设置套接字的信息

     log_name="./ID.txt";//初始化ID文件的名字
     file=new QFile(log_name);//为日志的文件分配空间
     new_password="123";//初始化新ID的通用密码

     QString dir_str = "log";
     QDir dir;
         if (!dir.exists(dir_str))
         {
             bool res = dir.mkpath(dir_str);            
         }

         QString dir_pic="picture";
         QDir dir1;
         if (!dir1.exists(dir_pic))
         {
             bool res = dir1.mkpath(dir_pic);
         }


     QDateTime current_date_time=QDateTime::currentDateTime();//将系统时间转换成Qstring的函数
     QString current_date=current_date_time.toString("yyyy.MM.dd");//将系统时间转换成Qstring的函数

     log_operate="./"+current_date+"oper.log";//生成名字是当天的日志名
     file_operate=new QFile("./log/"+log_operate);//为日志文件分配空间

     socklen = sizeof(struct sockaddr_in);//获得结构体长度

     for(int i=0;i<10000;i++)
     {
         my_ol[i].fd=0;
         my_ol[i].ID="";
         my_ol[i].islive=false;
         my_ol[i].room="";
         my_ol[i].name="";
         //my_ol[i].s_sockaddr=0;
     }
}
void socket_server::socket_start()
{
     if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)//生成socket套接字并判断是否成功
     {
         qDebug()<<"生成socket失败";
     }
     qDebug()<<"生成socket成功";

     int flag = 1; //防止因为timewait情况而使得某个端口无法绑定的情况
     setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)); //防止因为timewait情况而使得某个端口无法绑定的情况

     ref=bind(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr));//端口绑定
     if(ref<0)//端口绑定是否成功的判断
     {
       qDebug()<<"绑定失败";
     }
       qDebug()<<"绑定成功";

     int tmp=listen(sockfd,511);//套接字监听;nginx官方写的是这个511
     if(tmp<0)
     {
       qDebug()<<"监听失败";
     }
       qDebug()<<"监听成功";
}

void socket_server::epoll_init()
{
      epfd = epoll_create(m_worker_connections);//初始化epoll
      if(epfd==-1)//epoll是否成功的判断
      {
          qDebug()<<"epoll初始化失败";
          exit(2);
      }
}

void socket_server::socket_rec()
{
    epoll_init();
    ev.events = EPOLLIN | EPOLLET;//设置为可读和边缘触发
    ev.data.fd = sockfd;//监听套接字赋给事件
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev) < 0) //监听套接字事件加入epoll并判断是否成功
       {           \
           qDebug()<<"epoll set insertion error: fd="<<sockfd;
           exit(1);
       }
    int curfds=1;
    while (1)
    {
         nfds = epoll_wait(epfd, events, curfds, -1);//这个第三个-1不明白，可能是无限个吧，第四个-1是阻塞
         if (nfds == -1)//判断是否成功的标志
         {
           qDebug()<<"epoll_wait error";
           continue;
         }
         for(int i=0;i<nfds;i++)
         {

            if(events[i].data.fd==sockfd)//如果这个事件是监听套接字事件
            {
                cfd=accept(sockfd, (struct sockaddr *)&cliaddr,&socklen);
                if (cfd < 0) //成功的判断
                {
                 qDebug()<<"accept error";
                 continue;
                }
                qDebug()<<"接收地址为:"<<inet_ntoa(cliaddr.sin_addr)<<",端口为:"<<cliaddr.sin_port;
                if (curfds >= MAXEPOLLSIZE) //事件过多，停止接收
                   {
                    qDebug()<<"太多连接了超过了这个数："<<MAXEPOLLSIZE;
                    close(cfd);
                    continue;
                   }
                if (setnonblocking(cfd) < 0) //将这个非阻塞
                 {
                    qDebug()<<"设置非阻塞失败";
                 }

                ev.events = EPOLLIN | EPOLLET;//事件设置为可以和边缘
                ev.data.fd = cfd;//接收的套接字传入

                epoll_conncetion_pool l1;//生成一个对象池的对象
                l1.fd=cfd;//连接池元素的填写
                l1.s_sockaddr=cliaddr;//连接池元素的填写
                l1.islive=true;//连接池元素的填写

                if (epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev) < 0)//将该事件传入事件数组
                    {
                    qDebug()<<"add socket"<<cfd<<"to epoll failed:"<<strerror(errno);
                    exit(1);
                    }
                curfds++;

                continue;
            }

            // 处理客户端请求
          if (handle(events[i].data.fd) < 0)//处理请求
          {
           epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd,&ev);
           curfds--;
          }
         }
    }
}

/*这个是之前使用select的写法
void socket_server::socket_rec()
{




     fd_set rfds;
     std::set<int>fdset;// 保存所有的文件描述符
     fdset.insert(sockfd);
     while(1)
     {

         FD_ZERO(&rfds);
         for(int fd:fdset)
         {
            FD_SET(fd,&rfds);

         }
         int ret = select(*fdset.rbegin()+1, &rfds, NULL, NULL, NULL);
         if(ret>0)
         {
             for(int fd:fdset)
             {
                 if(fd==sockfd&&FD_ISSET(fd, &rfds))
                 {
                     struct sockaddr_in client;
                     socklen_t addrlen = sizeof(client);
                     int cfd = -1;
                     if (-1 == (cfd = accept(sockfd, (struct sockaddr*)&client, &addrlen)))
                       {
                         qDebug()<<"cfd accept error";
                         exit(1);
                       }

                     FD_SET(cfd, &rfds);
                     fdset.insert(cfd);
                     qDebug()<<"new connection"<<cfd;
                 }
                 else if(FD_ISSET(fd, &rfds))
                 {
                     memset(ReadBuffer,0,sizeof(ReadBuffer));
                     int lenrecv=-1;
                     lenrecv=recv(fd,ReadBuffer,2048,0);
                     if(lenrecv>0)
                     {
                         qDebug()<<ReadBuffer;
                         char tmp[]="i need ID";
                         int tmp1=strcmp(ReadBuffer,tmp);
                          if(tmp1==0)//收到客户端申请账号的请求
                          {
                              qDebug()<<"he need ID";
                              QString tmp_ID=create_ID();
                              QByteArray tmp_ID1=tmp_ID.toLatin1();
                              const char * tmp_ID2=tmp_ID1.data();
                              write(fd,tmp_ID2,sizeof(tmp_ID2));
                          }
                          if(ReadBuffer[0]=='I')//收到对方账号登录
                          {
                              if(file->isOpen())//debug出文件会在这里打开，又找不到错误，就在这里弄个判断
                              {
                                  file->close();
                              }
                              qDebug()<<"ReadBuffer"<<ReadBuffer;
                              bool tmp_panduan=check_ID(ReadBuffer);
                              if(tmp_panduan)
                              {
                                  emit e1->senddata(fdset.size()-1);
                                  qDebug()<<"tmp_panduan is ture";
                                  char panduan_password[]="yesyes";
                                  write(fd,panduan_password,sizeof(panduan_password));
                              }
                              else
                              {
                                    qDebug()<<"tmp_panduan is false";
                                  char panduan_password[]="NONO";
                                  write(fd,panduan_password,sizeof(panduan_password));
                              }
                          }
                          if(ReadBuffer[0]=='<')
                          {
                              std::set<int>fdset_re=fdset;//这个相当于TCP广播
                              fdset_to_close=fdset_re;
                              fdset_re.erase(sockfd);

                              fdset_re.erase(fd);//这个使发送者不必收到自己发送的那部分

                              for(int xml_fd:fdset_re)
                              {
                              send(xml_fd,ReadBuffer,strlen(ReadBuffer),0);
                              qDebug()<<tmp;
                              }
                          }
                          if(ReadBuffer[0]=='0'&&ReadBuffer[1]=='0'&&ReadBuffer[2]=='0'&&ReadBuffer[3]=='0'&&ReadBuffer[4]=='0')
                          {
                              qDebug()<<"this fd:"<<fd<<"want close";
                          }
                     }
                     else if(0 == lenrecv)//recv为0代表连接关闭
                     {
                         fdset.erase(fd);
                         emit e1->senddata(fdset.size()-1);
                         FD_CLR(fd, &rfds);
                         close(fd);
                     }
                     else
                     {
                        qDebug()<<"error";
                     }
                 }
             }
         }
         else
         {
            qDebug()<<"select error";
         }

     }




}*/


QString socket_server::create_ID()
{
    qDebug()<<"start create_ID";
    int ID_num=0;
    file->open(QFile::Text|QFile::ReadWrite);
    if(file->exists())
    {
        qDebug()<<"create file successs";
    }
    else
    {
        qDebug()<<"can't create file";
    }

    QTextStream in(file);
    while (!in.atEnd())
    {

        QString line = in.readLine();
        ID_num++;
        qDebug()<<line;
    }
    QString new_ID="000"+QString::number(ID_num);
    qDebug()<<"new ID and password:"<<new_ID<<" "<<new_password;
    file->close();
    file->open(QFile::Text|QFile::ReadWrite|QIODevice::Append);
    if(file->exists())
    {
        qDebug()<<"create file successs again";
    }
    else
    {
        qDebug()<<"can't create file again ";
    }
     QTextStream out(file);
     out<<new_ID<<" "<<new_password<<endl;
     file->close();
     return new_ID;

}

bool socket_server::check_ID(char * ID)
{

    char *tmp_check_ID=ID;
    QString str = QString(QLatin1String(tmp_check_ID));

     file->open(QFile::Text|QFile::ReadOnly);
     if(file->exists())
     {
         qDebug()<<"create file successs in  Check_ID";
     }
     else
     {
         qDebug()<<"can't create file in  Check_ID";
     }
     QTextStream in(file);
     bool is_right=false;
     while (!in.atEnd())
     {
        int a=0;
        qDebug()<<"in.atend:"<<++a;
        QString line = in.readLine();
        QString line2="ID:"+line;
         if(line2==str)
         {
             is_right=true;
             qDebug()<<"password is right";
             return is_right;
         }
         qDebug()<<line;
     }
     file->close();
     return is_right;

}

void socket_server::socket_log(QString op)
{
    file_operate->open(QFile::Text|QFile::ReadWrite|QIODevice::Append);

    if(file_operate->exists())
    {
        qDebug()<<"create log_file successs again";
    }
    else
    {
        qDebug()<<"can't log_create file again ";
    }
      QTextStream in(file_operate);
      QDateTime current_date_time=QDateTime::currentDateTime();
      QString current_date=current_date_time.toString("yyyy.MM.dd hh:mm:ss");
      in<<current_date<<":"<<endl;
      in<<op<<endl;
      file_operate->close();

}

void socket_server::socker_close()
{
    for(int tmp_fd: fdset_to_close)
    {
        char tmp_send[]="server ready to close";
        send(tmp_fd,tmp_send,strlen(tmp_send),0);
        ::shutdown(tmp_fd,SHUT_RDWR);
    }
}

int socket_server::setnonblocking(int sockfd)
{
    if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0)|O_NONBLOCK) == -1)
    {
           return -1;
    }
    return 0;
}

int socket_server::handle(int cfd)
{
    memset(ReadBuffer,0,sizeof(ReadBuffer));
    int lenrecv=recv(cfd,ReadBuffer,2048,0);
    if(lenrecv>0)
    {
        qDebug()<<ReadBuffer;
        QString tmp="i need ID";
        QString aa=to_qstring(ReadBuffer);


         if(tmp==aa)//收到客户端申请账号的请求
         {

             qDebug()<<"he need ID";
             QString tmp_ID=create_ID();
             QByteArray tmp_ID1=tmp_ID.toLatin1();
             const char * tmp_ID2=tmp_ID1.data();
             write(cfd,tmp_ID2,sizeof(tmp_ID2));
         }
         if(ReadBuffer[0]=='I')//收到对方账号登录
         {
             qDebug()<<"收到对方账号登录";
            QString tmp_str(ReadBuffer);
            tmp_str=tmp_str.mid(3);
            int tmp_str_num;
            for(int i=0;i<tmp_str.size();i++)
            {
                if(tmp_str[i]==" ")
                {
                    tmp_str_num=i;
                    break;
                }
            }
            tmp_str=tmp_str.mid(0,tmp_str_num);
            for(auto i:only_ID)
            {
                if(i==tmp_str)
                {
                    //发送该ID已经登陆的信息
                    char t[]="already have";
                    send(cfd,t,sizeof(t),0);
                    return 0;
                }
            }
             if(file->isOpen())//debug出文件会在这里打开，又找不到错误，就在这里弄个判断
             {
                 file->close();
             }            
             bool tmp_panduan=check_ID(ReadBuffer);
             if(tmp_panduan)//验证账号成功
             {

                 only_ID.insert(tmp_str);


                 my_ol[cfd].fd=cfd;
                 my_ol[cfd].ID=tmp_str;
                 my_ol[cfd].islive=true;
                 fdset.insert(cfd);
                 emit e1->senddata(fdset.size());//这里改成通过连接池的数量来判断
                 qDebug()<<"验证账号成功";
                 char panduan_password[]="yesyes";
                 write(cfd,panduan_password,sizeof(panduan_password));//发送验证成功的信息给客户端
                //  QString  str1 = QString(QLatin1String(ReadBuffer));

             }
             else//验证账号失败
             {

                 qDebug()<<"验证账号失败";
                 char panduan_password[]="NONO";
                 write(cfd,panduan_password,sizeof(panduan_password));//发送验证失败的信息给客户端
             }
         }
         if(ReadBuffer[0]=='R')
         {
             qDebug()<<"收到对方房间号的填写";
             QString tmp_str(ReadBuffer);
             tmp_str=tmp_str.mid(1);         
             int q[1];
             q[0]=q[1]=0;
             int u=0;
                for(int i=0;i<tmp_str.size();i++)
                {
                    if(tmp_str[i]=="!")
                    {
                        q[u]=i;
                        u++;
                    }
                }
                QString that_room=tmp_str.mid(0,q[0]);
                QString that_name;
                q[0]=q[0]+1;
                for(int i=0;i<q[1]-q[0];i++)
                {
                    that_name[i]=tmp_str[q[0]+i];
                }
                QString that_mima=tmp_str.mid(q[1]+1,tmp_str.size());
                my_ol[cfd].name=that_name;
                my_ol[cfd].room=that_room;
                my_ol[cfd].mima=that_mima;

                std::map<QString,QString>::iterator lit;
                lit=room_password.find(that_room);
                  if(lit==room_password.end())
                  {
                    room_password[that_room]=that_mima;
                    char tt[]="room password is once";
                    write(cfd,tt,sizeof(tt));

                  }
                  else
                  {
                      if(room_password[that_room]==that_mima)
                      {
                          char tt[]="room password is right";
                            write(cfd,tt,sizeof(tt));

                      }
                      else
                      {

                          char tt[]="room password is error";
                           write(cfd,tt,sizeof(tt));

                      }


                  }

        }
         if(ReadBuffer[0]=='P')
         {
             QString tmp_picture=to_qstring(ReadBuffer);
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
             //int filehandle=F_picture->handle();
             //FILE *pict=fdopen(filehandle,"r+");

             fdset_re=fdset;//这个相当于TCP广播
             fdset_to_close=fdset_re;
             fdset_re.erase(sockfd);

             fdset_re.erase(cfd);//这个使发送者不必收到自己发送的那部分
             for(int xml_fd:fdset_re)
             {
                 if(my_ol[xml_fd].room==my_ol[cfd].room)
                 {
                     send(xml_fd,ReadBuffer,strlen(ReadBuffer),0);

                 }
             qDebug()<<tmp;
             }


         }
         if(ReadBuffer[0]=='>'&&ReadBuffer[1]=='?'&&ReadBuffer[2]=='<')
         {

             QString tmp_pic=to_qstring(ReadBuffer);
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



               fdset_re=fdset;//这个相当于TCP广播
               fdset_to_close=fdset_re;
               fdset_re.erase(sockfd);

               fdset_re.erase(cfd);//这个使发送者不必收到自己发送的那部分
               for(int xml_fd:fdset_re)
               {
                   if(my_ol[xml_fd].room==my_ol[cfd].room)
                   {
                       send(xml_fd,ReadBuffer,strlen(ReadBuffer),0);

                   }
               qDebug()<<tmp;
               }

         }
         if(ReadBuffer[0]=='<')//这个是聊天信息的第一个字符，因为是xml格式，所以这里简写判断
         {
             qDebug()<<"my_ol"<<my_ol[1].fd<<" "<<my_ol[1].ID<<" "<<my_ol[1].islive;
              QFile chat_file("./data1.xml");
              chat_file.open(QFile::Text|QFile::ReadWrite);
              if(chat_file.exists())
              {
                  qDebug()<<"create file successs in  Check_ID";
              }
              else
              {
                  qDebug()<<"can't create file in  Check_ID";
              }
               QTextStream in(&chat_file);
              in<<ReadBuffer;
              chat_file.close();
              TiXmlDocument doc;
              if(!doc.LoadFile("data1.xml"))
              {
                  qDebug()<<"加载XML文件失败";
                  const char *errorStr = doc.ErrorDesc();
                  qDebug()<<errorStr; //打印失败原因；
              }
              else
              {
                  qDebug()<<"加载XML文件success";
                  TiXmlElement *root = doc.FirstChildElement();
                  TiXmlElement *child_str = root->FirstChildElement("STRING");
                  TiXmlElement *child_name = root->FirstChildElement("NAME");
                  TiXmlElement *child_time = root->FirstChildElement("TIME");
                  TiXmlElement *child_address = root->FirstChildElement("ADDRESS");
                  TiXmlElement *child_HOST = root->FirstChildElement("HOST");
                  TiXmlElement *child_room = root->FirstChildElement("ROOMNUMBER");
                  QString str=child_str->GetText();
                  QString name=child_name->GetText();
                  QString time=child_time->GetText();
                  QString address=child_address->GetText();
                  QString host=child_HOST->GetText();
                  QString room=child_room->GetText();
                  qDebug()<<"str:"<<str;
              }
                //...........................................




             fdset_re=fdset;//这个相当于TCP广播
             fdset_to_close=fdset_re;
             fdset_re.erase(sockfd);

             fdset_re.erase(cfd);//这个使发送者不必收到自己发送的那部分
             for(int xml_fd:fdset_re)
             {
                 if(my_ol[xml_fd].room==my_ol[cfd].room)
                 {
                     send(xml_fd,ReadBuffer,strlen(ReadBuffer),0);
                 }

             qDebug()<<tmp;
             }
         }
         //这是客户端关闭发送的信息，提前告诉我要关闭
         if(ReadBuffer[0]=='0'&&ReadBuffer[1]=='0'&&ReadBuffer[2]=='0'&&ReadBuffer[3]=='0'&&ReadBuffer[4]=='0')
         {
             qDebug()<<"this fd:"<<cfd<<"want close";
             fdset.erase(cfd);
             fdset_to_close.erase(cfd);
             emit e1->senddata(fdset.size());

             only_ID.erase(my_ol[cfd].ID);
             my_ol[cfd].fd=0;
             my_ol[cfd].ID="";
             my_ol[cfd].islive=false;
             my_ol[cfd].mima="";
             my_ol[cfd].name="";
             my_ol[cfd].room="";
         }
    }
    return 0;
}

QString socket_server::to_qstring(char *a)
{
      QString  str2 = QString(QLatin1String(a));
      return str2;
}
