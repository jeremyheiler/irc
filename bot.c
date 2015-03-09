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
#include <pthread.h>

#include "irc.h"

/* TODO: synchronize writes to stdout */

#define MAX_MESSAGE_SIZE 512

void
irc_parse_reply(char *reply)
{
        
}

int
bot_rstart(void *arg)
{
        return 0;
}

int
bot_wstart(void *arg)
{
        return 0;
}

int
main(int argc, char *argv[])
{
        char *channel;
        
        if (argc > 1) {
                channel = argv[1];
        } else {
                printf("Usage: %s <channel>\n", argv[0]);
                return -1;
        }
        
        int rv;
        
        char *host = "chat.freenode.net";
        char *port = "6667";

        int sock = irc_connect(host, port);
        
        if (sock == -1) {
                perror("socket");
                return -1;
        }

        pthread_t *rthread;
        pthread_t *wthread;

        /* pass in the other pthread for mutually assured destruction */
        rv = pthread_create(rthread, 
                            NULL, 
                            (void *) bot_rstart,
                            NULL);
        //(void *) wthread);

        if (rv != 0) {
                printf("pthread_create r: %d\n", rv);
                irc_free(sock);
                return -1;
        }
                
        rv = pthread_create(wthread,
                            NULL,
                            (void *) bot_wstart,
                            NULL);
        //(void *) rthread);

        if (rv != 0) {
                printf("pthread_create w: %d\n", rv);
                /* TODO(jeremy): cleanup rthread */
                rv = pthread_cancel(*rthread);
                if (rv != 0) {
                        printf("pthread_cancel rthread error %d\n", rv);
                }
                irc_free(sock);
                return -1;
        }

        int *rrv;
        int *wrv;
        
        rv = pthread_join(*rthread, (void **)&rrv);

        if (rv == -1) {
                perror("pthread_join r");
                /* TODO(jeremy): cleanup wthread */
                rv = pthread_cancel(*wthread);
                if (rv != 0) {
                        printf("pthread_cancel wthread error %d\n", rv);
                }
                irc_free(sock);
                return -1;
        }
        
        rv = pthread_join(*wthread, (void **)&wrv);

        if (rv == -1) {
                perror("pthread_join w");
                irc_free(sock);
                return -1;
        }
#ifdef COMMENT
#endif

        irc_free(sock);
        return 0;
}
