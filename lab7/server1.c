#include "parseRequest.c"
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT 2000
#define BUFFER_SIZE 1000

int main() {
  int server_fd, client_fd;
  struct sockaddr_in address;
  int opt = 1;
  int addrlen = sizeof(address);
  char buffer[BUFFER_SIZE];
  char fileName[256];

  // 1. Creare Socket
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("Eroare la crearea socket-ului");
    exit(EXIT_FAILURE);
  }

  // Setare opțiuni pentru a permite repornirea rapidă a serverului pe același
  // port
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  // 2. Bind
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("Eroare la bind");
    exit(EXIT_FAILURE);
  }

  // 3. Listen
  if (listen(server_fd, 3) < 0) {
    perror("Eroare la listen");
    exit(EXIT_FAILURE);
  }

  printf("Serverul web iterativ ruleaza pe portul %d...\n", PORT);

  while (1) {
    printf("\nAstept conexiuni...\n");

    // 4. Accept
    client_fd =
        accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
    if (client_fd < 0) {
      perror("Eroare la accept");
      continue;
    }

    // --- Pasul 1 din exercitiu: Recuperarea numelui fisierului ---
    memset(buffer, 0, BUFFER_SIZE);
    int valread = read(client_fd, buffer, BUFFER_SIZE);

    if (valread > 0) {
      // Folosim functia din parser.c
      if (parseRequest(buffer, BUFFER_SIZE, fileName, 256) == 0) {

        // --- Pasul 2: Trimiterea codului HTTP 200 ---
        char *header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
        write(client_fd, header, strlen(header));

        // --- Pasul 3: Citirea si trimiterea fisierului ---
        FILE *file = fopen(fileName, "r");
        if (file != NULL) {
          char file_content[1024];
          int bytes_read;
          while ((bytes_read =
                      fread(file_content, 1, sizeof(file_content), file)) > 0) {
            write(client_fd, file_content, bytes_read);
          }
          fclose(file);
        } else {
          printf("Eroare: Fisierul '%s' nu a putut fi deschis.\n", fileName);
          // Optional: trimite un mesaj de eroare 404 daca vrei sa fii profi
          char *not_found = "<html><body><h1>404 Not Found</h1></body></html>";
          write(client_fd, not_found, strlen(not_found));
        }
      } else {
        printf("Cerere HTTP invalida.\n");
      }
    }

    // Inchidem conexiunea cu clientul curent inainte de a trece la urmatorul
    // (iterativ)
    close(client_fd);
  }

  return 0;
}