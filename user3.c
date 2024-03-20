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
    strcpy(buf, "Hello");


    ret = m_sendto(s, buf, strlen(buf), 0, d_ip, d_port);
    if (ret < 0)
    {
        perror("sendto error\n");
        return -1;
    }
    printf("message sent\n");


    memset(buf, 0, sizeof(buf));
    strcpy(buf, "Hello2");

    ret = m_sendto(s, buf, strlen(buf), 0, d_ip, d_port);
    if (ret < 0)
    {
        perror("sendto error\n");
        return -1;
    }
    printf("message sent\n");

    sleep(8);

    ret = m_close(s);
    if (ret < 0)
    {
        perror("close error\n");
        return -1;
    }
    printf("socket closed\n");
    return 0;
}