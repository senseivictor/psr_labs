#include "parseRequest.c"
#include "utils.c"
#include <arpa/inet.h>
#include <errno.h> // Necesar pentru detectarea tipului de eroare (ex: EACCES)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT 2000
#define BUFFER_SIZE 1000

// Funcție utilitară pentru a trimite un fișier către client
void send_file(int client_socket, char *header, char *file_path) {
  write(client_socket, header, strlen(header));

  FILE *file = fopen(file_path, "r");
  if (file != NULL) {
    char buffer[1024];
    int bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
      write(client_socket, buffer, bytes_read);
    }
    fclose(file);
  } else {
    // Dacă nici fișierul de eroare nu poate fi deschis, trimitem un mesaj text
    // simplu
    char *fallback = "Error: Technical difficulties. Please try again later.";
    write(client_socket, fallback, strlen(fallback));
  }
}

int main() {
  int server_fd, client_fd;
  struct sockaddr_in address;
  int opt = 1;
  int addrlen = sizeof(address);
  char buffer[BUFFER_SIZE];
  char fileName[256];

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("Eroare Bind");
    exit(1);
  }

  listen(server_fd, 5);
  printf("Server Web pornit pe portul %d...\n", PORT);

  while (1) {
    client_fd =
        accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
    if (client_fd < 0)
      continue;

    memset(buffer, 0, BUFFER_SIZE);
    int valread = read(client_fd, buffer, BUFFER_SIZE);

    if (valread > 0) {
      // Pasul 1: Parsarea cererii
      if (parseRequest(buffer, BUFFER_SIZE, fileName, 256) == -1) {
        // EROARE 400: Sintaxă incorectă
        printf("DEBUG: Eroare 400 - Sintaxa invalida\n");
        send_file(client_fd,
                  "HTTP/1.1 400 BAD REQUEST\r\nContent-Type: text/html\r\n\r\n",
                  "file400.html");
      } else {
        // Pasul 2: Încercăm să deschidem fișierul cerut
        FILE *requested_file = fopen(fileName, "r");

        if (requested_file != NULL) {
          fclose(
              requested_file); // Îl închidem pentru a folosi funcția send_file

          // Determinăm tipul de conținut
          const char *contentType = get_content_type(fileName);

          // Construim un header dinamic
          char dynamicHeader[100];
          sprintf(dynamicHeader, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n",
                  contentType);

          printf("DEBUG: Trimit fisierul %s de tip %s\n", fileName,
                 contentType);

          send_file(client_fd, dynamicHeader, fileName);
        } else {
          // Pasul 3: Gestionarea erorilor de fișier folosind errno
          if (errno == EACCES) {
            // EROARE 500: Permisiuni insuficiente
            printf("DEBUG: Eroare 500 - Acces refuzat la %s\n", fileName);
            send_file(client_fd,
                      "HTTP/1.1 500 INTERNAL SERVER ERROR\r\nContent-Type: "
                      "text/html\r\n\r\n",
                      "file500.html");
          } else {
            // EROARE 404: Fișierul nu a fost găsit
            printf("DEBUG: Eroare 404 - Fisier negasit: %s\n", fileName);
            send_file(client_fd,
                      "HTTP/1.1 404 FILE NOT FOUND\r\nContent-Type: "
                      "text/html\r\n\r\n",
                      "file404.html");
          }
        }
      }
    }

    // Pasul 4: Închiderea conexiunii cu clientul conform protocolului HTTP/1.0
    close(client_fd);
  }

  return 0;
}