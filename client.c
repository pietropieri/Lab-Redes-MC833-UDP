#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#define SIZE 1024
#define TCP_PORT 9090
#define UDP_PORT 8080

void write_file(int sockfd, struct sockaddr_in addr) {
    int n;
    char buffer[SIZE];
    FILE* fp = fopen("received.mp3", "wb");
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

void send_command_udp(int sockfd, struct sockaddr_in addr, const char* command) {
    sendto(sockfd, command, strlen(command) + 1, 0, (struct sockaddr*)&addr, sizeof(addr));
}

void request_tcp(int index) {
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
    
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
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

int main(void) {
    int server_sockfd;
    struct sockaddr_in server_addr;

    server_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_sockfd < 0) {
        perror("[ERROR] Socket error");
        exit(1);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(UDP_PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int continue_running = 1;
    char input[10];

    while (continue_running) {
        printf("Enter command (1: Download MP3, 2: Close server, 3: Get list item, 0: Exit client): ");
        fgets(input, 10, stdin);
        input[strcspn(input, "\n")] = 0;  // Remove newline character

        if (strcmp(input, "1") == 0) {
            printf("[REQUEST] Requesting MP3 file from server.\n");
            send_command_udp(server_sockfd, server_addr, "1");
            write_file(server_sockfd, server_addr);
        } else if (strcmp(input, "2") == 0) {
            printf("[REQUEST] Requesting to close the server.\n");
            send_command_udp(server_sockfd, server_addr, "2");
            continue_running = 0;  // Optionally exit the client after server shutdown
        } else if (strcmp(input, "3") == 0) {
            printf("Enter index (0-7): ");
            int index;
            scanf("%d", &index);  // Read the index from user
            getchar(); // consume newline character
            request_tcp(index);
        } else if (strcmp(input, "0") == 0) {
            printf("[CLOSING] Exiting client.\n");
            continue_running = 0;
        } else {
            printf("[ERROR] Invalid command.\n");
        }
    }

    close(server_sockfd);
    return 0;
}
