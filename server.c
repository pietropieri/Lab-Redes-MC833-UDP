#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SIZE 1024
#define TCP_PORT 9090
#define UDP_PORT 8080



// Definição da estrutura para armazenar informações da música
typedef struct {
    int id;
    char titulo[100];
    char interprete[100];
    char idioma[50];
    char tipo[50];
    char refrao[200];
    char ano_lancamento[100];
} Musica;

// replace by 8 objects of type Musica so we can list them later
char* list[8] = {"aaaa", "bbbb", "cccc", "dddd", "eeee", "ffff", "gggg", "hhhh"};

void send_file_data(FILE* fp, int sockfd, struct sockaddr_in addr) {
    int n;
    char buffer[SIZE];
    socklen_t addr_size = sizeof(addr);

    while (fread(buffer, 1, SIZE, fp) > 0) {
        sendto(sockfd, buffer, SIZE, 0, (struct sockaddr*)&addr, addr_size);
    }

    // Sending the 'END'
    strcpy(buffer, "END");
    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&addr, addr_size);

    fclose(fp);
}

void* handle_udp(void* arg) {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[SIZE];
    int recv_len;
    socklen_t addr_size;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(UDP_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("UDP server started...\n");

    while (1) {
        addr_size = sizeof(client_addr);
        recv_len = recvfrom(sockfd, buffer, SIZE, 0, (struct sockaddr*)&client_addr, &addr_size);
        buffer[recv_len] = '\0';

        if (strcmp(buffer, "1") == 0) {
            FILE* fp = fopen("server.mp3", "rb");
            if (fp == NULL) {
                perror("File open failed");
            } else {
                send_file_data(fp, sockfd, client_addr);
            }
        } else if (strcmp(buffer, "2") == 0) {
            printf("Shutdown command received. Exiting.\n");
            break;
        }
    }

    close(sockfd);
    return NULL;
}

void* handle_tcp(void* arg) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    char buffer[SIZE] = {0};
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(TCP_PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        int valread = read(new_socket, buffer, SIZE);
        int index = atoi(buffer);
        if (index >= 0 && index < 8) {
            send(new_socket, list[index], strlen(list[index]), 0);
        } else {
            send(new_socket, "Invalid index", strlen("Invalid index"), 0);
        }
        close(new_socket);
    }

    close(server_fd);
    return NULL;
}

int main() {
    pthread_t thread_id_tcp, thread_id_udp;

    if (pthread_create(&thread_id_tcp, NULL, handle_tcp, NULL) != 0) {
        printf("Failed to create TCP thread\n");
        return 1;
    }
    if (pthread_create(&thread_id_udp, NULL, handle_udp, NULL) != 0) {
        printf("Failed to create UDP thread\n");
        return 1;
    }

    pthread_join(thread_id_tcp, NULL);
    pthread_join(thread_id_udp, NULL);

    return 0;
}
