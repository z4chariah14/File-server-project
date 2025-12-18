#include <fcntl.h> 
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

enum {
  PORT = 8080,
  BACKLOG = 10,
  BUFFER_SIZE = 1024, // sticking with this buffer size because it minimizes context switches when using recv and not reserving a significant large portion of the RAM,
  //initially was confused after our meeting, and was going to alter the size but after checking on google this seems to be a "Goldilocks" number
  // I learned memory is allocated in powers of 2 so 1024 (2^10), would probably make the process faster
  PATH_BUFFER = 4096 // on a linux environment this is the path max, so even though I dont plan on using a file path that long, by matching the os limit
  //i can garauntee the program can hadle anything my os can.
}; // switched to using an enum like we talked about


// implementation drawn from practice problem 15
typedef struct {
    int socket_fd;
    int is_active; // 0 = Empty, 1 = Busy
} client_slot_t;


client_slot_t client_slots[BACKLOG]; 

pthread_mutex_t slot_lock = PTHREAD_MUTEX_INITIALIZER;


static int shutdown_pipe[2]; // cmake flag returned variable is non const and globally accessible, was going declare as const but that means we cant write to it,
// so declare as static and now only file_server.c has access to it.


void handle_sigint(int sig) {
  (void)sig;
  char msg = 'x';
  write(shutdown_pipe[1], &msg, 1);
}

void* client_handler(void* socket_desc) {
  client_slot_t* my_slot = (client_slot_t*)socket_desc;
  int sock = my_slot->socket_fd;

  pthread_detach(pthread_self());

  char buffer[BUFFER_SIZE] = {0};
  char filename[BUFFER_SIZE] = {0};

  ssize_t valread = recv(sock, filename, sizeof(filename), 0);
  if (valread <= 0) {
    close(sock);
    pthread_mutex_lock(&slot_lock);
    my_slot->is_active = 0;
    pthread_mutex_unlock(&slot_lock);
    return NULL;
  }

  filename[strcspn(filename, "\r\n")] = 0;

  char filepath[PATH_BUFFER];
  int path_length = snprintf(filepath, sizeof(filepath), "../test_files/%s", filename); 

  if (path_length < 0 || path_length >= (int)(sizeof(filepath))){
    (void)fprintf(stderr, "Error: Filename too long.\n");
    return NULL;
  }; // introduced to check for overflow cases, snprintf will truncate if buffer isn't big enough, (solving one of the cmake warning)
  // but if the truncated file name actually exist, we could end up reading the wrong file and not returnning an error

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
  pthread_mutex_lock(&slot_lock);
  my_slot->is_active = 0; 
  pthread_mutex_unlock(&slot_lock);
  return NULL;
}

//extracted code from while loop to create helper - cmake kept complaining about too if/for statements
void accept_new_connection(int server_socket_id, struct sockaddr_in* address, socklen_t* addrlen) {
    //Cmake warning: use accept4() because it allows SOCK_CLOEXEC. https://stackoverflow.com/questions/22304631/what-is-the-purpose-to-set-sock-cloexec-flag-with-accept4-same-as-o-cloexec 
    // from my understanding from the reddit post, if my server ever spawns/produces a child process (like a helper program)
    // the socket is automatically closed so client connection is not leaked to that process
    int client_socket = accept4(server_socket_id, (struct sockaddr*)address, 
                                addrlen, SOCK_CLOEXEC);
    if (client_socket < 0) {
        perror("Accept failed");
        return;
    }

    printf("Accepted new connection.\n");

    pthread_mutex_lock(&slot_lock); 
    int found_slot = -1;
    for (int i = 0; i < BACKLOG; i++) {
        if (client_slots[i].is_active == 0) {
            found_slot = i;
            client_slots[i].is_active = 1; 
            client_slots[i].socket_fd = client_socket;
            break;
        }
    }
    pthread_mutex_unlock(&slot_lock); 

    if (found_slot != -1) {
        pthread_t thread_id = 0;
        if (pthread_create(&thread_id, NULL, client_handler, &client_slots[found_slot]) != 0) {
            perror("Could not create thread");
            pthread_mutex_lock(&slot_lock); 
            client_slots[found_slot].is_active = 0;
            pthread_mutex_unlock(&slot_lock);
            close(client_socket);
        }
    } else {
        (void)fprintf(stderr, "Server full: Rejecting client.\n");
        close(client_socket);
    }
}

int main() {
  int server_socket_id = 0;
  struct sockaddr_in address;
  socklen_t addrlen = sizeof(address);

  // Cmake warning warned to use pipe2() instead of pipe(), by default when we run a new program on the same
  // server, it inherits every file descriptor so another executable could write to this pipe, pipe2() and a flag (O_CLOEXEC) could tell the OS if i run another program
  // close this pipe. AI and https://pubs.opengroup.org/onlinepubs/9799919799/functions/pipe.html helped me understand this

  if (pipe2(shutdown_pipe, O_CLOEXEC) < 0) { 
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

  printf("Server running (Ctrl+C to stop)\n");

  struct pollfd fds[2];

  fds[0].fd = server_socket_id;
  fds[0].events = POLLIN;

  fds[1].fd = shutdown_pipe[0];
  fds[1].events = POLLIN;

  while (1) {
    printf("Waiting for activity\n");

    int activity = poll(fds, 2, -1);

    if (activity < 0) {
      continue;
    }

    // Cmake warning use of unsigned integer with binary bitwise operator
    // after looking at https://man7.org/linux/man-pages/man2/poll.2.html for the data type of revents in the struct pollfd,
    // revents is a short (signed 2 byte int), POLLIN is a raw bundle of bits (google search, short top description), which is,
    // unsigned int, so we can cast revents to unsigned to fix.
    if ((unsigned)fds[1].revents & (unsigned)POLLIN) { 
      printf("\nReceived shutdown signal.\n");
      break;
    }

    if ((unsigned)fds[0].revents & (unsigned)POLLIN) { 
      accept_new_connection(server_socket_id, &address, &addrlen);
    }
  }

  close(server_socket_id);
  close(shutdown_pipe[0]);
  close(shutdown_pipe[1]);
  printf("Server shutdown complete.\n");

  return 0;
}
