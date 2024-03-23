#include "msocket.h"

struct sembuf sem_lock = {0, -1, 0};
struct sembuf sem_unlock = {0, 1, 0};

int main()
{
    int s = m_socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
    {
        perror("socket error\n");
        return -1;
    }
    printf("socket created\n");
    printf("socket id = %d\n", s);

    long s_ip = inet_addr("127.0.0.1");
    int s_port = htons(1235);
    long d_ip = inet_addr("127.0.0.1");
    int d_port = htons(1234);

    int ret = m_bind(s, s_ip, s_port, d_ip, d_port);
    if (ret < 0)
    {
        perror("bind error\n");
        return -1;
    }

    printf("bind successful with s_ip = %ld, s_port = %d\n", s_ip, s_port);

    char buf[1024];
    // sleep(5);

    int fd = open("hello.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);

    while(1)
    {
        sleep(1);
        ret = m_recvfrom(s, buf, 1024, 0, d_ip, d_port);
        while(ret<0)
        {
            perror("recvfrom error");
            sleep(1);
            ret = m_recvfrom(s, buf, 1024, 0, d_ip, d_port);
        }

        if (strcmp(buf, "##########") == 0)
        {
            break;
        }

        write(fd, buf, strlen(buf));
        write(fd, "\n", 1);
    }

    // while(1);



    // ret = m_close(s);

    // if (ret < 0)
    // {
    //     perror("close error\n");
    //     return -1;
    // }

    // printf("socket closed\n");

    return 0;
}