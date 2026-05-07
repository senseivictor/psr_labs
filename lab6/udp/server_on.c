#include "../utils.c"
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  struct sockaddr_in server_addr;
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr); // FOARTE IMPORTANT pentru UDP
  ssize_t bytes_read;
  action_t action_to_do = NULL_ACTION;
  char buf[BUFSIZ];

  if (argc != 2) {
    fprintf(stdout, "usage: %s port\n", argv[0]);
    exit(-1);
  }

  char *port = argv[1];
  configServer(&server_addr, port);
  int sv_desc = openUDPSocket();
  bindSocketToPort(sv_desc, &server_addr);
  // NOTĂ: listen() și accept() NU EXISTĂ în UDP.

  while (1) {
    // 3. Primim un pachet și aflăm simultan cine l-a trimis (client_addr)
    memset(buf, 0, BUFSIZ);
    bytes_read = recvfrom(sv_desc, buf, BUFSIZ - 1, 0,
                          (struct sockaddr *)&client_addr, &client_len);
    if (bytes_read < 0)
      continue;
    buf[bytes_read] = '\0';

    if (strcmp(buf, "help\n") == 0)
      action_to_do = SHOW_HELP;
    else if (strcmp(buf, "put\n") == 0)
      action_to_do = INPUT_TEXT;
    else if (strcmp(buf, ".\n") == 0)
      action_to_do = FINISH_TEXT_INPUT;
    else if (strcmp(buf, "quit\n") == 0)
      action_to_do = NULL_ACTION; // În UDP "închidem" doar logic sesiunea
    else if (action_to_do != PRINT_TEXT)
      action_to_do = NULL_ACTION;

    switch (action_to_do) {
    case SHOW_HELP:
      sendto(sv_desc, prompts.help, strlen(prompts.help), 0,
             (struct sockaddr *)&client_addr, client_len);
      action_to_do = NULL_ACTION;
      break;
    case INPUT_TEXT:
      sendto(sv_desc, prompts.text, strlen(prompts.text), 0,
             (struct sockaddr *)&client_addr, client_len);
      action_to_do = PRINT_TEXT; // Trecem în modul de primire text
      break;
    case FINISH_TEXT_INPUT:
      sendto(sv_desc, prompts.command, strlen(prompts.command), 0,
             (struct sockaddr *)&client_addr, client_len);
      action_to_do = NULL_ACTION;
      break;
    case PRINT_TEXT:
      printf("%s", buf);
      break;
    default:
      sendto(sv_desc, prompts.command, strlen(prompts.command), 0,
             (struct sockaddr *)&client_addr, client_len);
    }
  }

  close(sv_desc);
  return 0;
}