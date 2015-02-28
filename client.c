 #include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
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

        int nfds = STDOUT_FILENO;
        if (sock > nfds) {
                nfds = sock;
        }
        ++nfds;

        int MAX_MESSAGE_SIZE = 512;
        
        int s;

        while (1) {
                fd_set rfds;
                fd_set wfds;
        
                FD_SET(STDIN_FILENO, &rfds);
                FD_SET(sock, &rfds);
                FD_SET(STDOUT_FILENO, &wfds);
                FD_SET(sock, &wfds);

                s = select(nfds, &rfds, &wfds, NULL, NULL);

                if (s == -1) {
                        perror("select");
                        return -1;
                }

                if (FD_ISSET(STDIN_FILENO, &rfds)) {
                        /* read input */
                        ssize_t r;

                        char inbuf[MAX_MESSAGE_SIZE + 2];
                        memset(&inbuf, 0, sizeof inbuf);
                        r = read(STDOUT_FILENO, inbuf, MAX_MESSAGE_SIZE);
                        if (r < 1) {
                                perror("recv");
                                irc_close(sock);
                                return -1;
                        }

                        if (inbuf[r - 1] == '\n') {
                                printf("replaceing new line...");
                                inbuf[r - 1] = '\r'; /* replaces \n */
                                inbuf[r] = '\n'; /* insert a new \n */
                        }
                        
                        /* ensure we can write to sock */
                        if (!FD_ISSET(sock, &wfds)) {
                                /* select on sock for writing */
                                fd_set wfds_sock;
                                FD_SET(sock, &wfds_sock);
                                printf("selecting write sock\n");
                                s = select(sock + 1, NULL, &wfds_sock, NULL, NULL);
                                /* always going to be sock or error */
                                if (s == -1) {
                                        perror("select");
                                        return -1;
                                }
                        }

                        /* write to socket */
                        r = send(sock, inbuf, r + 1, 0);
                        if (r == -1) {
                                perror("send");
                                irc_close(sock);
                                return -1;
                        }
                }

                if (FD_ISSET(sock, &rfds)) {
                        /* read socket */
                        ssize_t r;
                        
                        char buf[MAX_MESSAGE_SIZE + 1];
                        char ovr[MAX_MESSAGE_SIZE];
                        memset(&buf, 0, sizeof buf);
                        r = recv(sock, (void *) &buf, MAX_MESSAGE_SIZE, 0);
                        if (r < 1) {
                                perror("recv buf");
                                irc_close(sock);
                                return -1;
                        }

                        if (buf[MAX_MESSAGE_SIZE] != '\0') {
                                /* truncate message to inclde CRLN */
                                buf[MAX_MESSAGE_SIZE - 2] = '\r';
                                buf[MAX_MESSAGE_SIZE - 1] = '\n';
                                /* drop bytes over the initial 512 */
                                while (r == MAX_MESSAGE_SIZE) {
                                        r = recv(sock, (void *) &ovr, MAX_MESSAGE_SIZE, 0);
                                        if (r < 1) {
                                                perror("recv ovr");
                                                irc_close(sock);
                                                return -1;
                                        }
                                }
                        }
                        
                        /* ensure we can write to stdout */
                        if (FD_ISSET(STDOUT_FILENO, &wfds)) {
                                /* select on stdout for writing */
                                fd_set wfds_stdout;
                                FD_SET(STDOUT_FILENO, &wfds_stdout);
                                printf("selecting write stdout\n");
                                s = select(STDOUT_FILENO + 1, NULL, &wfds_stdout, NULL, NULL);
                                /* always going to be stdout or error */
                                if (s == -1) {
                                        perror("select");
                                        return -1;
                                }
                        }

                        /* write to stdout */
                        printf("> %s", buf);
                }

        }
                
        irc_close(sock);
        freeaddrinfo(servinfo);
        return 0;
}
