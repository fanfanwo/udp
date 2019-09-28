#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define ERRLOG(errmsg) do{\
                            perror(errmsg);\
                            printf("%s - %s - %d\n", __FILE__, __func__, __LINE__);\
                            exit(1);\
                        }while(0)

#define N 128

void do_help()
{
    system("clear");
    printf("---------------------\n");
    printf("------ 1. 下载 ------\n");
    printf("------ 2. 上传 ------\n");
    printf("------ 3. 退出 ------\n");
    printf("---------------------\n");
}

void do_download(int sockfd, struct sockaddr_in serveraddr)
{
    char filename[N] = {};
    printf("请输入要下载的文件名：");
    scanf("%s", filename);

    char data[1024] = "";
    int data_len;
    int fd;
    int flags = 0;
    int num = 0;
    int recv_len;

    //组数据并发送
    data_len = sprintf(data, "%c%c%s%c%s%c", 0, 1, filename, 0, "octet", 0);
    if(sendto(sockfd, data, data_len, 0, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
    {
        ERRLOG("fail to sendto");
    }

    //接收数据并分析处理
    socklen_t addrlen = sizeof(serveraddr);
    while(1)
    {
        if((recv_len = recvfrom(sockfd, data, sizeof(data), 0, (struct sockaddr *)&serveraddr, &addrlen)) < 0)
        {
            ERRLOG("fail to recvfrom");
        }

        //printf("%d - %u\n", data[1], ntohs(*(unsigned short *)(data + 2)));
        //printf("%s\n", data + 4);

        if(data[1] == 5)
        {
            printf("error: %s\n", data + 4);
            return ;
        }
        else if(data[1] == 3)
        {
            //防止文件内容清空
            if(flags == 0)
            {
                if((fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0664)) < 0)
                {
                    ERRLOG("fail to open");
                }
                flags = 1;
            }

            //判断数据包的编号是否是上一次的编号加1
            if(num + 1 == ntohs(*(unsigned short *)(data + 2)) && recv_len == 516)
            {
                //向文件写入数据
                write(fd, data + 4, recv_len - 4);
            
                //组数据发送给服务器
                data[1] = 4; 
                if(sendto(sockfd, data, 4, 0, (struct sockaddr *)&serveraddr, addrlen) < 0)
                {
                    ERRLOG("fail to sendto");
                }

                num = ntohs(*(unsigned short *)(data + 2));
            }
            //接收到的最后一次的数据
            else if(num + 1 == ntohs(*(unsigned short *)(data + 2)) && recv_len < 516)
            {
                write(fd, data + 4, recv_len - 4);
                break;
            }
        }
    }

    printf("文件下载成功\n");
}

void do_upload(int sockfd, struct sockaddr_in serveraddr)
{
    char filename[N] = {};
    printf("请输入要上传的文件名：");
    scanf("%s", filename);

    //打开文件并判断文件是否存在
    int fd;
    if((fd = open(filename, O_RDONLY)) < 0)
    {
        if(errno == ENOENT)
        {
            printf("文件%s不存在，请重新输入\n", filename);
            return ;
        }
        else
        {
            ERRLOG("fail to open");
        }
    }

    //组数据并发送给服务器执行上传功能
    char data[1024] = {};
    int data_len;
    socklen_t addrlen = sizeof(serveraddr);

    data_len = sprintf(data, "%c%c%s%c%s%c", 0, 2, filename, 0, "octet", 0);

    if(sendto(sockfd, data, data_len, 0, (struct sockaddr *)&serveraddr, addrlen) < 0)
    {
        ERRLOG("fail to sendto");
    }

    //接收服务器发送的数据并分析处理
    int recv_len;
    int num = 0;
    ssize_t bytes;
    while(1)
    {
        if((recv_len = recvfrom(sockfd, data, sizeof(data), 0, (struct sockaddr *)&serveraddr, &addrlen)) < 0)
        {
            ERRLOG("fail to recvfrom");
        }

        //printf("%d - %d\n", data[1], ntohs(*(unsigned short *)(data + 2)));
        //printf("%s\n", data + 4);

        if(data[1] == 4 && num == ntohs(*(unsigned short *)(data + 2)))
        {
            num++;
            bytes = read(fd, data + 4, 512);
            data[1] = 3;
            *(unsigned short *)(data + 2) = htons(num);

            if(bytes == 512)
            {
                if(sendto(sockfd, data, bytes + 4, 0, (struct sockaddr *)&serveraddr, addrlen) < 0)
                {
                    ERRLOG("fail to sendto");
                }
            }
            else
            {
                if(sendto(sockfd, data, bytes + 4, 0, (struct sockaddr *)&serveraddr, addrlen) < 0)
                {
                    ERRLOG("fail to sendto");
                }
                break;
            } 
        }
    }

    printf("文件上传完毕\n");
}

int main(int argc, char const *argv[])
{
    int sockfd;
    struct sockaddr_in serveraddr;
    //创建套接字
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        ERRLOG("fail to socket");
    }

    //填充服务器网络信息结构体
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(argv[1]);
    serveraddr.sin_port = htons(69);

    system("clear");
    printf("------------------------------\n");
    printf("----请输入help查看帮助信息----\n");
    printf("------------------------------\n");
    printf(">>> ");

    char buf[N] = {};
NEXT:
    fgets(buf, N, stdin);
    buf[strlen(buf) - 1] = '\0';
    if(strncmp(buf, "help", 4) == 0)
    {
        do_help();
    }
    else 
    {
        printf("您输入的有误，请重新输入\n");
        goto NEXT;
    }

    int num;
    while(1)
    {
        printf("input>>> ");
        scanf("%d", &num);
        switch (num)
        {
        case 1:
            do_download(sockfd, serveraddr);
            break;
        case 2:
            do_upload(sockfd, serveraddr);
            break;
        case 3:
            close(sockfd);
            exit(0);
            break;
        default:
            printf("您输入的有误，请重新输入\n");
            break;
        }
    }

    return 0;
}
