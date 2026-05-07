#include "../macros.h"
#include "../utils.c"
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

char buf[BUFSIZ];

int main(int argc, char *argv[]) {
  struct sockaddr_in sv_addr;
  struct hostent *host;
  ssize_t bytes_read_from_server;
  ssize_t n;

  if (argc != 3) {
    printf("Usage: %s hostname port\n", argv[0]);
    exit(-1);
  }

  int cl_desc = openTCPSocket();

  if ((host = gethostbyname(argv[1])) == NULL)
    ERROR("gethostbyname");

  char *port = argv[2];
  configClient(&sv_addr, host, port);

  if (connect(cl_desc, (struct sockaddr *)&sv_addr, sizeof(sv_addr)) < 0)
    ERROR("connect");

  while (strcmp(buf, "quit\n") != 0) {

    // atata timp cat serverul nu trimite promptul >>, asteapta raspunsul.
    // in general, clientul primeste microfonul doar cand serverul trimite >>.
    // serverul primeste microfonul doar cand a primit date.
    // fac update la buffer de fiecare data cand trimit ceva, de exemplu, am
    // scris "quit\n", bufferul va arata asa: ['q', 'u', 'i', 't', '\n', null,
    // null, null, ....] in momentul in care serverul trimite ceva, bufferul va
    // arata asa: ['>','>', 'q', 'u', 'i', 't', '\n', null, null, ....], adica
    // bufferul se rescrie de la inceput, de asta verific daca primele doua
    // caractere sunt >>, iar la urma adaug
    // \0, ca strlen() sa citeasca stringul doar pana la \0, care este caracter
    // default de terminare

    while (strlen(buf) < 2 ||
           !((buf[strlen(buf) - 2] == '>') && (buf[strlen(buf) - 1] == '>'))) {

      n = read(cl_desc, buf, sizeof(buf) - 1);

      if (n < 0)
        ERROR("read");

      if (n == 0)
        break;

      buf[n] = '\0';
      printf("%s", buf);
    }

    // daca promptul este cel de "introduceti text", trimitem la server
    // fiecare linie, pana cand gasim "."
    // * aici clientul nu asculta serverul deloc, doar trimite date
    if (strcmp(buf, prompts.text) == 0) {
      while (buf[0] != '.') {
        if (fgets(buf, sizeof(buf), stdin) != NULL) {
          if (write(cl_desc, buf, strlen(buf)) < 0)
            ERROR("write");
        }
      }
    } else {
      if (fgets(buf, sizeof(buf), stdin) != NULL) {
        if (write(cl_desc, buf, strlen(buf)) < 0)
          ERROR("write");
      }
    }
  }

  close(cl_desc);
  return 0;
}