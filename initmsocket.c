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

void *R();
void *S();

struct sembuf sem_lock = {0, -1, 0};
struct sembuf sem_unlock = {0, 1, 0};

int new_bind[25] = {0}; // used to check if the socket is newly binding , taken care of by the R thread on timeout to add new socket to the fd set

void removeall()
{
    // delete the shared memory and semaphores
    key_t key = ftok("initmsocket.c", 2);
    int sm_id = shmget(key, sizeof(SM) * 25, 0666);
    shmctl(sm_id, IPC_RMID, NULL);

    key_t sem_key = ftok("initmsocket.c", 1);
    int sem_id = semget(sem_key, 1, 0666);
    semctl(sem_id, 0, IPC_RMID, 0);

    key_t sockinfo = ftok("initmsocket.c", 3);
    int sockinfo_id = shmget(sockinfo, sizeof(SOCK_INFO), 0666);
    shmctl(sockinfo_id, IPC_RMID, NULL);

    key_t sem_key1 = ftok("initmsocket.c", 4);
    int sem_id1 = semget(sem_key1, 1, 0666);
    semctl(sem_id1, 0, IPC_RMID, 0);

    key_t sem_key2 = ftok("initmsocket.c", 5);
    int sem_id2 = semget(sem_key2, 1, 0666);
    semctl(sem_id2, 0, IPC_RMID, 0);

    printf("Shared memory and semaphores deleted\n");
}

void signal_handler(int signum)
{
    if (signum == SIGINT || signum == SIGTSTP)
    {
        // removeall();
        exit(0);
    }
}

int main()
{
    signal(SIGINT, signal_handler);
    // make semaphore for the shared memory
    key_t sem_key = ftok("initmsocket.c", 1);
    int sem_id = semget(sem_key, 1, IPC_CREAT | 0666);

    // initialise the semaphore
    semctl(sem_id, 0, SETVAL, 1);

    // create a shared memory for SM
    key_t key = ftok("initmsocket.c", 2);
    int sm_id = shmget(key, sizeof(SM) * 25, IPC_CREAT | 0666);
    if (sm_id == -1)
    {
        perror("shmget error");
        exit(1);
    }

    // attach the shared memory to the process and initialse the SM to 0
    SM *sm = (SM *)shmat(sm_id, NULL, 0);

    memset(sm, 0, sizeof(SM) * 25);
    pthread_t r, s;
    pthread_create(&r, NULL, R, NULL);
    pthread_create(&s, NULL, S, NULL);

    // making the SOCKINFO shared memory and sem1 and sem2
    key_t sockinfo = ftok("initmsocket.c", 3);
    int sockinfo_id = shmget(sockinfo, sizeof(SOCK_INFO), IPC_CREAT | 0666);

    // attach the shared memory to the process and initialse the SM to 0
    SOCK_INFO *si = (SOCK_INFO *)shmat(sockinfo_id, NULL, 0);
    memset(si, 0, sizeof(SOCK_INFO));

    // make semaphore for the shared memory
    key_t sem_key1 = ftok("initmsocket.c", 4);
    int sem_id1 = semget(sem_key1, 1, IPC_CREAT | 0666);
    key_t sem_key2 = ftok("initmsocket.c", 5);
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
    while (1)
    {
        P(sem_id1);
        // check if all fields of si are 0
        if (si->sock_id == 0 && si->ip == 0 && si->port == 0 && si->errnum == 0)
        {
            // it is a socket call
            // create a udp socket
            si->sock_id = socket(AF_INET, SOCK_DGRAM, 0);
            if (si->sock_id == -1)
            {
                si->errnum = errno;
            }
        }
        else if (si->sock_id != 0 && si->ip != 0 && si->port != 0)
        {
            // it is a bind call
            struct sockaddr_in server;
            server.sin_family = AF_INET;
            server.sin_port = htons(si->port);
            server.sin_addr.s_addr = si->ip;

            if (bind(si->sock_id, (struct sockaddr *)&server, sizeof(server)) < 0)
            {
                si->errnum = errno;
                si->sock_id = -1;
            }
            else
            {
                // find the index in the shared memory
                int i;
                for (i = 0; i < 25; i++)
                {
                    if (si->sock_id == sm[i].udp_id)
                        break;
                }
                new_bind[i] = 1;
            }
        }
        V(sem_id2);
    }
}

// have delete the  shared memory and semaphore
// handle garbage

void *S()
{
    // make an array of 25 that will store time of last msg sent
    time_t last_msg[25][15];

    memset(last_msg, -1, sizeof(last_msg));

    // make semaphore for the shared memory
    key_t sem_key = ftok("initmsocket.c", 1);
    int sem_id = semget(sem_key, 1, 0666);

    // get the shared memory
    key_t key = ftok("initmsocket.c", 2);
    int sm_id = shmget(key, sizeof(SM) * 25, 0666);

    // attach the shared memory to the process
    SM *sm = (SM *)shmat(sm_id, NULL, 0);

    while (1)
    {
        sleep(T / 2);
        P(sem_id);
        time_t now = time(NULL);

        // checking if there are any messages that require retranmission
        for (int i = 0; i < 25; i++)
        {
            if (sm[i].alloted == 1)
            {
                struct sockaddr_in server;
                server.sin_family = AF_INET;
                server.sin_port = htons(sm[i].port);
                server.sin_addr.s_addr = sm[i].ip;
                // find the time difference
                time_t diff;

                for (int j = sm[i].swnd.middle; j < sm[i].swnd.right; j++)
                {
                    diff = difftime(now, last_msg[i][j]);
                    if (diff > T)
                    {
                        // timeout on the socket
                        // resend the messages from from middle to right -1

                        char message[1032];
                        // assigning the header
                        // since it is a data message
                        message[0] = '0';
                        message[1] = j / 8 ? '1' : '0';
                        message[2] = (j % 8) / 4 ? '1' : '0';
                        message[3] = (j % 4) / 2 ? '1' : '0';
                        message[4] = (j % 2) ? '1' : '0';

                        message[5] = message[6] = message[7] = '0';
                        message[8] = '\0';
                        strcat(message, sm[i].sendbuffer[j].text);
                        // padding the message
                        for (int k = strlen(sm[i].sendbuffer[j].text); k < 1024; k++)
                            message[k + 8] = '0';

                        // send the message
                        int n = sendto(sm[i].udp_id, message, 1032, 0, (struct sockaddr *)&server, sizeof(server));

                        if (n == -1)
                        {
                            perror("Error in sending the message\n");
                            // removeall();
                            pthread_exit(NULL);
                        }

                        last_msg[i][j] = time(NULL);
                    }
                }
            }
        }

        // now sender has to update the window
        for (int i = 0; i < 25; i++)
        {
            if (sm[i].alloted == 1)
            {
                // move the left upto middle
                while (sm[i].swnd.left != sm[i].swnd.middle)
                {
                    sm[i].swnd.array[sm[i].swnd.left] = -1;
                    sm[i].swnd.left = (sm[i].swnd.left + 1) % 15;
                }
                sm[i].sendbuffer_out = sm[i].swnd.middle;
            }

            // update right value
            int temp ;
            if(sm[i].swnd.middle + sm[i].swnd.window_size < sm[i].last_seq + 1)
            {
                temp = sm[i].swnd.middle + sm[i].swnd.window_size;
            }
            else 
            {
                temp = sm[i].last_seq + 1;
            }

            while (sm[i].swnd.right != temp)
            {
                if (sm[i].swnd.right != -1)
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
                    message[1] = sm[i].swnd.right / 8 ? '1' : '0';
                    message[2] = (sm[i].swnd.right % 8) / 4 ? '1' : '0';
                    message[3] = (sm[i].swnd.right % 4) / 2 ? '1' : '0';
                    message[4] = (sm[i].swnd.right % 2) ? '1' : '0';

                    message[5] = message[6] = message[7] = '0';
                    message[8] = '\0';
                    strcat(message, sm[i].sendbuffer[sm[i].swnd.right].text);
                    // padding the message
                    for (int k = strlen(sm[i].sendbuffer[sm[i].swnd.right].text); k < 1024; k++)
                        message[k + 8] = '0';

                    // send the message
                    int n = sendto(sm[i].udp_id, message, 1032, 0, (struct sockaddr *)&server, sizeof(server));

                    if (n == -1)
                    {
                        perror("Error in sending the message\n");
                        // removeall();
                        pthread_exit(NULL);
                    }

                    last_msg[i][sm[i].swnd.right] = time(NULL);
                }
                sm[i].swnd.right = (sm[i].swnd.right + 1) % 15;
            }
        }
        V(sem_id);
    }

    // detach the shared memory
    shmdt(sm);
}

void *R()
{
    // make semaphore for the shared memory
    key_t sem_key = ftok("initmsocket.c", 1);
    int sem_id = semget(sem_key, 1, 0666);

    // get the shared memory
    key_t key = ftok("initmsocket.c", 2);
    int sm_id = shmget(key, sizeof(SM) * 25, 0666);

    // attach the shared memory to the process
    SM *sm = (SM *)shmat(sm_id, NULL, 0);

    fd_set fd;
    fd_set readfds;

    struct timeval tv;
    tv.tv_sec = T;
    tv.tv_usec = 0;

    int timeout_cnt[25] = {0};

    FD_ZERO(&fd);

    while (1)
    {
        readfds = fd;

        int n = select(FD_SETSIZE, &readfds, NULL, NULL, &tv);

        if (n == -1)
        {
            perror("Error in select\n");
            // removeall();
            pthread_exit(NULL);
        }

        if (n == 0)
        {
            // timeout
            P(sem_id);
            // check if we have new sockets
            for (int i = 0; i < 25; i++)
            {
                if (new_bind[i] == 1)
                {
                    new_bind[i] = 0;
                    FD_SET(sm[i].udp_id, &fd);
                }
            }

            tv.tv_sec = T;
            tv.tv_usec = 0;

            // increment the timeout counter
            for (int i = 0; i < 25; i++)
            {
                if (sm[i].alloted == 1)
                {
                    timeout_cnt[i]++;
                    if ((timeout_cnt[i] == 3 || sm[i].flag == 1) && ((sm[i].swnd.middle > 0) || (sm[i].swnd.middle == 0 && sm[i].swnd.left != -1)))
                    {
                        // send an ack for the last message received in order
                        int seq = (sm[i].rwnd.middle - 1 + 15) % 15;
                        int rem = (sm[i].rwnd.right - sm[i].rwnd.middle + 15) % 15;

                        char message[8];

                        message[0] = '1';
                        message[1] = seq / 8 ? '1' : '0';
                        message[2] = (seq % 8) / 4 ? '1' : '0';
                        message[3] = (seq % 4) / 2 ? '1' : '0';
                        message[4] = (seq % 2) ? '1' : '0';

                        message[5] = rem / 4 ? '1' : '0';
                        message[6] = (rem % 4) / 2 ? '1' : '0';
                        message[7] = (rem % 2) ? '1' : '0';

                        struct sockaddr_in server;
                        server.sin_family = AF_INET;
                        server.sin_port = htons(sm[i].port);
                        server.sin_addr.s_addr = sm[i].ip;

                        int n = sendto(sm[i].udp_id, message, 8, 0, (struct sockaddr *)&server, sizeof(server));

                        if (n == -1)
                        {
                            perror("Error in sending the ack\n");
                            // removeall();
                            pthread_exit(NULL);
                        }
                        if (sm[i].flag == 1)
                        {
                            sm[i].flag = 0;
                        }
                        timeout_cnt[i] = 0;
                    }
                }
            }
            V(sem_id);
            continue;
        }

        // we have some activity
        for (int i = 0; i < 25; i++)
        {
            P(sem_id);
            if (sm[i].alloted)
            {
                if (FD_ISSET(sm[i].udp_id, &readfds))
                {
                    // so 2 possibilities
                    // 1. it is an ack
                    // 2. it is a data message

                    char header[8];
                    struct sockaddr_in server;
                    socklen_t len = sizeof(server);
                    int n = recvfrom(sm[i].udp_id, header, 8, MSG_DONTWAIT, (struct sockaddr *)&server, &len);

                    if (server.sin_addr.s_addr != sm[i].ip || server.sin_port != sm[i].port)
                    {
                        // not the correct socket
                        printf("Not the correct socket\n");
                        V(sem_id);
                        continue;
                    }

                    if (n == -1)
                    {
                        perror("Error in receiving the message\n");
                        // removeall();
                        pthread_exit(NULL);
                    }

                    while (n > 0)
                    {
                        // we have received a message
                        // check if it is an ack or a data message
                        if (header[0] == '0')
                        {
                            // it is a data message
                            // recv the data
                            char message[1024];
                            n = recvfrom(sm[i].udp_id, message, 1024, MSG_DONTWAIT, (struct sockaddr *)&server, &len);

                            int seq = (header[1] - '0') * 8 + (header[2] - '0') * 4 + (header[3] - '0') * 2 + (header[4] - '0');

                            // check if the message is in the window
                            if (seq >= sm[i].rwnd.middle && seq < sm[i].rwnd.right)
                            {
                                // it is in the window

                                // check if it is a duplicate message
                                if (sm[i].rwnd.array[seq] == -1)
                                {
                                    // not a duplicate msg
                                    // check if it is the next message in order
                                    if (seq == sm[i].rwnd.middle)
                                    {
                                        // inorder msg
                                        sm[i].rwnd.array[seq] = seq % 5;
                                        strcpy(sm[i].recvbuffer[seq % 5].text, message);
                                        sm[i].rwnd.middle = (sm[i].rwnd.middle + 1) % 15;
                                    }
                                    else
                                    {
                                        // out of order msg
                                        sm[i].rwnd.array[seq] = seq % 5;
                                        strcpy(sm[i].recvbuffer[seq % 5].text, message);
                                    }

                                    // send an ack for the last message received in order
                                    int seq = (sm[i].rwnd.middle - 1 + 15) % 15;
                                    int rem = (sm[i].rwnd.right - sm[i].rwnd.middle + 15) % 15;

                                    if (rem == 0)
                                    {
                                        // no space
                                        sm[i].nospace = 1;
                                    }

                                    char ackm[8];

                                    ackm[0] = '1';
                                    ackm[1] = seq / 8 ? '1' : '0';
                                    ackm[2] = (seq % 8) / 4 ? '1' : '0';
                                    ackm[3] = (seq % 4) / 2 ? '1' : '0';
                                    ackm[4] = (seq % 2) ? '1' : '0';

                                    ackm[5] = rem / 4 ? '1' : '0';
                                    ackm[6] = (rem % 4) / 2 ? '1' : '0';
                                    ackm[7] = (rem % 2) ? '1' : '0';

                                    int n = sendto(sm[i].udp_id, ackm, 8, 0, (struct sockaddr *)&server, sizeof(server));

                                    if (n == -1)
                                    {
                                        perror("Error in sending the ack\n");
                                        // removeall();
                                        pthread_exit(NULL);
                                    }
                                }
                            }
                        }
                        else
                        {
                            // it is an ack
                            // it is not a bad guy also

                            int seq = (header[1] - '0') * 8 + (header[2] - '0') * 4 + (header[3] - '0') * 2 + (header[4] - '0');
                            int rem = (header[5] - '0') * 4 + (header[6] - '0') * 2 + (header[7] - '0');

                            // update the window
                            // check if the seq is in the window
                            if (seq >= sm[i].swnd.middle && seq < sm[i].swnd.right)
                            {
                                // it is in window , we can update the middle
                                sm[i].swnd.middle = (seq + 1) % 15;
                            }

                            // update the receive window size
                            sm[i].swnd.window_size = rem;
                        }
                        memset(header, '\0', 8);
                        n = recvfrom(sm[i].udp_id, header, 8, MSG_DONTWAIT, (struct sockaddr *)&server, &len);
                        // bad guy check
                        while (n > 0 && (server.sin_addr.s_addr != sm[i].ip || server.sin_port != sm[i].port))
                        {
                            // not the correct socket
                            printf("Not the correct socket\n");

                            // receive the message
                            char message[1024];

                            if (header[0] == '0')
                            {
                                // data message
                                n = recvfrom(sm[i].udp_id, message, 1024, MSG_DONTWAIT, (struct sockaddr *)&server, &len);
                            }
                            memset(header, '\0', 8);

                            n = recvfrom(sm[i].udp_id, header, 8, MSG_DONTWAIT, (struct sockaddr *)&server, &len);
                        }
                    }
                }
            }
            V(sem_id);
        }
    }

    // detach the shared memory
    shmdt(sm);
}