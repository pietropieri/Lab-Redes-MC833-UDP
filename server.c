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

// Lista de músicas
Musica list[8] = {
    {0, "Mamacita", "Mike Leite", "Inglês", "Instrumental", "N/A", "2022"},
    {1, "Baião de Dois", "Luiz Gonzaga", "Português", "Forró", "N/A", "1946"},
    {2, "Chorinho Típico", "Ernesto Nazareth", "Português", "Choro", "N/A", "1906"},
    {3, "Assanhado", "Jacob do Bandolim", "Português", "Choro", "N/A", "1966"},
    {4, "Everyday", "Jason Farnham", "Inglês", "Instrumental", "N/A", "2013"},
    {5, "Cylinder Five", "Chris Zabriskie", "Inglês", "Instrumental", "N/A", "2014"},
    {6, "AEROHEAD", "Haven", "Inglês", "Instrumental", "N/A", "2014"},
    {7, "Dreams", "Joakim Karud", "Inglês", "Instrumental", "N/A", "2017"}
};

// funcao de mandar a musica para o client
void send_file_data(FILE* fp, int sockfd, struct sockaddr_in addr) {
    int n;
    char buffer[SIZE];
    socklen_t addr_size = sizeof(addr);

    while (fread(buffer, 1, SIZE, fp) > 0) {
        sendto(sockfd, buffer, SIZE, 0, (struct sockaddr*)&addr, addr_size);
    }

    strcpy(buffer, "END");
    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&addr, addr_size);

    fclose(fp);
}

// funcao responsavel pelo funcinamento UDP do servidor
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

    printf("UDP/TCP server started...\n");

    // loop responsavel por gerir o lado UDP do servidor
    while (1) {
        addr_size = sizeof(client_addr);
        recv_len = recvfrom(sockfd, buffer, SIZE, 0, (struct sockaddr*)&client_addr, &addr_size);
        buffer[recv_len] = '\0';

        if (strcmp(buffer, "1") == 0) {
            recv_len = recvfrom(sockfd, buffer, SIZE, 0, (struct sockaddr*)&client_addr, &addr_size);
            buffer[recv_len] = '\0';
            int id = atoi(buffer);

            char filename[20];
            snprintf(filename, sizeof(filename), "%d.mp3", id);

            FILE* fp = fopen(filename, "rb");
            if (fp == NULL) {
                perror("File open failed");
                strcpy(buffer, "File not found");
                sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&client_addr, addr_size);
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


// funcao responsavel por gerir o funcionamento TCP do servidor
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

    // loop responsavel pelo lado TCP do servidor
    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        int valread = read(new_socket, buffer, SIZE);
        buffer[valread] = '\0';
        int index = atoi(buffer);
        if (index >= 0 && index < 8) {
            char response[SIZE];
            snprintf(response, SIZE, "ID: %d\nTítulo: %s\nIntérprete: %s\nIdioma: %s\nTipo: %s\nRefrão: %s\nAno de Lançamento: %s\n",
                     list[index].id, list[index].titulo, list[index].interprete, list[index].idioma,
                     list[index].tipo, list[index].refrao, list[index].ano_lancamento);
            send(new_socket, response, strlen(response), 0);
        } else if (index == 8) {
            char response[SIZE * 8] = {0};
            for (int i = 0; i < 8; i++) {
                char song_info[SIZE];
                snprintf(song_info, SIZE, "ID: %d\nTítulo: %s\nIntérprete: %s\nIdioma: %s\nTipo: %s\nRefrão: %s\nAno de Lançamento: %s\n\n",
                         list[i].id, list[i].titulo, list[i].interprete, list[i].idioma,
                         list[i].tipo, list[i].refrao, list[i].ano_lancamento);
                strcat(response, song_info);
            }
            send(new_socket, response, strlen(response), 0);
        } else {
            send(new_socket, "Invalid index", strlen("Invalid index"), 0);
        }
        close(new_socket);
    }

    close(server_fd);
    return NULL;
}

// funcao responsavel pelo funcionamento do servidor
int main() {
    pthread_t thread_id_tcp, thread_id_udp;

    // criacao da thread TCP
    if (pthread_create(&thread_id_tcp, NULL, handle_tcp, NULL) != 0) {
        printf("Failed to create TCP thread\n");
        return 1;
    }

    // criacao da thread UDP
    if (pthread_create(&thread_id_udp, NULL, handle_udp, NULL) != 0) {
        printf("Failed to create UDP thread\n");
        return 1;
    }

    pthread_join(thread_id_tcp, NULL);
    pthread_join(thread_id_udp, NULL);

    return 0;
}
