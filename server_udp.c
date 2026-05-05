#include <stdio.h>      // Biblioteca standard pentru intrare/ieșire
#include <stdlib.h>     // Biblioteca pentru funcții utilitare (exit, atoi)
#include <string.h>     // Biblioteca pentru manipularea șirurilor de caractere
#include <sys/types.h>  // Definiții de tipuri de date pentru sistem
#include <sys/socket.h> // Biblioteca principală pentru socket-uri
#include <netinet/in.h> // Structuri pentru adrese Internet
#include <netdb.h>      // Definiții pentru operații de rețea
#include <sys/param.h>  // Definiții de parametri de sistem
#include <errno.h>      // Pentru gestionarea erorilor
#include <unistd.h>     // Necesară pentru funcțiile read, write și close

#define ERROR(s)                   \
  {                                \
    fprintf(stderr, "%d-", errno); \
    perror(s);                     \
    return (-1);                   \
  } // Macro pentru erori

int main(int argc, char *argv[]) // Definit ca int pentru standardele moderne
{
  struct sockaddr_in sa;     // Structură pentru adresa serverului
  struct sockaddr_in caller; // Structură pentru adresa clientului
  int sd;                    // Descriptor socket (în UDP avem unul singur pentru toți)
  int length;                // Lungimea structurii adresei
  int retval;                // Rezultatul citirii/scrierii
  int action;                // Variabilă de stare pentru logică
  char buf[BUFSIZ];          // Buffer pentru date

  // Mesaje trimise către client
  char *prompt[] = {">>",
                    "Available commands: \n\tput - to send a text to server \thelp - to view this help\n\tquit - to close the connection\n>>",
                    "Enter the text terminating it with '.' in a new line.\n>>",
                    "Enter a command\n>>"};

  if (argc != 2)
  { // Verifică dacă portul a fost dat ca argument
    fprintf(stdout, "usage: %s port\n", argv[0]);
    exit(-1);
  }

  sa.sin_family = AF_INET;                      // Familia de protocoale IPv4
  sa.sin_addr.s_addr = INADDR_ANY;              // Ascultă pe toate interfețele disponibile
  sa.sin_port = htons((uint16_t)atoi(argv[1])); // Portul convertit în format de rețea

  // --- MODIFICARE UDP: folosim SOCK_DGRAM în loc de SOCK_STREAM ---
  if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    ERROR("socket");

  if (bind(sd, (struct sockaddr *)&sa, sizeof sa) < 0) // Asociere socket cu portul
    ERROR("bind");

  // NOTĂ: În UDP NU există listen() și NU există accept()!

  printf("Serverul UDP asteapta pachete pe portul %s...\n", argv[1]);
  action = -1; // Inițializăm starea

  while (1)
  {
    length = sizeof(caller);

    // --- MODIFICARE UDP: folosim recvfrom pentru a primi date și adresa expeditorului ---
    if ((retval = recvfrom(sd, buf, sizeof(buf), 0, (struct sockaddr *)&caller, &length)) <= 0)
    {
      continue;
    }
    buf[retval] = '\0'; // Terminăm șirul citit

    // Interpretarea comenzilor primite
    if (strcmp(buf, "help\n") == 0)
      action = 0;
    else if (strcmp(buf, "put\n") == 0)
      action = 1;
    else if (strcmp(buf, "quit\n") == 0)
      action = 2;
    else if (strcmp(buf, ".\n") == 0)
      action = 3;

    // --- MODIFICARE UDP: folosim sendto în loc de write pentru a răspunde clientului ---
    switch (action)
    {
    case 0: // Comanda Help
      sendto(sd, prompt[1], strlen(prompt[1]), 0, (struct sockaddr *)&caller, length);
      action = -1;
      break;
    case 1: // Comanda Put
      sendto(sd, prompt[2], strlen(prompt[2]), 0, (struct sockaddr *)&caller, length);
      action = 4; // Trecem în modul de afișare text
      break;
    case 2: // Comanda Quit
      printf("Clientul a trimis 'quit'.\n");
      char *bye = "Deconectat din modul UDP.\n";
      sendto(sd, bye, strlen(bye), 0, (struct sockaddr *)&caller, length);
      action = -1;
      break;
    case 3: // Terminare bloc text (punctul)
      sendto(sd, prompt[3], strlen(prompt[3]), 0, (struct sockaddr *)&caller, length);
      action = -1;
      break;
    case 4: // Afișăm în terminalul serverului ce scrie clientul
      printf("%s", buf);
      // Trimitem promptul de confirmare înapoi
      sendto(sd, prompt[0], strlen(prompt[0]), 0, (struct sockaddr *)&caller, length);
      break;
    default: // Orice altceva (comandă necunoscută)
      sendto(sd, prompt[3], strlen(prompt[3]), 0, (struct sockaddr *)&caller, length);
    }
  }

  close(sd);
  exit(0);
}