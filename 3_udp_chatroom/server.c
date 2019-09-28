//udp服务器的实现
#include <stdio.h> //printf
#include <stdlib.h> //exit
#include <sys/types.h>
#include <sys/socket.h> //socket
#include <netinet/in.h> //sockaddr_in
#include <arpa/inet.h> //htons inet_addr
#include <unistd.h> //close
#include <string.h>

//信息结构体
typedef struct{
    char recv_ip[16];
    unsigned short recv_port;
    char text[128];
}MSG;

//链表结点结构体
typedef struct node{
    struct sockaddr_in addr;
    struct node *next;
}linklist;

int main(int argc, char const *argv[])
{
    if(argc < 3)
    {
        fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]);
        exit(1);
    }

    int sockfd; //文件描述符
    struct sockaddr_in serveraddr; //服务器网络信息结构体
    socklen_t addrlen = sizeof(serveraddr);

    //第一步：创建套接字
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("fail to socket");
        exit(1);
    }

    //第二步：填充服务器网络信息结构体
    //inet_addr：将点分十进制字符串ip地址转化为整形数据
    //htons：将主机字节序转化为网络字节序
    //atoi：将数字型字符串转化为整形数据
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(argv[1]);
    serveraddr.sin_port = htons(atoi(argv[2]));

    //第三步：将套接字与服务器网络信息结构体绑定
    if(bind(sockfd, (struct sockaddr *)&serveraddr, addrlen) < 0)
    {
        perror("fail to bind");
        exit(1);
    }

    //服务器接收数据并作出处理
    char buf[128] = "";
    struct sockaddr_in clientaddr;

    //定义一个链表的头指针
    linklist *head = NULL, *p, *temp;
    int flags = 0;
    MSG msg;
    while(1)
    {
        if(recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&clientaddr, &addrlen) < 0)
        {
            perror("fail to recvfrom");
            exit(1);
        }

        printf("[%s - %d]: %s\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), buf);

        //判断一下链表中是否已经存储了当前客户端的信息结构体
        p = head;
        while(p != NULL)
        {
            //memcmp：比较两个内存区域内容是否一样，返回值为0表示一样
            if(memcmp(&clientaddr, &p->addr, addrlen) == 0)
            {
                flags = 1;
                break;
            }

            p = p->next;
        }

        //根据flags的值决定是否将当前客户端的信息插入到链表中
        if(flags == 0)
        {
            //申请空间并赋值
            temp = (linklist *)malloc(sizeof(linklist));
            temp->addr = clientaddr;
            temp->next = NULL;

            //判断当前链表是否为空
            if(head == NULL)
            {
                head = temp;
                head->next = NULL;
            }
            else 
            {
                temp->next = head;
                head = temp;
            }
        }

        //处理数据并遍历链表、发送数据给所有在线的用户（自己不接收）
        strcpy(msg.recv_ip, inet_ntoa(clientaddr.sin_addr));
        msg.recv_port = ntohs(clientaddr.sin_port);
        strcpy(msg.text, buf);
        
        p = head;
        while(p != NULL)
        {
            if(memcmp(&clientaddr, &p->addr, addrlen) == 0)
            {
                p = p->next;
            }
            else 
            {
                if(sendto(sockfd, &msg, sizeof(MSG), 0, (struct sockaddr *)&p->addr, addrlen) < 0)
                {
                    perror("fail to sendto");
                    exit(1);
                }

                p = p->next;
            }
        }
    }

    return 0;
}
