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
#include <pthread.h>

#define T 5
#define P(s) semop(s, &sem_lock, 1)
#define V(s) semop(s, &sem_unlock, 1)

void* R();
void* S();

typedef struct{
    int sock_id;
    long ip;
    int port;
    int errnum;
}SOCK_INFO;

typedef struct{
    char text[1024];
}msg;


typedef struct{
    int window_size ; // useful for the sender to know how much space is there in receiver buffer
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
    int flag;
}SM;

int main()
{
    // make semaphore for the shared memory
    key_t sem_key = ftok("SM",1);
    int sem_id = semget(sem_key, 1, IPC_CREAT | 0666);

    // initialise the semaphore
    semctl(sem_id, 0, SETVAL, 1);

    struct sembuf sem_lock = {0, -1, 0};
    struct sembuf sem_unlock = {0, 1, 0};


    // create a shared memory for SM
    key_t key = ftok("SM",2);
    int sm_id = shmget(key, sizeof(SM)*25, IPC_CREAT | 0666);
    if(sm_id == -1)
    {
        perror("shmget error");
        exit(1);
    }

    // attach the shared memory to the process and initialse the SM to 0
    SM *sm = (SM *)shmat(sm_id, NULL, 0);

    memset(sm, 0, sizeof(SM)*25);
    pthread_t r,s;
    pthread_create(&r, NULL, R, NULL);
    pthread_create(&s, NULL, S, NULL);


    // making the SOCKINFO shared memory and sem1 and sem2
    key_t sockinfo = ftok("SM",3);
    int sockinfo_id = shmget(sockinfo, sizeof(SOCK_INFO), IPC_CREAT | 0666);

    // attach the shared memory to the process and initialse the SM to 0
    SOCK_INFO *si = (SOCK_INFO *)shmat(sockinfo_id, NULL, 0);
    memset(si, 0, sizeof(SOCK_INFO));

    // make semaphore for the shared memory
    key_t sem_key1 = ftok("SM",4);
    int sem_id1 = semget(sem_key1, 1, IPC_CREAT | 0666);
    key_t sem_key2 = ftok("SM",5);
    int sem_id2 = semget(sem_key2, 1, IPC_CREAT | 0666);

    // initialise the semaphore
    semctl(sem_id1, 0, SETVAL, 0);
    semctl(sem_id2, 0, SETVAL, 0);



    // garbage collection
    // while(1)
    // {
    //     sleep(2*T);

    //     // do garbage collection
    //     for(int i=0; i<25; i++)
    //     {
    //         if(sm[i].alloted == 0)
    //         {
    //             // close the udp socket
    //             if(sm[i].udp_id != 0)
    //                 close(sm[i].udp_id);

    //             memset(&sm[i], 0, sizeof(SM));
    //         }
    //     }
    // }


    // creating or binding the socket
    while(1)
    {
        P(sem_id1);
        // check if all fields of si are 0
        if(si->sock_id == 0 && si->ip == 0 && si->port == 0 && si->errnum == 0)
        {
            // it is a socket call 
            // create a udp socket
            si->sock_id = socket(AF_INET, SOCK_DGRAM, 0);
            if(si->sock_id == -1)
            {
                si->errnum = errno;
            }
            
        }
        else if(si->sock_id != 0 && si->ip != 0 && si->port != 0)
        {
            // it is a bind call
            struct sockaddr_in server;
            server.sin_family = AF_INET;
            server.sin_port = htons(si->port);
            server.sin_addr.s_addr = si->ip;

            if(bind(si->sock_id, (struct sockaddr *)&server, sizeof(server)) < 0)
            {
                si->errnum = errno;
                si->sock_id = -1;
            }
        }
        V(sem_id2);
    }

}


// have delete the  shared memory and semaphore
// handle garbage