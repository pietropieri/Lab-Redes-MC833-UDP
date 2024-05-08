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

  sendto(server_sockfd, "Hello", 6, 0, (struct sockaddr*)&server_addr, sizeof(server_addr)); // Send initial message to server to request file

  write_file(server_sockfd, server_addr);

  printf("[SUCCESS] Data transfer complete.\n");
  printf("[CLOSING] Disconnecting from the server.\n");

  close(server_sockfd);

  return 0;
}
