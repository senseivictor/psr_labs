#include <stdio.h>  // Biblioteca standard pentru intrare/ieșire
#include <stdlib.h>  // Biblioteca pentru funcții utilitare (exit, atoi)
#include <string.h>  // Biblioteca pentru manipularea șirurilor de caractere
#include <sys/types.h>  // Definiții de tipuri de date pentru sistem
#include <sys/socket.h>  // Biblioteca principală pentru socket-uri
#include <netinet/in.h>  // Structuri pentru adrese Internet
#include <netdb.h>  // Definiții pentru operații de rețea
#include <sys/param.h>  // Definiții de parametri de sistem
#include <errno.h>  // Pentru gestionarea erorilor
#include <unistd.h>  // Necesară pentru funcțiile read, write și close

#define ERROR(s) {fprintf(stderr,"%d-",errno); perror(s); return(-1);}  // Macro pentru erori

int main (int argc, char *argv[])  // Definit ca int pentru standardele moderne
{
   struct sockaddr_in sa;  // Structură pentru adresa serverului
   struct sockaddr_in caller;  // Structură pentru adresa clientului
   int ssd;  // Descriptor socket de ascultare (cel principal)
   int csd;  // Descriptor socket de conexiune (specific fiecărui client)
   int length;  // Lungimea structurii adresei
   int retval;  // Rezultatul citirii/scrierii
   int action;  // Variabilă de stare pentru logică
   char buf[BUFSIZ];  // Buffer pentru date
   
   // Mesaje trimise către client
   char* prompt[]={">>", 
       "Available commands: \n\tput - to send a text to server \thelp - to view this help\n\tquit - to close the connection\n>>", 
       "Enter the text terminating it with '.' in a new line.\n>>", 
       "Enter a command\n>>"};
   
   if (argc != 2) {  // Verifică dacă portul a fost dat ca argument
     fprintf (stdout, "usage: %s port\n", argv[0]);
     exit (-1);
   }
   
   sa.sin_family= AF_INET;  // Familia de protocoale IPv4
   sa.sin_addr.s_addr = INADDR_ANY;  // Ascultă pe toate interfețele disponibile
   sa.sin_port= htons((u_short) atoi(argv[1]));  // Portul convertit în format de rețea
   
   if ((ssd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)  // Creare socket TCP
     ERROR ("socket");
   
   if (bind(ssd, (struct sockaddr *)&sa, sizeof sa) < 0)  // Asociere socket cu portul
     ERROR ("bind");  
   
   listen (ssd, 5);  // Serverul intră în stare de ascultare (coadă de 5 conexiuni)

   // --- MODIFICARE: Buclă infinită pentru a menține serverul activ ---
   while(1) {
     printf("Serverul asteapta conexiuni pe portul %s...\n", argv[1]);
     
     length = sizeof(caller);
     // Acceptă o conexiune nouă. Programul se blochează aici până vine un client.
     if ((csd = accept(ssd, (struct sockaddr *)&caller, &length)) < 0)
       ERROR ("accept");

     printf("Client nou conectat!\n");
     action = -1;  // Resetăm starea pentru noul client

     write(csd, prompt[0], strlen(prompt[0]));  // Trimitem primul salut

     // Bucla de comunicare cu clientul curent
     while(action != 2){
       // Citim datele. Dacă read returnează <= 0, clientul s-a deconectat forțat.
       if((retval = read(csd, buf, sizeof(buf))) <= 0) {
         action = 2;  // Ieșim din bucla acestui client
         continue;
       }
       buf[retval]='\0';  // Terminăm șirul citit

       // Interpretarea comenzilor primite
       if (strcmp(buf,"help\n")==0) action=0;
       else if (strcmp(buf,"put\n")==0) action=1;
       else if (strcmp(buf,"quit\n")==0) action=2;
       else if (strcmp(buf,".\n")==0) action=3;

       switch (action) {
         case 0:   // Comanda Help
           write(csd, prompt[1], strlen(prompt[1]));
           action=-1;
           break;
         case 1:   // Comanda Put
           write(csd, prompt[2], strlen(prompt[2]));
           action=4; // Trecem în modul de afișare text
           break;
         case 2:   // Comanda Quit
           printf("Clientul a cerut deconectarea.\n");
           break;
         case 3:   // Terminare bloc text (punctul)
           write(csd, prompt[3], strlen(prompt[3]));
           action=-1; 
           break;
         case 4 :  // Afișăm în terminalul serverului ce scrie clientul
           printf("%s", buf);
           break;
         default:  // Orice altceva (comandă necunoscută)
           write(csd, prompt[3], strlen(prompt[3])); 
       }
     }
     
     close(csd);  // ÎNCHIDEM DOAR conexiunea cu clientul curent
     printf("Sesiune incheiata. Serverul ramane deschis.\n\n");
     // După close(csd), bucla while(1) revine la accept() pentru următorul client.
   }

   close(ssd);  // Această linie nu va fi atinsă, dar e bine să fie acolo
   exit(0);
}