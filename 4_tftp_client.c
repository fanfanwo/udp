//tftp客户端的编写
#include <stdio.h> //printf
#include <stdlib.h> //exit
#include <sys/types.h>
#include <sys/socket.h> //socket
#include <netinet/in.h> //sockaddr_in
#include <arpa/inet.h> //htons inet_addr
#include <unistd.h> //close
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

//__FILE__：获取当前的文件名
//__func__：获取当前的函数名
//__LINE__：获取当前的行号
#define ERR_LOG(errmsg) do{\
                            perror(errmsg);\
                            printf("%s -- %s -- %d\n", __FILE__, __func__, __LINE__);\
                            exit(1);\
                        }while(0)

void do_help()
{
    system("clear");
    printf("************************\n");
    printf("****** 1.下载文件 ******\n");
    printf("****** 2.上传文件 ******\n");
    printf("****** 3.退出     ******\n");
    printf("************************\n");
}

void do_download(int sockfd, struct sockaddr_in serveraddr)
{
    char filename[128] = "";
    printf("请输入要下载的文件名: ");
    scanf("%s", filename);

    //给服务器发送消息，告知服务器执行下载操作
    unsigned char text[1024] = "";
    int text_len;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    int fd;
    int flags = 0;
    int num = 0;
    ssize_t bytes;

    text_len = sprintf(text, "%c%c%s%c%s%c", 0, 1, filename, 0, "octet", 0);
    if(sendto(sockfd, text, text_len, 0, (struct sockaddr *)&serveraddr, addrlen) < 0)
    {
        ERR_LOG("fail to sendto");
    }

    while(1)
    {
        //接收服务器发送过来的数据并处理,
        //本来可以不用再指定接收方的地址，但是 tftp协议的端口号是临时的，一直变化，所以要再次指定接收方
        if((bytes = recvfrom(sockfd, text, sizeof(text), 0, (struct sockaddr *)&serveraddr, &addrlen)) < 0)
        {
            ERR_LOG("fail to recvfrom");
        }

        //printf("操作码：%d, 块编号：%u\n", text[1], ntohs(*(unsigned short *)(text+2)));
        //printf("数据：%s\n", text+4);

        //判断操作码执行相应的处理
        if(text[1] == 5)  //text[1]里面存的是ASCII 码值，但是字符是3
        {
            printf("error: %s\n", text+4);
            return ;
        }
        else if(text[1] == 3)
        {
            if(flags == 0)
            {
                //创建文件
                if((fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0664)) < 0)
                {
                    ERR_LOG("fail to open");
                }
                flags = 1;
            }

            //对比快编号和接收的数据大小并将文件内容写入文件
            if((num+1 == ntohs(*(unsigned short *)(text+2))) && (bytes == 516))
            {
                num = ntohs(*(unsigned short *)(text+2));
                if(write(fd, text + 4, bytes - 4) < 0)
                {
                    ERR_LOG("fail to write");
                }

                //当文件写入完毕后，给服务器发送ACK
                text[1] = 4;
                if(sendto(sockfd, text, 4, 0, (struct sockaddr *)&serveraddr, addrlen) < 0)
                {
                    ERR_LOG("fail to sendto");
                }
            }
            //当最后一个数据接收完毕后，写入文件后退出函数
            else if((num+1 == ntohs(*(unsigned short *)(text+2))) && (bytes < 516))
            {
                if(write(fd, text + 4, bytes - 4) < 0)
                {
                    ERR_LOG("fail to write");
                }

                text[1] = 4;
                if(sendto(sockfd, text, 4, 0, (struct sockaddr *)&serveraddr, addrlen) < 0)
                {
                    ERR_LOG("fail to sendto");
                }

                printf("文件下载完毕\n");
                return ;
            }
        }
    }
}

int main(int argc, char const *argv[])
{
    if(argc < 2)
    {
        fprintf(stderr, "Usage: %s <server_ip>\n", argv[0]);
        exit(1);
    }

    int sockfd;
    struct sockaddr_in serveraddr;

    //创建套接字
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        ERR_LOG("fail to socket");
    }

    //填充服务器网络信息结构体
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(argv[1]);
    serveraddr.sin_port = htons(69); //tftp服务器的端口号默认是69

    system("clear");
    printf("-------------------------\n");
    printf("--- 请输入help查看选项 ---\n");
    printf("-------------------------\n");
    char buf[128] = "";
NEXT:
    printf(">>> ");
    fgets(buf, sizeof(buf), stdin);
    buf[strlen(buf) - 1] = '\0';
    if(strncmp(buf, "help", 4) == 0)
    {
        do_help();
    }
    else 
    {
        printf("您输入的有误，请重新输入help\n");
        goto NEXT;
    }

    int num;
    while(1)
    {
        printf("input: ");
        scanf("%d", &num);

        switch (num)
        {
        case 1:
            do_download(sockfd, serveraddr); //下载操作
            break;
        case 2:
            //do_upload(); //上传操作
            break;
        case 3:
            exit(0);
            break;
        default:
            printf("您输入的有误，请重新输入\n");
            break;
        } 
    }

    return 0;
}
