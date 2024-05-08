#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SIZE 1024

void send_file_data(FILE* fp, int sockfd, struct sockaddr_in addr) {
  int n;
  char buffer[SIZE];
  socklen_t addr_size = sizeof(addr);

  while (fread(buffer, 1, SIZE, fp) > 0) {
    printf("[SENDING] Data of %d bytes\n", strlen(buffer));
    n = sendto(sockfd, buffer, SIZE, 0, (struct sockaddr*)&addr, addr_size);
    if (n == -1) {
      perror("[ERROR] Sending data to the client.");
      exit(1);
    }
  }

  // Sending the 'END'
  strcpy(buffer, "END");
  sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&addr, addr_size);

  fclose(fp);
}

int main() {
  char* ip = "127.0.0.1";
  const int port = 8080;
  int server_sockfd;
  struct sockaddr_in server_addr, client_addr;
  char buffer[SIZE];
  int e;

  server_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (server_sockfd < 0) {
    perror("[ERROR] Socket error");
    exit(1);
  }
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = inet_addr(ip);

  e = bind(server_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
  if (e < 0) {
    perror("[ERROR] Bind error");
    exit(1);
  }

  printf("[STARTING] UDP File Server started. Waiting for client request...\n");

  socklen_t addr_size = sizeof(client_addr);
  recvfrom(server_sockfd, buffer, SIZE, 0, (struct sockaddr*)&client_addr, &addr_size); // Wait for any message from client

  FILE* fp = fopen("server.mp3", "rb");
  if (fp == NULL) {
    perror("[ERROR] File not found");
    exit(1);
  }

  send_file_data(fp, server_sockfd, client_addr);

  printf("[SUCCESS] Data transfer complete.\n");
  printf("[CLOSING] Closing the server.\n");

  close(server_sockfd);

  return 0;
}
