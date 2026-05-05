#include <stdio.h>  // Biblioteca pentru intrare/ieșire standard
#include <stdlib.h>  // Biblioteca pentru funcții de conversie și control (exit, atoi)
#include <string.h>  // Biblioteca pentru manipularea șirurilor de caractere
#include <sys/types.h>  // Tipuri de date folosite în apelurile de sistem
#include <sys/socket.h>  // Definiții pentru lucrul cu socket-uri
#include <netinet/in.h>  // Structuri pentru adrese Internet IPv4
#include <netdb.h>  // Pentru rezoluția numelor de host (gethostbyname)
#include <sys/param.h>  // Parametri de sistem
#include <errno.h>  // Pentru gestionarea codurilor de eroare
#include <unistd.h> // Pentru functiile read/write/close

#define ERROR(s) {printf("%d-",errno); perror(s); exit(-1);}  // Macro pentru raportarea erorilor

char buf[BUFSIZ];  // Buffer global pentru stocarea mesajelor primite sau trimise

int main (int argc, char *argv[]) // Folosim standardul int main
{
 int sock;  // Descriptorul de socket pentru client
 struct sockaddr_in sa;  // Structura pentru adresa serverului
 struct hostent * host;  // Structura pentru informații despre host (IP)
 int n;  // Variabilă pentru numărul de octeți citiți
 int addr_len; // Lungimea structurii adresei pentru recvfrom
 
 if (argc!=3) {  // Verifică dacă utilizatorul a furnizat host-ul și portul
   printf("Usage: %s hostname port\n",argv[0]);
   exit (-1);
 }
 
 // --- MODIFICARE UDP: Schimbăm SOCK_STREAM în SOCK_DGRAM ---
 if((sock=socket(AF_INET, SOCK_DGRAM, 0))<0)  
   ERROR("socket");
 
 if((host=gethostbyname(argv[1]))==(struct hostent*)NULL)  // Traducerea numelui host-ului în adresă IP
   ERROR("gethostbyname");
 
 memcpy((char*)&sa.sin_addr, (char*)host->h_addr, host->h_length);  // Copierea adresei IP găsite în structura sockaddr_in
 sa.sin_family=AF_INET;  // Setează familia de adrese la IPv4
 sa.sin_port=htons((u_short)atoi(argv[2]));  // Convertește portul primit ca string în format de rețea
 
 // NOTA: In UDP nu este strict necesar connect(). 
 // Vom trimite un prim pachet "dummy" pentru a initia dialogul cu serverul
 sprintf(buf, "INIT"); 
 sendto(sock, buf, strlen(buf), 0, (struct sockaddr *)&sa, sizeof sa);

 while(strcmp(buf,"quit\n")!=0) {  // Buclă principală până la introducerea comenzii quit
   
   // --- MODIFICARE UDP: Asteptam promptul folosind recvfrom ---
   while(!((buf[strlen(buf)-2]=='>') && (buf[strlen(buf)-1]=='>'))) {  
     addr_len = sizeof(sa);
     if ((n=recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *)&sa, &addr_len)) < 0)  
       ERROR("recvfrom");
     buf[n]=0;  // Asigură închiderea șirului de caractere pentru printf
     printf("%s", buf);  // Afișează mesajul primit pe ecran
   } 

   if(strcmp(buf, "Enter the text terminating it with '.' in a new line.\n>>")==0)   // Verifică dacă serverul cere un bloc de text
     while(buf[0]!='.') {  // Continuă să citească de la tastatură până la introducerea punctului
       fgets(buf, sizeof(buf), stdin);  // Citește o linie de la tastatură
       // --- MODIFICARE UDP: Trimitem folosind sendto ---
       if (sendto(sock, buf, strlen(buf), 0, (struct sockaddr *)&sa, sizeof sa)<0)  
         ERROR("sendto");
     }
   else {  // Cazul în care se așteaptă o comandă simplă (help, put, quit)
     fgets(buf, sizeof(buf), stdin);  // Citește comanda de la utilizator
     // --- MODIFICARE UDP: Trimitem folosind sendto ---
     if (sendto(sock, buf, strlen(buf), 0, (struct sockaddr *)&sa, sizeof sa)<0)  
       ERROR("sendto");    
     
     // Daca e quit, iesim din bucla
     if(strcmp(buf, "quit\n")==0) break;
   } 
   
   // Resetam bufferul pentru a reintra in bucla de asteptare prompt (recvfrom)
   memset(buf, 0, sizeof(buf));
 }

 close(sock);
 return 0;
}