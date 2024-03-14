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

    long s_ip = htonl(INADDR_ANY);
    int s_port = 1234;
    long d_ip = inet_addr("127.0.0.1");
    int d_port = 1235;

    int ret = m_bind(s, s_ip, s_port, d_ip, d_port);
    if (ret < 0)
    {
        perror("bind error\n");
        return -1;
    }
    printf("bind successful\n");

    char buf[1024];
    strcpy(buf, "Hello");

    ret = m_sendto(s, buf, strlen(buf), 0, d_ip, d_port);
    if (ret < 0)
    {
        perror("sendto error\n");
        return -1;
    }
    printf("message sent\n");

    ret = m_close(s);
    if (ret < 0)
    {
        perror("close error\n");
        return -1;
    }
    printf("socket closed\n");
    return 0;
}