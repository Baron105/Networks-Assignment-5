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
#include "struct.h"

#define T 5
#define P_val 0.05

#define P(s) semop(s, &sem_lock, 1)
#define V(s) semop(s, &sem_unlock, 1)



int m_socket(int domain, int type, int protocol);
int m_bind(int sock,unsigned long s_ip, int s_port,unsigned long d_ip, int d_port);
int m_sendto(int sock, char *buf, int len, int flags,unsigned long d_ip, int d_port);
int m_recvfrom(int sock, char *buf, int len, int flags,unsigned long s_ip, int s_port);
int m_close(int sock);

int dropMessage(float p);
