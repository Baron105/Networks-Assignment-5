#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <pthread.h>

#define T 5
#define P(s) semop(s, &sem_lock, 1)
#define V(s) semop(s, &sem_unlock, 1)

void* R();
void* S();



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


    while(1)
    {
        sleep(2*T);

        // do garbage collection
        for(int i=0; i<25; i++)
        {
            if(sm[i].alloted == 0)
            {
                // close the udp socket
                if(sm[i].udp_id != 0)
                    close(sm[i].udp_id);

                memset(&sm[i], 0, sizeof(SM));
            }
        }
    }

}


// have delete the  shared memory and semaphore