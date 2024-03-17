typedef struct SOCK_INFO
{
    int sock_id;
    long ip;
    int port;
    int errnum;
} SOCK_INFO;

typedef struct msg
{
    char text[1024];
} msg;

typedef struct window
{
    int window_size; // useful for the sender to know how much space is there in receiver buffer
    int array[15];
    int left;
    int middle;
    int right;
} window;

typedef struct SM
{
    int alloted;
    pid_t pid;
    int mtp_id;
    int udp_id;
    long ip;
    int port;
    msg sendbuffer[10];
    int last_seq;
    int sendbuffer_in;
    int sendbuffer_out;
    msg recvbuffer[5];
    int recvbuffer_in;
    int recvbuffer_out;
    window swnd;
    window rwnd;
    int nospace;
    int flag;
} SM;