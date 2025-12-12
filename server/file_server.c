#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BACKLOG 10
#define BUFFER_SIZE 1024
#define PATH_BUFFER 1100

int shutdown_pipe[2];

void handle_sigint(int sig) {
  (void)sig;
  char msg = 'x';
  write(shutdown_pipe[1], &msg, 1);
}

void* client_handler(void* socket_desc) {
  int sock = *(int*)socket_desc;
  free(socket_desc);

  pthread_detach(pthread_self());

  char buffer[BUFFER_SIZE] = {0};
  char filename[BUFFER_SIZE] = {0};

  ssize_t valread = recv(sock, filename, sizeof(buffer), 0);
  if (valread <= 0) {
    close(sock);
    return NULL;
  }

  filename[strcspn(filename, "\r\n")] = 0;

  char filepath[PATH_BUFFER];
  (void)snprintf(filepath, sizeof(filepath), "../test_files/%s", filename); 

  printf("[Thread %lu] Client wants: %s\n", pthread_self(), filepath);

  FILE* file = fopen(filepath, "rbe");
  if (file == NULL) {
    char* error = "ERROR: File not found\n";
    send(sock, error, strlen(error), 0);
  } else {
    size_t bytes_read = 0;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
      send(sock, buffer, bytes_read, 0);
    }
    (void)fclose(file);
  }

  printf("[Thread %lu] Done. Closing socket.\n", pthread_self());
  close(sock);
  return NULL;
}

int main() {
  int server_socket_id = 0;
  int client_socket = 0;
  struct sockaddr_in address;
  int addrlen = sizeof(address);

  if (pipe(shutdown_pipe) < 0) { 
    perror("Pipe failed");
    return EXIT_FAILURE; 
  }

  (void)signal(SIGINT, handle_sigint);

  if ((server_socket_id = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("Socket failed");
    return EXIT_FAILURE;
  }

  int value_enab = 1;
  if (setsockopt(server_socket_id, SOL_SOCKET, SO_REUSEADDR, &value_enab,
                 sizeof(value_enab))) {
    perror("setsockopt");
    return EXIT_FAILURE; 
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  if (bind(server_socket_id, (struct sockaddr*)&address, sizeof(address)) < 0) {
    perror("Bind failed");
    return EXIT_FAILURE; 
  }

  if (listen(server_socket_id, BACKLOG) < 0) {
    perror("Listen failed");
    return EXIT_FAILURE; 
  }

  printf("Server running (Ctrl+C to stop)...\n");

  struct pollfd fds[2];

  fds[0].fd = server_socket_id;
  fds[0].events = POLLIN;

  fds[1].fd = shutdown_pipe[0];
  fds[1].events = POLLIN;

  while (1) {
    printf("Waiting for activity...\n");

    int activity = poll(fds, 2, -1);

    if (activity < 0) {
      continue;
    }


    if (fds[1].revents & POLLIN) { 
      printf("\nReceived shutdown signal! Cleaning up...\n");
      break;
    }

    if (fds[0].revents & POLLIN) { 
      if ((client_socket = accept(server_socket_id, (struct sockaddr*)&address, 
                                  (socklen_t*)&addrlen)) < 0) {
        perror("Accept failed");
        continue;
      }

      printf("Accepted new connection.\n");

      pthread_t thread_id = 0; 
      int* new_sock = malloc(sizeof(int));
      *new_sock = client_socket;

      if (pthread_create(&thread_id, NULL, client_handler, (void*)new_sock) != 0) {
        perror("Could not create thread");
        free(new_sock);
        close(client_socket);
      }
    }
  }

  close(server_socket_id);
  close(shutdown_pipe[0]);
  close(shutdown_pipe[1]);
  printf("Server shutdown complete. Bye!\n");

  return 0;
}