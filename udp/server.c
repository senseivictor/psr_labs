#include "../utils.c"
#include <netdb.h>      // Definiții pentru operații de rețea
#include <netinet/in.h> // Structuri pentru adrese Internet
#include <stdbool.h>
#include <stdint.h>     // Pentru tipuri de date standard (uint16_t)
#include <stdio.h>      // Biblioteca standard pentru intrare/ieșire
#include <stdlib.h>     // Biblioteca pentru funcții utilitare (exit, atoi)
#include <sys/param.h>  // Definiții de parametri de sistem
#include <sys/socket.h> // Biblioteca principală pentru socket-uri
#include <sys/types.h>  // Definiții de tipuri de date pentru sistem
#include <unistd.h>     // Pentru funcțiile read, write, close

int main(int argc, char *argv[]) {
  struct sockaddr_in server_addr;
  struct sockaddr_in client_addr;
  ssize_t bytes_read; // 0-n or -1 in case of error
  action_t action_to_do = NULL_ACTION;
  char buf[BUFSIZ];

  if (argc != 2) { // Verifică prezența argumentului port
    fprintf(stdout, "usage: %s port\n", argv[0]);
    exit(-1);
  }

  char *port = argv[1];
  configServer(&server_addr, port);
  int sv_desc = openUDPSocket();
  bindSocketToPort(sv_desc, &server_addr);

  sendUDPPrompt(sv_desc, prompts.help, client_addr, sizeof(client_addr));

  while (action_to_do != CLOSE_CONNECTION) {

    bytes_read = readUDPMessage(sv_desc, buf, client_addr, sizeof(client_addr));

    if (strcmp(buf, "help\n") == 0)
      action_to_do = SHOW_HELP;
    if (strcmp(buf, "put\n") == 0)
      action_to_do = INPUT_TEXT;
    if (strcmp(buf, "quit\n") == 0 || bytes_read == 0)
      action_to_do = CLOSE_CONNECTION;
    if (strcmp(buf, ".\n") == 0)
      action_to_do = FINISH_TEXT_INPUT;

    switch (action_to_do) {
    case SHOW_HELP:
      sendUDPPrompt(sv_desc, prompts.help, client_addr, sizeof(client_addr));
      action_to_do = NULL_ACTION;
      break;
    case INPUT_TEXT:
      sendUDPPrompt(sv_desc, prompts.text, client_addr, sizeof(client_addr));
      action_to_do = PRINT_TEXT;
      break;
    case CLOSE_CONNECTION:
      printf("\nEnding connection ...\n");
      break;
    case FINISH_TEXT_INPUT:
      sendUDPPrompt(sv_desc, prompts.command, client_addr, sizeof(client_addr));
      action_to_do = NULL_ACTION;
      break;
    case PRINT_TEXT:
      printf("%s", buf);
      break;
    default:
      sendUDPPrompt(sv_desc, prompts.command, client_addr, sizeof(client_addr));
    }
  }
  close(sv_desc);
  return 0;
}