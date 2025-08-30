#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>

#define DEFAULT_PORT 3381
#define MAXMESSAGESIZE 100

int sockfd;
struct sockaddr_in client;
pthread_t tid_r;
pthread_t tid_w;
volatile int running = 1;
volatile int flag = 0; 
void* ReadThread(void*);
void* WriteThread(void*);

void ConnectionLostHandler(int signum){
	if(flag != 0){
    printf("Connection lost\n Waiting connection...\n");
	}
	else{
		printf("Waiting connection:\n");
		}
    close(sockfd);
    sockfd = -1;
    while(running){
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("Socket creation failed");
            sleep(1);
            continue;
        }
        int optval = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        if(connect(sockfd, (struct sockaddr*)&client, sizeof(client)) < 0){
            close(sockfd);
            sockfd = -1; 
            sleep(1); 
        }
		else{
			if(flag != 0){
            	printf("Reconnected to server:\n");
			}
			else{
				printf("Connected to server:\n");
				++flag;
			}
            if (pthread_create(&tid_r, NULL, ReadThread, NULL)) {
                perror("Thread creation failed on read thread");
                exit(EXIT_FAILURE);
            }
            if (pthread_create(&tid_w, NULL, WriteThread, NULL)) {
                perror("Thread creation failed on write thread");
                exit(EXIT_FAILURE);
            }
            pthread_join(tid_r, NULL);
            pthread_join(tid_w, NULL);
            break;
        }
    }
return;
}

void ExitHandler(int signum){
    running = 0; 
    if(sockfd != -1){
        send(sockfd, "exit", strlen("exit"), 0);
    }
    printf("Exiting...\n");
    exit(EXIT_SUCCESS);
}

void* ReadThread(void* arg){
    char buffer[MAXMESSAGESIZE];
    while(running){
        memset(buffer, 0, sizeof(buffer));
        int recv_retval = recv(sockfd, buffer, sizeof(buffer), 0);
        if(recv_retval <= 0){
            printf("Read error or server disconnected. Raising signal to reconnect...\n");
            raise(SIGUSR1);
            break;
        }
		strtok(buffer,"\n");
        printf(" %s\n", buffer);
    }
    return NULL;
}

void* WriteThread(void* arg){
    char buffer[MAXMESSAGESIZE];
    while (running){
        memset(buffer, 0, sizeof(buffer));
        fgets(buffer, sizeof(buffer), stdin);
        if(strstr(buffer, "exit") != NULL){
            raise(SIGUSR2);
            break; 
        }
        if(send(sockfd, buffer, strlen(buffer), 0) <= 0){
            printf("Write error. Raising signal to reconnect...\n");
            raise(SIGUSR1);
            break; 
        }
    }
    return NULL;
}

int main(){
    signal(SIGINT, ExitHandler);
    signal(SIGTERM, ExitHandler);
	signal(SIGHUP,&ExitHandler);	
    signal(SIGUSR1, ConnectionLostHandler);
    signal(SIGUSR2, ExitHandler);

    sockfd = -1;
    client.sin_family = AF_INET;
    client.sin_port = htons(DEFAULT_PORT);
    client.sin_addr.s_addr = inet_addr("127.0.0.1");
    ConnectionLostHandler(0);
    return 0;
}
