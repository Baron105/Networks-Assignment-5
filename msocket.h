#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <time.h>


#define P(s) semop(s, &sem_lock, 1)
#define V(s) semop(s, &sem_unlock, 1)

struct sembuf sem_lock = {0, -1, 0};
struct sembuf sem_unlock = {0, 1, 0};


typedef struct{
    char text[1024];
}msg;

typedef struct{
    int window_size ;
    int array[15];
    int left ;
    int middle ;
    int right ;
}window;

typedef struct{
    int alloted ;
    pid_t pid;
    int mtp_id;
    int udp_id;
    long ip;
    int port;
    msg sendbuffer[10];
    int sendbuffer_in ;
    int sendbuffer_out ;
    msg recvbuffer[5];
    int recvbuffer_in ;
    int recvbuffer_out ;
    window swnd;
    window rwnd;
    int nospace;
}SM;

int m_socket(int domain, int type, int protocol);
int m_bind(int sock,long s_ip,int s_port,long d_ip,int d_port);
int m_sendto(int sock,char *buf,int len,int flags,long d_ip,int d_port);
int m_recvfrom(int sock,char *buf,int len,int flags,long s_ip,int s_port);
int m_close(int sock);

int dropMessage(float p);
