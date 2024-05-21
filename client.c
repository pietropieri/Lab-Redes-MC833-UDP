#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SIZE 1024
#define TCP_PORT 9090
#define UDP_PORT 8080


// essa funcao cria um arquivo com a musica que foi recebida
void write_file(int sockfd, struct sockaddr_in addr, char id) {
    int n;
    char buffer[SIZE];
    char filename[50];

    // nome do arquivo
    sprintf(filename, "received-%c.mp3", id);

    FILE* fp = fopen(filename, "wb");
    if (fp == NULL) {
        perror("[ERROR] File opening");
        exit(1);
    }

    socklen_t addr_size;
    while (1) {
        addr_size = sizeof(addr);
        n = recvfrom(sockfd, buffer, SIZE, 0, (struct sockaddr*)&addr, &addr_size);
        if (strcmp(buffer, "END") == 0) {
            break;
        }
        fwrite(buffer, 1, n, fp);
        bzero(buffer, SIZE);
    }

    fclose(fp);
}

// essa funcao envia um comando udp para o servidor
void send_command_udp(int sockfd, struct sockaddr_in addr, const char* command) {
    sendto(sockfd, command, strlen(command) + 1, 0, (struct sockaddr*)&addr, sizeof(addr));
}

// essa funcao envia um request tcp para o servidor
void request_tcp(int index, const char* server_ip) {
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    char index_str[10];
    sprintf(index_str, "%d", index);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(TCP_PORT);
    
    // configuracao do ip
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        close(sock);
        return;
    }
  
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        close(sock);
        return;
    }

    send(sock, index_str, strlen(index_str), 0);
    valread = read(sock, buffer, 1024);
    printf("Received: %s\n", buffer);
    
    close(sock);
}

// funcao main responsavel pelo funcionamento do client
int main(void) {
    int server_sockfd;
    struct sockaddr_in server_addr;

    server_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_sockfd < 0) {
        perror("[ERROR] Socket error");
        exit(1);
    }

    // entrada do IP do server
    char server_ip[20];
    printf("Enter server IP address: ");
    fgets(server_ip, sizeof(server_ip), stdin);
    server_ip[strcspn(server_ip, "\n")] = 0;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(UDP_PORT);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    int continue_running = 1;
    char input[10];

    // loop de funcionamento do client
    while (continue_running) {
        printf("Enter command\n1: Download MP3;\n3: List music by ID;\n8: List all musics;\n0: Exit client:\n");
        fgets(input, 10, stdin);
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "1") == 0) {
            send_command_udp(server_sockfd, server_addr, "1");
            printf("Enter id (0-7): ");
            char id;
            scanf("%s", &id);
            getchar(); 
            printf("[REQUEST] Requesting MP3 file from server.\n");
            send_command_udp(server_sockfd, server_addr, &id);
            write_file(server_sockfd, server_addr, id);
        } else if (strcmp(input, "3") == 0) {
            printf("Enter ID (0-7): ");
            int index;
            scanf("%d", &index);
            getchar();
            request_tcp(index, server_ip);
        } else if (strcmp(input, "0") == 0) {
            printf("[CLOSING] Exiting client.\n");
            continue_running = 0;
        } else if (strcmp(input, "8") == 0)  {
            request_tcp(8, server_ip);
        } else {
            printf("[ERROR] Invalid command.\n");
        }
    }

    close(server_sockfd);
    return 0;
}
