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

#include "irc.h"

/* TODO: synchronize writes to stdout */

int
main(int argc, char *argv[])
{
        int status;
        
        char *host = "chat.freenode.net";
        char *port = "6667";

        int sock = irc_connect(host, port);
        
        if (sock == -1) {
                perror("socket");
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
                        irc_free(sock);
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
                                irc_free(sock);
                                return -1;
                        }

                        if (inbuf[r - 1] == '\n') {
                                inbuf[r - 1] = '\r';
                                inbuf[r] = '\n';
                        }

                        /* ensure we can write to sock */
                        if (!FD_ISSET(sock, &wfds)) {
                                /* select on sock for writing */
                                fd_set wfds_sock;
                                FD_SET(sock, &wfds_sock);
                                s = select(sock + 1, NULL, &wfds_sock, NULL, NULL);
                                /* always going to be sock or error */
                                if (s == -1) {
                                        perror("select");
                                        irc_free(sock);
                                        return -1;
                                }
                        }

                        /* write to socket */
                        r = send(sock, inbuf, r + 1, 0);
                        if (r == -1) {
                                perror("send");
                                irc_free(sock);
                                return -1;
                        }
                }

                if (FD_ISSET(sock, &rfds)) {
                        /* read socket */
                        ssize_t r;
                        
                        char buf[MAX_MESSAGE_SIZE + 1];
                        memset(&buf, 0, sizeof buf);
                        r = recv(sock, (void *) &buf, MAX_MESSAGE_SIZE, 0);
                        if (r < 1) {
                                perror("recv buf");
                                irc_free(sock);
                                return -1;
                        }

                        if (buf[MAX_MESSAGE_SIZE] != '\0') {
                                char ovr[MAX_MESSAGE_SIZE];
                                /* truncate message to inclde CRLN
                                buf[MAX_MESSAGE_SIZE - 2] = '\r';
                                buf[MAX_MESSAGE_SIZE - 1] = '\n';*/
                                /* drop bytes over the initial 512 */
                                while (r == MAX_MESSAGE_SIZE) {
                                        r = recv(sock, (void *) &ovr, MAX_MESSAGE_SIZE, 0);
                                        if (r < 1) {
                                                perror("recv ovr");
                                                irc_free(sock);
                                                return -1;
                                        }
                                }
                        }
                        
                        /* ensure we can write to stdout */
                        if (FD_ISSET(STDOUT_FILENO, &wfds)) {
                                /* select on stdout for writing */
                                fd_set wfds_stdout;
                                FD_SET(STDOUT_FILENO, &wfds_stdout);
                                s = select(STDOUT_FILENO + 1, NULL, &wfds_stdout, NULL, NULL);
                                /* always going to be stdout or error */
                                if (s == -1) {
                                        perror("select");
                                        return -1;
                                }
                        }

                        /* write to stdout */
                        printf("%s", buf);
                }

        }
                
        irc_free(sock);
        return 0;
}
