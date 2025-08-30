#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <signal.h>

#define MAXNAMESIZE 20
#define MAXMESSAGESIZE 100
#define DEFAULT_PORT 3381
#define MAXCLIENTCOUNT 10
#define MAXBECKLOG 10

typedef struct {
    int fds;
    pthread_t tid;
    char name[MAXNAMESIZE];
    int index;
} ThreadInfo;

ThreadInfo thread[MAXCLIENTCOUNT];
int client_count = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void handler(int signum){
	printf("Process catch an SIGPIPE:\n Something goes wrong:\n Server disconnecting:\n");
	exit(EXIT_FAILURE);
return;
}

void* ThreadFunction(void* arg){
    int index = *(int*)arg;
    char buffer[MAXMESSAGESIZE];
    while(1){
        memset(buffer, 0, sizeof(buffer));
        int recv_retval = recv(thread[index].fds, buffer, sizeof(buffer) - 1, 0);
        if(recv_retval == 0 || recv_retval == -1){
            printf("Connection lost to %s client...\n", thread[index].name);
            pthread_exit(NULL);
        }
        buffer[recv_retval] = '\0';
        if(strncmp(buffer, "exit", strlen("exit")) == 0){
            printf("Connection lost from %s:\n", thread[index].name);
            close(thread[index].fds);
            pthread_exit(NULL);
        }
        char* tmp = strchr(buffer, '@');
        if(tmp == NULL){
            char meesage[MAXMESSAGESIZE + MAXNAMESIZE];
            strcpy(meesage,thread[index].name);
            strcpy(meesage + strlen(thread[index].name), ": ");
            strcpy(meesage + strlen(thread[index].name) + 2, buffer);

            printf("Message broadcasting ...\n");
            pthread_mutex_lock(&mutex);
            for(int index1 = 0; index1 < client_count; ++index1){
                if(index != index1) {
                    send(thread[index1].fds, meesage, strlen(meesage), 0);
                }
            }
            pthread_mutex_unlock(&mutex);
        }
		else{
            ++tmp;
            pthread_mutex_lock(&mutex);
            for(int index1 = 0; index1 < client_count; ++index1) {
                if (strstr(buffer, thread[index1].name) != NULL) {
                    send(thread[index1].fds, tmp, strlen(tmp), 0);
                    printf("Message from %s sent to %s:\n", thread[index].name, thread[index1].name);
                    break;
                }
            }
            pthread_mutex_unlock(&mutex);
        }
    }
    return NULL;
}

int main() {
    printf("Server start working:\n");
    signal(SIGPIPE, handler);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        perror("Socket creation failed:");
        exit(EXIT_FAILURE);
    }
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(optval));
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (const void*)&optval, sizeof(optval));
    struct sockaddr_in server;
    memset((void*)&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(DEFAULT_PORT);
    server.sin_addr.s_addr = INADDR_ANY;
    if(bind(sockfd, (const struct sockaddr*)&server, sizeof(server)) < 0){
        perror("Socket binding failed:\n");
        exit(EXIT_FAILURE);
    }
    if(listen(sockfd, MAXBECKLOG) == -1){
        perror("Socket listening failed:\n");
        exit(EXIT_FAILURE);
    }
    while(1){
        if(client_count >= MAXCLIENTCOUNT){
            printf("Max client limit reached. New connections will be refused.\n");
            sleep(1);
            continue;
        }
        thread[client_count].fds = accept(sockfd, NULL, NULL);
        if(thread[client_count].fds == -1){
            printf("Accept failed:\n");
            continue;
        }
        char namebuf[MAXNAMESIZE];
        memset(namebuf, 0, sizeof(namebuf));
        int recv_retval = recv(thread[client_count].fds, namebuf, sizeof(namebuf) - 1, 0);
        if(recv_retval == -1){
            perror("Connection lost...");
            continue;
        }
        strtok(namebuf, "\n");
        strcpy(thread[client_count].name, namebuf);
        printf("Client %s connected to server:\n", thread[client_count].name);
        int *index = malloc(sizeof(int));
        *index = client_count;
        if (pthread_create(&thread[client_count].tid, NULL, ThreadFunction, index)) {
            perror("Thread creation failed:\n");
            exit(EXIT_FAILURE);
        }
        client_count++;
    }
    close(sockfd);
    return 0;
}

