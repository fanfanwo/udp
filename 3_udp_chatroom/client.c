//udp聊天室客户端的实现
#include <stdio.h> //printf
#include <stdlib.h> //exit
#include <sys/types.h>
#include <sys/socket.h> //socket
#include <netinet/in.h> //sockaddr_in
#include <arpa/inet.h> //htons inet_addr
#include <unistd.h> //close
#include <string.h>
#include <pthread.h>

typedef struct{
    char recv_ip[16];
    unsigned short recv_port;
    char text[128];
}MSG;

int sockfd; //文件描述符
struct sockaddr_in serveraddr; //服务器网络信息结构体
socklen_t addrlen = sizeof(serveraddr);

void *recv_fun(void *arg)
{
    MSG msg;

    while(1)
    {
        if(recvfrom(sockfd, &msg, sizeof(MSG), 0, (struct sockaddr *)&serveraddr, &addrlen) < 0)
        {
            perror("fail to recvfrom");
            exit(1);
        }

        printf("[%s - %u]: %s\n", msg.recv_ip, msg.recv_port, msg.text);
    }
}

int main(int argc, char const *argv[])
{
    if(argc < 3)
    {
        fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]);
        exit(1);
    }

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

    //创建一个子线程接收服务器发送的数据
    pthread_t thread;
    if(pthread_create(&thread, NULL, recv_fun, NULL) != 0)
    {
        perror("fail to pthread_create");
        exit(1);
    }
    pthread_detach(thread);

    //主控线程负责发送数据
    char buf[128] = "";
    while(1)
    {
        fgets(buf, sizeof(buf), stdin);
        buf[strlen(buf) - 1] = '\0';

        if(sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&serveraddr, addrlen) < 0)
        {
            perror("fail to sendto");
            exit(1);
        }
    }

    return 0;
}