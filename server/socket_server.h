#ifndef SOCKET_SERVER_H
#define SOCKET_SERVER_H
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <error.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <limits.h>
#include <sys/stropts.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <time.h>
#include <list>
#include <arpa/inet.h>
#include<thread>
#include<QFile>
#include"chioce.h"
#include<set>
#include<map>
#define BUF_SIZE 1024//默认缓冲区
#define SERVER_PORT 8889//监听端口
#define SERVER_HOST "127.0.0.1"   //服务器IP地址
#define MAXEPOLLSIZE 10000//事件最多容纳数

struct epoll_conncetion_pool
{
  int   fd;//保存对方套接字
  struct sockaddr_in  s_sockaddr;//保存对方地址信息用的
  QString ID;//保存对方登录账号
  bool islive;//是否断开
  QString room;
  QString name;
  QString mima;
};
class socket_server
{
private:
    socket_server();//单例模式的写法

public:
    static socket_server* getInstance()//单例模式的写法
    {
        if(server==nullptr)
        {
            server=new socket_server();//单例模式的写法
        }
        return server;
    }
    static socket_server* freeInstance()//单例模式的写法
    {
        return server;
    }
public:
    static socket_server* server;//单例模式的写法


public://关于函数的定义
    void socket_start();//socket初始化函数
    void  epoll_init();//epoll初始化函数
    void socket_rec();//socket接收数据函数
    QString create_ID();//创建ID函数
    bool check_ID(char * ID);//检查ID函数
    void socket_log(QString op);//写入日志函数
    void socker_close();//socket关闭函数
    int setnonblocking(int sockfd);//设置非阻塞的函数
    int handle(int cfd);//交流的函数


public://关于其他交互变量的定义
    QFile *file;//记录生成ID的文件
    QString new_password;//初始化生成的ID默认密码都是123
    QString log_name;//生成ID文件的名字
    QFile *file_operate;//记录生成日志的文件
    QString log_operate;//生成日志文件的名字
    int sum_con;//记录当前连接的人数
    chioce *e1;// 这个是发送chioce里面的信号必须要生成对象的指针




public://关于epoll,socket的定义
    int sockfd,len,ref;//创建自己的socket描述符以及相关参数
    struct sockaddr_in  servaddr,servaddr_c;//创建socket相关的结构体
    char ReadBuffer[2048];//接收数据的缓冲区
    char SendBuffer[2048];//发送数据的缓冲区
    int nfds;//获得醒来的事件数量
    int cfd;//socket接受套接字
    struct sockaddr_in cliaddr;
    socklen_t socklen;//结构体长度
    std::set<int>fdset;// 保存所有的文件描述符
    struct epoll_event ev;//定义单个epoll事件
    struct epoll_event events[MAXEPOLLSIZE];//定义epoll事件的数组
    int  m_worker_connections;//epoll连接的最大项数
    int  m_ListenPortCount;//所监听的端口数量
    int  epfd;//epoll_create返回的句柄
    std::set<int>fdset_to_close;
    std::set<epoll_conncetion_pool>con_pool;//连接池的集合
    std::set<int>fdset_re;
    std::set<QString>only_ID;

    epoll_conncetion_pool my_ol[10000];
    std::map<QString,QString>room_password;

public:
    QString to_qstring(char *a);


};

#endif // SOCKET_SERVER_H
