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
#include "struct.h"

#define T 5
#define P(s) semop(s, &sem_lock, 1)
#define V(s) semop(s, &sem_unlock, 1)

void* R();
void* S();

struct sembuf sem_lock = {0, -1, 0};
struct sembuf sem_unlock = {0, 1, 0};

void signal_handler(int signum)
{
    if(signum == SIGINT)
    {
        // delete the shared memory and semaphores
        key_t key = ftok("SM",2);
        int sm_id = shmget(key, sizeof(SM)*25, 0666);
        shmctl(sm_id, IPC_RMID, NULL);

        key_t sem_key = ftok("SM",1);
        int sem_id = semget(sem_key, 1, 0666);
        semctl(sem_id, 0, IPC_RMID, 0);

        key_t sockinfo = ftok("SM",3);
        int sockinfo_id = shmget(sockinfo, sizeof(SOCK_INFO), 0666);
        shmctl(sockinfo_id, IPC_RMID, NULL);

        key_t sem_key1 = ftok("SM",4);
        int sem_id1 = semget(sem_key1, 1, 0666);
        semctl(sem_id1, 0, IPC_RMID, 0);

        key_t sem_key2 = ftok("SM",5);
        int sem_id2 = semget(sem_key2, 1, 0666);
        semctl(sem_id2, 0, IPC_RMID, 0);

        printf("Shared memory and semaphores deleted\n");
        exit(0);
    }
}

int main()
{
    signal(SIGINT,signal_handler);
    // make semaphore for the shared memory
    key_t sem_key = ftok("SM",1);
    int sem_id = semget(sem_key, 1, IPC_CREAT | 0666);

    // initialise the semaphore
    semctl(sem_id, 0, SETVAL, 1);



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



void * R()
{
    // make semaphore for the shared memory
    key_t sem_key = ftok("SM",1);
    int sem_id = semget(sem_key, 1, 0666);

    // get the shared memory
}

void * S()
{
    // make an array of 25 that will store time of last msg sent
    time_t last_msg[25];

    memset(last_msg, 0, sizeof(time_t)*25);

    // make semaphore for the shared memory
    key_t sem_key = ftok("SM",1);
    int sem_id = semget(sem_key, 1, 0666);

    // get the shared memory
    key_t key = ftok("SM",2);
    int sm_id = shmget(key, sizeof(SM)*25, 0666);

    // attach the shared memory to the process
    SM *sm = (SM *)shmat(sm_id, NULL, 0);

    

    while(1)
    {
        sleep(T/2);
        P(sem_id);
        time_t now = time(NULL);

        // checking if there are any messages that require retranmission
        for(int i=0;i<25;i++)
        {
            if(sm[i].alloted == 1)
            {
                // find the time difference
                time_t diff = difftime(now, last_msg[i]);
                if(diff > T)
                {
                    // timeout on the socket
                    // resend the messages from from middle to right -1
                    
                    struct sockaddr_in server;
                    server.sin_family = AF_INET;
                    server.sin_port = htons(sm[i].port);
                    server.sin_addr.s_addr = sm[i].ip;

                    for(int j=sm[i].swnd.middle; j<sm[i].swnd.right; j++)
                    {
                        char message[1032];
                        // assigning the header
                        // since it is a data message
                        message[0] = '0';
                        message[1] = j/8 ? '1' : '0';
                        message[2] = (j%8)/4 ? '1' : '0';
                        message[3] = (j%4)/2 ? '1' : '0';
                        message[4] = (j%2) ? '1' : '0';

                        message[5] = message[6] = message[7] = '0'; 
                        message[8] = '\0';
                        strcat(message, sm[i].sendbuffer[j].text);
                        // padding the message
                        for(int k=strlen(sm[i].sendbuffer[j].text); k<1024; k++)
                            message[k+8] = '0';

                        // send the message
                        int n = sendto(sm[i].udp_id, message, 1032, 0, (struct sockaddr *)&server, sizeof(server));

                        if(n == -1)
                        {
                            perror("Error in sending the message\n");
                            pthread_exit(NULL);
                        }

                        last_msg[i] = time(NULL);

                    }
                }
            }
        }

        // now sender has to update the window 
        for(int i=0;i<25;i++)
        {
            if(sm[i].alloted == 1)
            {
                // move the left upto middle
                while(sm[i].swnd.left != sm[i].swnd.middle)
                {
                    sm[i].swnd.array[sm[i].swnd.left] = -1;
                    sm[i].swnd.left= (sm[i].swnd.left+1)%15;
                }
                sm[i].sendbuffer_out = sm[i].swnd.middle;                
            }

            // update right value 
            int temp = min(sm[i].swnd.middle+sm[i].swnd.window_size,sm[i].last_seq+1);

            while(sm[i].swnd.right != temp)
            {
                if(sm[i].swnd.right != -1)
                {
                    // send this message for the first time 
                    struct sockaddr_in server;
                    server.sin_family = AF_INET;
                    server.sin_port = htons(sm[i].port);
                    server.sin_addr.s_addr = sm[i].ip;

                    char message[1032];
                    // assigning the header
                    // since it is a data message

                    message[0] = '0';
                    message[1] = sm[i].swnd.right/8 ? '1' : '0';
                    message[2] = (sm[i].swnd.right%8)/4 ? '1' : '0';
                    message[3] = (sm[i].swnd.right%4)/2 ? '1' : '0';
                    message[4] = (sm[i].swnd.right%2) ? '1' : '0';

                    message[5] = message[6] = message[7] = '0';
                    message[8] = '\0';
                    strcat(message, sm[i].sendbuffer[sm[i].swnd.right].text);
                    // padding the message
                    for(int k=strlen(sm[i].sendbuffer[sm[i].swnd.right].text); k<1024; k++)
                        message[k+8] = '0';
                    
                    // send the message
                    int n = sendto(sm[i].udp_id, message, 1032, 0, (struct sockaddr *)&server, sizeof(server));

                    if(n == -1)
                    {
                        perror("Error in sending the message\n");
                        pthread_exit(NULL);
                    }

                    last_msg[i] = time(NULL);
                }
                sm[i].swnd.right = (sm[i].swnd.right+1)%15;
            }
        }
        V(sem_id);

    }
}