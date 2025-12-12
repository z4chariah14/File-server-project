#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024

int main() {
  int sock = 0;
  struct sockaddr_in serv_addr;
  char buffer[BUFFER_SIZE] = {0};
  char filename[100];

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("\n Socket creation error \n");
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);

  if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
    printf("\nInvalid address / Address not supported \n");
    return -1;
  }

  if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    printf("\nConnection Failed \n");
    return -1;
  }

  printf("Connected to server!\n");
  printf("Enter filename to download: ");
  scanf("%99s", filename);

  send(sock, filename, strlen(filename), 0);
  printf("Request sent for '%s'. Waiting for data...\n", filename);

  char output_path[200];

  snprintf(output_path, sizeof(output_path), "../received_files/%s", filename);

  printf("Saving file to: %s\n", output_path);

  FILE* fp = fopen(output_path, "wb");
  if (fp == NULL) {
    perror(
        "Failed to open file for writing ");
    return -1;
  }

  int bytes_received;
  while ((bytes_received = read(sock, buffer, BUFFER_SIZE)) > 0) {
    fwrite(buffer, 1, bytes_received, fp);
  }

  printf("File saved as '%s'\n", output_path);

  fclose(fp);
  close(sock);
  return 0;
}