#include "macros.h"
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef enum {
  NULL_ACTION,
  SHOW_HELP,
  INPUT_TEXT,
  CLOSE_CONNECTION,
  FINISH_TEXT_INPUT,
  PRINT_TEXT,
} action_t;

struct {
  const char *help;
  const char *text;
  const char *command;
} prompts = {
    .help = "Available commands: \n\tput - to send a text to server \n\thelp - "
            "to view this help\n\tquit - to close the connection\nEnter a "
            "command >>",
    .text = "Enter the text terminating it with '.' in a new line.\n>>",
    .command = "Enter a command >>"};

int configServer(struct sockaddr_in *sv_addr, char *port) {
  sv_addr->sin_family = AF_INET;         // Setează familia IPv4
  sv_addr->sin_addr.s_addr = INADDR_ANY; // Ascultă pe orice interfață
  sv_addr->sin_port =
      htons((uint16_t)atoi(port)); // --- MODIFICARE: uint16_t pentru port ---
  return 0;
}

int configClient(struct sockaddr_in *sv_addr, struct hostent *host,
                 char *port) {

  //"copy-paste" la nivel de memorie, transferând adresa IP a serverului (găsită
  // în urma rezolvării DNS) în structura de date pe care o va folosi socket-ul
  // pentru a se conecta.
  memcpy(&sv_addr->sin_addr, host->h_addr_list[0], (size_t)host->h_length);
  sv_addr->sin_family = AF_INET;
  // htons convertește ordinea octeților (byte order) de la arhitectura
  // procesorului nostru (Host) la ordinea cerută de protocoalele de internet
  // (Network).
  sv_addr->sin_port = htons((uint16_t)atoi(port));
  return 0;
}

int openTCPSocket() {
  int descriptor = socket(AF_INET, SOCK_STREAM, 0);
  if (descriptor < 0)
    ERROR("socket");
  return descriptor;
}

int openUDPSocket() {
  int descriptor = socket(AF_INET, SOCK_DGRAM, 0);
  if (descriptor < 0)
    ERROR("socket");
  return descriptor;
}
int bindSocketToPort(int sv_desc, struct sockaddr_in *sv_addr) {
  int bind_result = bind(sv_desc, (struct sockaddr *)sv_addr, sizeof(*sv_addr));
  if (bind_result == -1)
    ERROR("bind");
  return bind_result;
}

int acceptClientConnection(int sv_desc, struct sockaddr_in *cl_addr) {
  socklen_t cl_addr_length = sizeof(*cl_addr);
  int cl_desc = accept(sv_desc, (struct sockaddr *)cl_addr, &cl_addr_length);
  if (cl_desc < 0)
    ERROR("accept");
  return cl_desc;
}

void sendTCPPrompt(int cl_desc, const char *prompt) {
  write(cl_desc, prompt, strlen(prompt));
}
void sendUDPPrompt(int sv_desc, const char *prompt, struct sockaddr_in cl_addr,
                   socklen_t cl_len) {
  sendto(sv_desc, prompt, strlen(prompt), 0, (struct sockaddr *)&cl_addr,
         cl_len);
}
int readTCPMessage(int cl_desc, char *buf) {
  ssize_t bytesRead = read(cl_desc, buf, sizeof(buf) - 1);
  if (bytesRead < 0)
    ERROR("Reading stream message\n");

  buf[bytesRead] = '\0';
  return bytesRead;
}

int readUDPMessage(int cl_desc, char *buf, struct sockaddr_in cl_addr,
                   socklen_t cl_len) {
  ssize_t bytesRead = recvfrom(cl_desc, buf, sizeof(buf) - 1, 0,
                               (struct sockaddr *)&cl_addr, &cl_len);
  if (bytesRead < 0)
    ERROR("Reading stream message\n");

  buf[bytesRead] = '\0';
  return bytesRead;
}
