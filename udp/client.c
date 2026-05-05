#include "../macros.h"
#include "../utils.c"
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

char buf[BUFSIZ];

int main(int argc, char *argv[]) {
  struct sockaddr_in sv_addr;
  struct hostent *host;
  ssize_t n;

  if (argc != 3) {
    printf("Usage: %s hostname port\n", argv[0]);
    exit(-1);
  }

  // 1. Deschidem un socket de tip SOCK_DGRAM (UDP)
  int cl_desc = openUDPSocket();

  if ((host = gethostbyname(argv[1])) == NULL)
    ERROR("gethostbyname");

  char *port = argv[2];
  configClient(&sv_addr, host, port);

  // 2. Connect pe UDP nu face "handshake", doar fixează adresa serverului
  // în kernel, permițându-ne să folosim read() și write().
  if (connect(cl_desc, (struct sockaddr *)&sv_addr, sizeof(sv_addr)) < 0)
    ERROR("connect");

  // Inițializăm bufferul pentru a intra în buclă
  memset(buf, 0, sizeof(buf));

  while (strcmp(buf, "quit\n") != 0) {

    // Curățăm buffer-ul pentru a începe primirea unui nou răspuns
    memset(buf, 0, sizeof(buf));

    // --- SECȚIUNEA RECEPȚIE (Așteptăm microfonul de la server) ---
    // Citim pachete până când găsim prompt-ul ">>"
    while (strstr(buf, ">>") == NULL) {
      char temp[BUFSIZ];
      memset(temp, 0, BUFSIZ);

      n = read(cl_desc, temp, sizeof(temp) - 1);
      if (n < 0)
        ERROR("read");
      if (n == 0)
        break; // Serverul a închis (puțin probabil în UDP)

      temp[n] = '\0';
      printf("%s", temp);
      fflush(stdout);

      // Acumulăm în buf pentru a verifica dacă a venit ">>"
      strncat(buf, temp, sizeof(buf) - strlen(buf) - 1);
    }

    // --- SECȚIUNEA TRANSMISIE (Avem microfonul) ---

    // Cazul 1: Serverul ne-a cerut text multiline
    if (strcmp(buf, prompts.text) == 0) {
      // Resetăm buf[0] pentru a intra în bucla de scriere
      buf[0] = '\0';
      while (buf[0] != '.') {
        if (fgets(buf, sizeof(buf), stdin) != NULL) {
          if (write(cl_desc, buf, strlen(buf)) < 0)
            ERROR("write");
        }
      }
    }
    // Cazul 2: Comandă normală (o singură linie)
    else {
      if (fgets(buf, sizeof(buf), stdin) != NULL) {
        if (write(cl_desc, buf, strlen(buf)) < 0)
          ERROR("write");
      }
    }
  }

  printf("Clientul se închide...\n");
  close(cl_desc);
  return 0;
}