#include "parseRequest.c"
#include "utils.c"
#include <arpa/inet.h>
#include <errno.h>  // Necesar pentru detectarea tipului de eroare (ex: EACCES)
#include <signal.h> // Necesar pentru signal()
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

  signal(
      SIGCHLD,
      SIG_IGN); // Spune sistemului să curețe automat procesele copii terminate

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
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Pasul 1: Acceptăm conexiunea (doar părintele stă aici)
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0)
      continue;

    // Pasul 2: Creăm un proces nou
    pid_t pid = fork();

    if (pid < 0) {
      perror("Eroare la fork");
      close(client_fd);
    } else if (pid == 0) {
      // --- SUNTEM ÎN PROCESUL COPIL ---
      close(server_fd); // Copilul nu are nevoie de socket-ul de ascultare al
                        // părintelui

      // Logica de procesare (cea de la Lab 7)

      memset(buffer, 0, BUFFER_SIZE);
      int valread = read(client_fd, buffer, BUFFER_SIZE);

      if (valread > 0) {
        // Pasul 1: Parsarea cererii
        if (parseRequest(buffer, BUFFER_SIZE, fileName, 256) == -1) {
          // EROARE 400: Sintaxă incorectă
          printf("DEBUG: Eroare 400 - Sintaxa invalida\n");
          send_file(
              client_fd,
              "HTTP/1.1 400 BAD REQUEST\r\nContent-Type: text/html\r\n\r\n",
              "file400.html");
        } else {
          // Pasul 2: Încercăm să deschidem fișierul cerut
          FILE *requested_file = fopen(fileName, "r");

          if (requested_file != NULL) {
            fclose(requested_file); // Îl închidem pentru a folosi funcția
                                    // send_file

            // Determinăm tipul de conținut
            const char *contentType = get_content_type(fileName);

            // Construim un header dinamic
            char dynamicHeader[100];
            sprintf(dynamicHeader,
                    "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n", contentType);

            printf("[LOG %s]: Trimit fisierul %s de tip %s\n",
                   inet_ntoa(address.sin_addr), fileName, contentType);

            printf("[DEBUG] Procesul %d doarme 7 secunde pentru a simula o "
                   "sarcina grea...\n",
                   getpid());
            sleep(7);

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

      close(client_fd);
      exit(0); // Foarte important: procesul copil se termină aici!
    } else {
      // --- SUNTEM ÎN PROCESUL PĂRINTE ---
      // Părintele a delegat sarcina copilului, deci el închide copia sa de
      // client_fd
      close(client_fd);

      // Pasul 3: Curățarea proceselor "zombie" (opțional, dar recomandat)
      // Se poate folosi signal(SIGCHLD, SIG_IGN); înainte de while
    }
  }

  return 0;
}