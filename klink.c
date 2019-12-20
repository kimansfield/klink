/*
 ** klink.c -- a datagram sockets "server" demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MYPORT "4950"    // the port users will be connecting to
#define SERVERPORT 4950

#define MYTALKPORT "5932"

#define MAXBUFLEN 100

#define NUMIPS 16
#define LENGTHIPS 16

char registeredIP[NUMIPS][LENGTHIPS];

void initregs(void)
{
   int i;

   for (i=0; i<NUMIPS; i++)
   {
      registeredIP[i][0] = 0;
   }

}

void rmregs(char *ip)
{
   int i;
   int done = 0;

   while (!done)
   {
      if (!strcmp(registeredIP[i],ip))
      {
         registeredIP[i][0] = 0;
         done = 1;
      }
      i++;
      if (i >= NUMIPS)
         done = 1;
   }
}

void regs(char *ip)
{
   int i;
   int done = 0;
   char tip[NUMIPS];

   sprintf(tip,"%d.%d.%d.%d",ip[0],ip[1],ip[2],ip[3]);
   printf("register IP = %s\n",tip);
   i = 0;
   while (!done)
   {
      if (registeredIP[i][0] == 0)
      {
         strcpy(registeredIP[i],tip);
         done = 1;
      }
      else
      if (!strcmp(registeredIP[i],tip))
      {
         done = 1;
      }
      if (i >= NUMIPS)
         done = 1;

      i++;
   }
   //for (i=0; i<NUMIPS; i++)
   //{
   //   printf("Value %d = %s\n",i,registeredIP[i]);
   //}
   return ;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
   if (sa->sa_family == AF_INET) {
      return &(((struct sockaddr_in*)sa)->sin_addr);
   }

   return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int broadcast(char *ip, char *message)
{
    int sockfd;
    struct sockaddr_in their_addr; // connector's address information
    struct hostent *he;
    int numbytes;
    int broadcast = 1;
    //char broadcast = '1'; // if that doesn't work, try this

    //if (argc != 3) {
    //    fprintf(stderr,"usage: broadcaster hostname message\n");
    //    exit(1);
    //}

    if ((he=gethostbyname(ip)) == NULL) {  // get the host info
        perror("gethostbyname");
        exit(1);
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    // this call is what allows broadcast packets to be sent:
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast,
        sizeof broadcast) == -1) {
        perror("setsockopt (SO_BROADCAST)");
        exit(1);
    }

    their_addr.sin_family = AF_INET;     // host byte order
    their_addr.sin_port = htons(SERVERPORT); // short, network byte order
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    memset(their_addr.sin_zero, '\0', sizeof their_addr.sin_zero);

    if ((numbytes=sendto(sockfd, message, strlen(message), 0,
             (struct sockaddr *)&their_addr, sizeof their_addr)) == -1) {
        perror("sendto");
        exit(1);
    }

    printf("sent %d bytes to %s\n", numbytes,
        inet_ntoa(their_addr.sin_addr));

    close(sockfd);

    return 0;
}

int talk(char *ip, char *message)
{
   int sockfd;
   struct addrinfo hints, *servinfo, *p;
   int rv;
   int numbytes;

   memset(&hints, 0, sizeof hints);
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_DGRAM;

   if ((rv = getaddrinfo(ip, MYTALKPORT, &hints, &servinfo)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
      return 1;
   }

   // loop through all the results and make a socket
   for(p = servinfo; p != NULL; p = p->ai_next) {
      if ((sockfd = socket(p->ai_family, p->ai_socktype,
                  p->ai_protocol)) == -1) {
         perror("talker: socket");
         continue;
      }

      break;
   }

   if (p == NULL) {
      fprintf(stderr, "talker: failed to create socket\n");
      return 2;
   }

   if ((numbytes = sendto(sockfd, message, strlen(message), 0,
               p->ai_addr, p->ai_addrlen)) == -1) {
      perror("talker: sendto");
      exit(1);
   }

   freeaddrinfo(servinfo);

   printf("talker: sent %d bytes to %s\n", numbytes, ip);
   close(sockfd);

   return 0;
}

int main(void)
{
   int sockfd;
   struct addrinfo hints, *servinfo, *p;
   int rv;
   int numbytes;
   struct sockaddr_storage their_addr;
   char buf[MAXBUFLEN];
   socklen_t addr_len;
   char s[INET6_ADDRSTRLEN];
   struct timeval read_timeout;
   int txDelay = 0;
   char *tok1;
   char *tok2;

   initregs();
   read_timeout.tv_sec = 0;
   read_timeout.tv_usec = 10;

   memset(&hints, 0, sizeof hints);
   hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
   hints.ai_socktype = SOCK_DGRAM;
   hints.ai_flags = AI_PASSIVE; // use my IP

   if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
      return 1;
   }

   // loop through all the results and bind to the first we can
   for(p = servinfo; p != NULL; p = p->ai_next) {
      if ((sockfd = socket(p->ai_family, p->ai_socktype,
                  p->ai_protocol)) == -1) {
         perror("listener: socket");
         continue;
      }

      if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
         close(sockfd);
         perror("listener: bind");
         continue;
      }

      break;
   }

   if (p == NULL) {
      fprintf(stderr, "listener: failed to bind socket\n");
      return 2;
   }

   setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

   freeaddrinfo(servinfo);

   //printf("listener: waiting to recvfrom...\n");

   while(1)
   {
      txDelay++;
      if (txDelay > 128)
      {
        broadcast("192.168.0.255","houseinfo");
        txDelay = 0;
	// send values
        for (int i=0; i<NUMIPS; i++)
        {
           if (registeredIP[i][0] != 0)
           {
              sprintf(buf,"houseinfo %d",rand());
	      printf("send %s to %s\n",buf,registeredIP[i]);
              talk(registeredIP[i],buf);
           }
        }
      }

      addr_len = sizeof their_addr;
      if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
                  (struct sockaddr *)&their_addr, &addr_len)) == -1) {
         //perror("recvfrom");
         //exit(1);
      }
      else
      {

         //printf("listener: got packet from %s\n",
         //      inet_ntop(their_addr.ss_family,
         //         get_in_addr((struct sockaddr *)&their_addr),
         //         s, sizeof s));
         //printf("listener: packet is %d bytes long\n", numbytes);
         buf[numbytes] = '\0';
         //printf("klink: packet contains \"%s\"\n", buf);
         tok1 = strtok(buf," ");
         tok2 = strtok(NULL," ");
         if (!strcmp(tok1,"reg") && !strcmp(tok2,"houseinfo"))
         {
            // maybe we should return a value to see if we could
            // register this value
            regs((char *)get_in_addr((struct sockaddr *)&their_addr));
            //talk((char *)inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),s, sizeof s),(char *)"Register HouseInfo");
         }
         else
         {
            printf("Command not recognized\n");
         }
      }
      //printf("look for receive packet again\n");
   }

   close(sockfd);

   return 0;
}
