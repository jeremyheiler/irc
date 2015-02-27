#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

int run = 1;

void
irc_handle_signal(int signo)
{
        run = 0;
}


void 
irc_close(int fd)
{
        if (close(fd) == -1) {
                perror("close");
        }
}

int
main(int argc, char *argv[])
{
        if (signal(SIGINT, irc_handle_signal) == SIG_ERR) {
                perror("signal");
                return -1;
        }

        int status;
        
        char *host = "chat.freenode.net";
        char *port = "6667";

        struct addrinfo hints;
        struct addrinfo *servinfo;

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        status = getaddrinfo(host, port, &hints, &servinfo);

        if (status == -1) {
                perror(NULL);
                return -1;
        }
        
        /* don't assume the first address in servinfo is good */
        struct sockaddr_in *ip = (struct sockaddr_in *) servinfo->ai_addr;
        void *addr = &(ip->sin_addr);
        char ipstr[INET_ADDRSTRLEN];
        inet_ntop(servinfo->ai_family, addr, ipstr, sizeof ipstr);
        printf("%s\n", ipstr);

        /* socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol); */
        int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == -1) {
                perror("Socket");
                return -1;
        }
        printf("Socket FD: %d\n", sock);

        status = connect(sock, servinfo->ai_addr, servinfo->ai_addrlen);
        if (status == -1) {
                perror(NULL);
                irc_close(sock);
                return -1;
        }

        int MAX_MESSAGE_SIZE = 512;
        char buf[MAX_MESSAGE_SIZE + 1];
        char ovr[MAX_MESSAGE_SIZE];

        ssize_t read;

        while (run == 1) {
                memset(&buf, 0, sizeof buf);
                read = recv(sock, (void *) &buf, MAX_MESSAGE_SIZE, 0);
                if (read < 1) {
                        perror("recv");
                        irc_close(sock);
                        return -1;
                }

                /* drop bytes over the initial 512 */
                if (buf[MAX_MESSAGE_SIZE] != '\0') {
                        /* truncate message to inclde CRLN */
                        buf[MAX_MESSAGE_SIZE - 2] = '\r';
                        buf[MAX_MESSAGE_SIZE - 1] = '\n';
                        while (read == MAX_MESSAGE_SIZE) {
                                read = recv(sock, (void *) &ovr, MAX_MESSAGE_SIZE, 0);
                                if (read < 1) {
                                        perror("recv");
                                        irc_close(sock);
                                        return -1;
                                }
                        }
                }
                printf("%s", buf);
        }
                
        irc_close(sock);
        freeaddrinfo(servinfo);
        return 0;
}
