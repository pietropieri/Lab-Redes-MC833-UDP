#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#define SIZE 1024

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

void send_command(int sockfd, struct sockaddr_in addr, const char* command) {
    sendto(sockfd, command, strlen(command) + 1, 0, (struct sockaddr*)&addr, sizeof(addr)); // +1 to include the null terminator
}

int main(void) {
    char *ip = "127.0.0.1";
    const int port = 8080;
    int server_sockfd;
    struct sockaddr_in server_addr;

    server_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_sockfd < 0) {
        perror("[ERROR] Socket error");
        exit(1);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    int continue_running = 1;
    char input[10];

    while (continue_running) {
        printf("Enter command (1: Download MP3, 2: Close server, 0: Exit client): ");
        fgets(input, 10, stdin);
        input[strcspn(input, "\n")] = 0;  // Remove newline character

        if (strcmp(input, "1") == 0) {
            printf("[REQUEST] Requesting MP3 file from server.\n");
            send_command(server_sockfd, server_addr, "1");
            write_file(server_sockfd, server_addr);
        } else if (strcmp(input, "2") == 0) {
            printf("[REQUEST] Requesting to close the server.\n");
            send_command(server_sockfd, server_addr, "2");
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
