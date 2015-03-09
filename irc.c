#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

/*
#include <sys/types.h>
#include <arpa/inet.h>

#include <sys/select.h>
*/
#include "irc.h"

/* QUES(jeremy): does his have the right scope? */
struct addrinfo *servinfo;

/* QUES(jeremy): return an irc struct?
 * TODO(jeremy): rename to dial?
 */
int
irc_connect(char *host, char *port)
{
        struct addrinfo hints;
        
        memset(&hints, 0, sizeof hints);
        
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        
        int status = getaddrinfo(host, port, &hints, &servinfo);
        /* TODO(jeremy): don't assume the first address in servinfo is good */
        
        if (status == -1) {
                perror("addrinfo");
                return -1;
        }
        
        int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (sock == -1) {
                freeaddrinfo(servinfo);
                perror("connect socket");
                return -1;
        }

        status = connect(sock, servinfo->ai_addr, servinfo->ai_addrlen);

        if (status == -1) {
                perror("connect");
                irc_free(sock);
                return -1;
        }

        /* QUES(jeremy): can i free servinfo here? do i want to? */
        
        return sock;
}

/*
int
irc_listen(char *port)
{
        int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (sock == -1) {
                perror("listen socket");
                return -1;
        }

        int one = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int)) == -1) {
                perror("sol sock opt");
                irc_listen_free(sock);
                return -1;
        }

        int status;

        struct sockaddr_in addr;
        memset(addr.sin_zero, '\0', sizeof addr.sin_zero);
        addr.sin_family = AF_INET;
        addr.sin_port = port; //htons(2323);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        
        status = bind(sock, (struct sockaddr *) &addr, sizeof addr);
        
        if (status == -1) {
                perror("bind");
                irc_listen_free(sock);
                return -1;
        }

        status = listen(sock, 10);

        if (status == -1) {
                perror("listen");
                irc_listen_free(sock);
                return -1;
        }
        
}

void
irc_listen_free(int sockfd)
{
        if (close(sockfd) == -1) {
                perror("listen close");
        }        
}
*/

void
irc_free(int sockfd)
{
        freeaddrinfo(servinfo);
        if (close(sockfd) == -1) {
                perror("connect close");
        }        
}

