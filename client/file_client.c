#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


// used enum instead of define
enum{
  PORT = 8080,
  BUFFER_SIZE = 1024,
  PATH_LEN = 4096
};

// Like we talked about the possible errors that comes with using scanf. This function should get rid every character 
// currently sitting in stdin until it finds a newline
void clear_input_buffer() {
    int get_rid = 0;
    while ((get_rid = getchar()) != '\n' && get_rid != EOF) {}
}

int main() {
  int sock = 0;
  struct sockaddr_in serv_addr;
  char buffer[BUFFER_SIZE] = {0};
  char filename[BUFFER_SIZE];

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("\n Socket creation error \n");
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);

  if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
    printf("\nInvalid address / Address not supported \n");
    return -1;
  }

  if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    printf("\nConnection Failed \n");
    return -1;
  }

  printf("Connected to server!\n");
  printf("Enter filename to download: ");
  if (scanf("%99s", filename) != 1){
    (void)fprintf(stderr, "Error reading input\n");
        close(sock);
        return -1;
    } // value from scanf should be used (cmake warning) so I just implemented a simple check function

  clear_input_buffer();

  send(sock, filename, strlen(filename), 0);
  printf("Request sent for '%s'. Waiting for data...\n", filename);

  char output_path[PATH_LEN];

  int needed = snprintf(output_path, sizeof(output_path), "../received_files/%s", filename);
  // dealing with cmake warning, value return by this function should be used
  
  if (needed < 0 || needed >= (int)sizeof(output_path)) {
        (void)fprintf(stderr, "Error: Filename too long\n");
        close(sock);
        return -1;
    }

  printf("Saving file to: %s\n", output_path);

  FILE* filepath = fopen(output_path, "wbe");
  if (filepath == NULL) {
    perror(
        "Failed to open file for writing ");
    return -1;
  }

  ssize_t bytes_received = 0; //read returns ssize_t, if read returned a huge number, when put into int it might chop off half the bits and 
  // the remaining might be a negative number, our while loop would fail even though it was a success.
  while ((bytes_received = read(sock, buffer, BUFFER_SIZE)) > 0) { // read returns ssize_t, long int
    (void)fwrite(buffer, 1, (size_t)bytes_received, filepath); // fwrite expects size_t, read needs ssize_t to return -1, even though we wouldn't get
    // to the line if read return -1, to silence cmake warning I still casted it. 
  }

  printf("File saved as '%s'\n", output_path);

  (void)fclose(filepath);
  close(sock);
  return 0;
}
