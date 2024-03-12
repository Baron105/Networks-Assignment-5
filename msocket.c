#include "msocket.h"

int m_socket(int domain, int type, int protocol)
{
    // get the semaphore for the shared memory
    key_t sem_key = ftok("SM",1);
    int sem_id = semget(sem_key, 1, 0666);

    // get the shared memory
    key_t key = ftok("SM",2);
    int sm_id = shmget(key, sizeof(SM)*25, 0666);

    // attach the shared memory to the process
    SM *sm = (SM *)shmat(sm_id, NULL, 0);


    P(sem_id);
    // find the first free SM
    int i;
    for(i=0; i<25; i++)
    {
        if(sm[i].alloted == 0 && sm[i].udp_id == 0)
        {
            sm[i].alloted = 1;
            break;
        }
    }

    if(i>=25){

        // set errno to ENOBUFS
        errno = ENOBUFS;
        perror("No space available in SM\n");
        V(sem_id);
        return -1;
    }

    // create a udp socket
    int udp_id = socket(domain, type, protocol);
    if(udp_id<0)
    {
        perror("socket error\n");
        memset(&sm[i], 0, sizeof(SM));
        V(sem_id);
        return -1;
    }

    // set the udp_id in the SM
    sm[i].udp_id = udp_id;
    sm[i].mtp_id = i+1;
    sm[i].pid = getpid();

    // set recieve buffer to \0
    for(int j=0; j<5; j++)
        memset(sm[i].recvbuffer[j].text, '\0', 1024);

    // set the window size
    sm[i].swnd.window_size = 5;
    sm[i].swnd.left = -1;
    sm[i].swnd.middle = -1;
    sm[i].swnd.right = -1;

    sm[i].rwnd.window_size = 5;
    sm[i].rwnd.left = 0;
    sm[i].rwnd.middle = 0;
    sm[i].rwnd.right = 5;

    // set the sendand receive buffer to variables
    sm[i].sendbuffer_in = -1;
    sm[i].sendbuffer_out = -1;

    sm[i].recvbuffer_in = -1;
    sm[i].recvbuffer_out = -1;

    V(sem_id);


    return i+1;
}

int m_bind(int sock,long s_ip,int s_port,long d_ip,int d_port)
{
    // get the semaphore for the shared memory
    key_t sem_key = ftok("SM",1);
    int sem_id = semget(sem_key, 1, 0666);

    // get the shared memory
    key_t key = ftok("SM",2);
    int sm_id = shmget(key, sizeof(SM)*25, 0666);

    // attach the shared memory to the process
    SM *sm = (SM *)shmat(sm_id, NULL, 0);

    P(sem_id);

    // find the SM with the given sock
    int i;
    for(i=0; i<25; i++)
    {
        if(sm[i].mtp_id == sock)
            break;
    }

    if(i>=25)
    {
        // set errno to EBADF
        errno = EBADF;
        perror("No such socket\n");
        V(sem_id);
        return -1;
    }

    // bind the udp socket
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(s_port);
    addr.sin_addr.s_addr = s_ip;

    int ret = bind(sm[i].udp_id, (struct sockaddr *)&addr, sizeof(addr));
    if(ret<0)
    {
        perror("bind error\n");
        V(sem_id);
        return -1;
    }

    sm[i].ip = s_ip;
    sm[i].port = s_port;

    V(sem_id);

    return 0;

}

int m_sendto(int sock,char *buf,int len,int flags,long d_ip,int d_port)
{
    // get the semaphore for the shared memory
    key_t sem_key = ftok("SM",1);
    int sem_id = semget(sem_key, 1, 0666);

    // get the shared memory
    key_t key = ftok("SM",2);
    int sm_id = shmget(key, sizeof(SM)*25, 0666);

    // attach the shared memory to the process
    SM *sm = (SM *)shmat(sm_id, NULL, 0);

    P(sem_id);

    // find the SM with the given sock
    int i;
    for(i=0; i<25; i++)
    {
        if(sm[i].mtp_id == sock)
            break;
    }

    if(i>=25)
    {
        // set errno to EBADF
        errno = EBADF;
        perror("No such socket\n");
        V(sem_id);
        return -1;
    }

    char msg[1024];
    memset(msg, '\0', 1024);
    strcpy(msg, buf);

    // pad the message with \0
    for(int j=strlen(buf); j<1024; j++)
        msg[j] = '\0';

    // check is the d_ip and d_port are valid
    if(sm[i].ip != d_ip || sm[i].port != d_port)
    {
        printf("ENOTBOUND error.Invalid destination or port. \n");
        V(sem_id);
        return -1;
    }

    // check if the send buffer is full
    if((sm[i].sendbuffer_in+1)%10 == sm[i].sendbuffer_out)
    {
        errno = ENOBUFS;
        perror("Send buffer full\n");
        V(sem_id);
        return -1;
    }

    if(sm[i].sendbuffer_in == -1 && sm[i].sendbuffer_out == -1)
    {
        sm[i].sendbuffer_in = 0;
        sm[i].sendbuffer_out = 0;
        strcpy(sm[i].sendbuffer[sm[i].sendbuffer_in].text, msg);
    }
    else 
    {
        sm[i].sendbuffer_in = (sm[i].sendbuffer_in+1)%10;
        strcpy(sm[i].sendbuffer[sm[i].sendbuffer_in].text, msg);
    }

    V(sem_id);

    return len;
}

int m_recvfrom(int sock,char *buf,int len,int flags,long s_ip,int s_port)
{
    // get the semaphore for the shared memory
    key_t sem_key = ftok("SM",1);
    int sem_id = semget(sem_key, 1, 0666);

    // get the shared memory
    key_t key = ftok("SM",2);
    int sm_id = shmget(key, sizeof(SM)*25, 0666);

    // attach the shared memory to the process
    SM *sm = (SM *)shmat(sm_id, NULL, 0);

    P(sem_id);

    // find the SM with the given sock
    int i;
    for(i=0; i<25; i++)
    {
        if(sm[i].mtp_id == sock)
            break;
    }

    if(i>=25)
    {
        // set errno to EBADF
        errno = EBADF;
        perror("No such socket\n");
        V(sem_id);
        return -1;
    }

    // check is the s_ip and s_port are valid
    if(sm[i].ip != s_ip || sm[i].port != s_port)
    {
        printf("ENOTBOUND error.Invalid source or port. \n");
        V(sem_id);
        return -1;
    }

    // check if the recv buffer is empty
    if(sm[i].recvbuffer_in == -1 && sm[i].recvbuffer_out == -1)
    {
        errno = ENOMSG;
        perror("Recv buffer empty\n");
        V(sem_id);
        return -1;
    }

    if(sm[i].recvbuffer_in == sm[i].recvbuffer_out)
    {
        if(strncmp(sm[i].recvbuffer[sm[i].recvbuffer_out].text, "\0",1) == 0)
        {
            errno = ENOMSG;
            perror("Recv buffer empty\n");
            V(sem_id);
            return -1;
        }
        strcpy(buf, sm[i].recvbuffer[sm[i].recvbuffer_out].text);
        memset(sm[i].recvbuffer[sm[i].recvbuffer_out].text, '\0', 1024);
        sm[i].recvbuffer_in = -1;
        sm[i].recvbuffer_out = -1;
    }
    else 
    {
        if(strncmp(sm[i].recvbuffer[sm[i].recvbuffer_out].text, "\0",1) == 0)
        {
            errno = ENOMSG;
            perror("Recv buffer empty\n");
            V(sem_id);
            return -1;
        }
        strcpy(buf, sm[i].recvbuffer[sm[i].recvbuffer_out].text);
        memset(sm[i].recvbuffer[sm[i].recvbuffer_out].text, '\0', 1024);
        sm[i].recvbuffer_out = (sm[i].recvbuffer_out+1)%5;
    }

    V(sem_id);

    return strlen(buf);
}

int m_close(int sock)
{
    // get the semaphore for the shared memory
    key_t sem_key = ftok("SM",1);
    int sem_id = semget(sem_key, 1, 0666);

    // get the shared memory
    key_t key = ftok("SM",2);
    int sm_id = shmget(key, sizeof(SM)*25, 0666);

    // attach the shared memory to the process
    SM *sm = (SM *)shmat(sm_id, NULL, 0);

    P(sem_id);

    // find the SM with the given sock
    int i;
    for(i=0; i<25; i++)
    {
        if(sm[i].mtp_id == sock)
            break;
    }

    if(i>=25)
    {
        // set errno to EBADF
        errno = EBADF;
        perror("No such socket\n");
        V(sem_id);
        return -1;
    }

    // close the udp socket
    if(sm[i].udp_id != 0){
        if(close(sm[i].udp_id)<0){
            perror("close error\n");
            V(sem_id);
            return -1;
        }
        
    }

    memset(&sm[i], 0, sizeof(SM));

    V(sem_id);

    return 0;
}

int dropMessage(float p)
{
    // generate a random number between 0 and 1, if it is less than p, return 1, else return 0
    float r = (float)rand()/(float)RAND_MAX;
    if(r<p)
        return 1;
    else
        return 0;
}