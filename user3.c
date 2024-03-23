#include "msocket.h"

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
    int s_port = htons(1236);
    long d_ip = inet_addr("127.0.0.1");
    int d_port = htons(1237);

    int ret = m_bind(s, s_ip, s_port, d_ip, d_port);
    if (ret < 0)
    {
        perror("bind error\n");
        return -1;
    }
    printf("bind successful with s_ip = %ld, s_port = %d\n", s_ip, s_port);

    sleep(5);

    char buf[1024];

    int fd = open("romeo.txt", O_RDONLY);

    while (1)
    {
        sleep(1);
        int x = read(fd, buf, 1023);
        if (x == 0)
        {
            break;
        }
        ret = m_sendto(s, buf, strlen(buf), 0, d_ip, d_port);
        while (ret < 0)
        {
            perror("sendto error\n");
            sleep(1);
            ret = m_sendto(s, buf, strlen(buf), 0, d_ip, d_port);
        }
        printf("message put in buf=%s\n", buf);

        memset(buf, 0, sizeof(buf));
    }

    // terminal message
    // 10 # symbols
    strcpy(buf, "##########");

    ret = m_sendto(s, buf, strlen(buf), 0, d_ip, d_port);
    while (ret < 0)
    {
        perror("sendto error\n");
        sleep(1);
        ret = m_sendto(s, buf, strlen(buf), 0, d_ip, d_port);
    }

    sleep(100);

    // ret = m_close(s);
    // if (ret < 0)
    // {
    //     perror("close error\n");
    //     return -1;
    // }
    // printf("socket closed\n");

    // get the shared memory
    // key_t key = ftok("initmsocket.c", 2);
    // int sm_id = shmget(key, sizeof(SM) * 25, 0666|IPC_CREAT);

    // attach the shared memory to the process
    // SM *sm = (SM *)shmat(sm_id, NULL, 0);

    // for(int i=15;i<100;i++)
    // {
    //     printf("%d\n",i);
    //     for(int j=0;j<10;j++)
    //     {
    //         printf("sendbuffer[%d] = %s\n", j, sm[0].sendbuffer[j].text);
    //     }
    //     printf("\n");
    //     sleep(1);
    // }

    return 0;
}