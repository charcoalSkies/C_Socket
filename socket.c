#include "socket.h"

pthread_mutex_t g_mutex;    // mutex 선언
int g_clnt_socks[CLNT_MAX]; // 클라이언트 소켓 배열
int g_clnt_count = 0;       // 클라이언트 수
char recvMSG[BUFFSIZE];     // 수신 메시지 버퍼

int main(int args, char **argv)
{
    int sockfd, new_fd;
    struct sockaddr_in server_addr;
    uint8_t sin_size;

    /* buffer */
    int rcv_byte;
    char sendMSG[BUFFSIZE];
    char parent_recvMSG[BUFFSIZE];

    pthread_t t_thread;
    pthread_mutex_init(&g_mutex, NULL); // mutex 초기화

    int val = 1;

    sockfd = socket(AF_INET, SOCK_STREAM, 0); // 소켓 생성
    if (sockfd == -1)
    {
        perror("Server-socket() error!");
        exit(EXIT_FAILURE);
    }
    else
        printf("Server-socket() sockfd is OK...\n");

    server_addr.sin_family = AF_INET;

    server_addr.sin_port = htons(SERV_PORT); // 포트 설정

    server_addr.sin_addr.s_addr = INADDR_ANY; // IP 주소 설정

    memset(&(server_addr.sin_zero), 0, 8);

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&val, sizeof val) < 0) // 소켓 옵션 설정
    {
        perror("setsockopt");
        close(sockfd);
        return -1;
    }

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) // 소켓에 IP와 포트 할당
    {
        perror("Server-bind() error!");
        exit(EXIT_FAILURE);
    }
    else
        printf("Server-bind() is OK...\n");

    if (listen(sockfd, BACKLOG) == -1) // 클라이언트 연결 대기
    {
        perror("listen() error!");
        exit(EXIT_FAILURE);
    }
    else
        printf("listen() is OK...\n\n");

    while (1)
    {
        sin_size = sizeof(struct sockaddr_in);
        new_fd = accept(sockfd, (struct sockaddr *)&server_addr, &sin_size); // 클라이언트 연결 수락
        printf("accept() is OK...\n");
        printf("\n");
        pthread_mutex_lock(&g_mutex);                                     // mutex lock
        g_clnt_socks[g_clnt_count++] = new_fd;                            // 클라이언트 소켓 배열에 추가
        pthread_mutex_unlock(&g_mutex);                                   // mutex unlock
        pthread_create(&t_thread, NULL, clnt_connection, (void *)new_fd); // 클라이언트 연결 처리를 위한 스레드 생성
    }
    close(new_fd);
    close(sockfd);
    exit(0);
}

void *clnt_connection(void *arg)
{
    int new_fd = (int)arg;
    while (1)
    {
        recv(new_fd, recvMSG, sizeof(recvMSG), 0); // 클라이언트로부터 메시지 수신
        if (strcmp(recvMSG, "\0") == 0)            // 수신된 메시지가 NULL인 경우
        {
            printf("FD[%d]의 연결이 끊어졌습니다.\n", new_fd);
            close(new_fd);              // 소켓 연결 종료
            pthread_exit(EXIT_SUCCESS); // 스레드 종료
        }
        printf("%s\n", recvMSG);                   // 수신된 메시지 출력
        send(new_fd, recvMSG, strlen(recvMSG), 0); // 클라이언트로 메시지 전송
        // recvMSG[0] = '\0';
        // send_all_clnt(recvMSG, new_fd);

        recvMSG[0] = '\0'; // 수신 메시지 버퍼 초기화
    }

    pthread_mutex_lock(&g_mutex); // mutex lock
    for (int i = 0; i < g_clnt_count; i++)
    {
        if (new_fd == g_clnt_socks[i]) // 연결 종료된 클라이언트 소켓을 배열에서 제거
        {
            for (; i < g_clnt_count - 1; i++)
                g_clnt_socks[i] = g_clnt_socks[i + 1];
            break;
        }
    }
    pthread_mutex_lock(&g_mutex); // mutex unlock
    close(new_fd);                // 소켓 연결 종료
    pthread_exit(EXIT_SUCCESS);   // 스레드 종료
    return NULL;
}

void send_all_clnt(char *msg, int my_sock)
{
    pthread_mutex_lock(&g_mutex); // mutex lock
    for (int i = 0; i < g_clnt_count; i++)
    {
        if (g_clnt_socks[i] != my_sock) // 자신을 제외한 모든 클라이언트에게 메시지 전송
            write(g_clnt_socks[i], msg, strlen(msg) + 1);
    }
    pthread_mutex_unlock(&g_mutex); // mutex unlock
}
