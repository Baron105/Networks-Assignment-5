#include "msocket.h"

struct sembuf sem_lock = {0, -1, 0};
struct sembuf sem_unlock = {0, 1, 0};

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
    int s_port = htons(1234);
    long d_ip = inet_addr("127.0.0.1");
    int d_port = htons(1235);

    int ret = m_bind(s, s_ip, s_port, d_ip, d_port);
    if (ret < 0)
    {
        perror("bind error\n");
        return -1;
    }
    printf("bind successful with s_ip = %ld, s_port = %d\n", s_ip, s_port);

    sleep(5);

    char buf[1024];

    int i;
    
    for (i = 0; i < 100; i++)
    {
        sprintf(buf, "Hello%d", i);

        sleep(1);


        ret = m_sendto(s, buf, strlen(buf), 0, d_ip, d_port);
        while(ret<0)
        {
            perror("sendto error\n");
            sleep(1);
            ret = m_sendto(s, buf, strlen(buf), 0, d_ip, d_port);
        }
        // return -1;
        printf("message put in buf=%s\n",buf);

        memset(buf, 0, sizeof(buf));
    }

    // write terminal message
    strcpy(buf, "##########");
    ret = m_sendto(s, buf, strlen(buf), 0, d_ip, d_port);
    while(ret<0)
    {
        perror("sendto error\n");
        sleep(1);
        ret = m_sendto(s, buf, strlen(buf), 0, d_ip, d_port);
    }


    memset(buf, 0, sizeof(buf));

    // get the semaphore for the shared memory
    key_t sem_key = ftok("initmsocket.c", 1);
    int sem_id = semget(sem_key, 1, 0666 | IPC_CREAT);

    // get the shared memory
    key_t key = ftok("initmsocket.c", 2);
    int sm_id = shmget(key, sizeof(SM) * 25, 0666 | IPC_CREAT);

    // attach the shared memory to the process
    SM *sm = (SM *)shmat(sm_id, NULL, 0);

    sleep(30);

    P(sem_id);
    double avg_transmission_cnt = (1.0*sm[s-1].transmission_cnt) / (i);
    printf("Average transmission count = %lf\n", avg_transmission_cnt);
    V(sem_id);



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

    // // attach the shared memory to the process
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

    // shmdt(sm);

    while(1);

    return 0;
}