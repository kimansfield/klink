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

#define MYTALKPORT "5932"

#define MAXBUFLEN 100

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
   if (sa->sa_family == AF_INET) {
      return &(((struct sockaddr_in*)sa)->sin_addr);
   }

   return &(((struct sockaddr_in6*)sa)->sin6_addr);
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

   printf("listener: waiting to recvfrom...\n");

   while(1)
   {
      addr_len = sizeof their_addr;
      if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
                  (struct sockaddr *)&their_addr, &addr_len)) == -1) {
         //perror("recvfrom");
         //exit(1);
      }
      else
      {

         printf("listener: got packet from %s\n",
               inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr),
                  s, sizeof s));
         printf("listener: packet is %d bytes long\n", numbytes);
         buf[numbytes] = '\0';
         printf("listener: packet contains \"%s\"\n", buf);
         if (!strcmp(buf,"HouseInfo"))
         {
            talk((char *)inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),s, sizeof s),"Register HouseInfo");
         }
      }
   }

   close(sockfd);

   return 0;
}
