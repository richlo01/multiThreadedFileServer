// Rionel Dmello(15028109) and Richard Lopez(69944917)
#include <unistd.h> 
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netdb.h>
#include <netinet/in.h> 
#include <string.h>
#define MAXLINE 256
#define MAXARGS 128

/* Global Variables */

/* Function prototypes */
int open_clientfd(char* hostname, char* port);
int parseline(char *buf, char **argv);
int eval(char* cmdline, int fd);

int main(int argc, char* argv[])
{   // command line argument: “./client server.ics.uci.edu 30000”
    int clientfd;
    char host[MAXLINE], port[MAXLINE], buf[MAXLINE];
    int keepGoing = 1;
    char cmdline[MAXARGS];

    strcpy(host, argv[1]);
    strcpy(port, argv[2]);

    clientfd = open_clientfd(host,port);

    while (keepGoing == 1 && clientfd >= 0){
        /* Read */
        printf("> ");
        fgets(cmdline, MAXLINE, stdin);
        if (feof(stdin))
            break;

        /* evaluate command line */
        keepGoing = eval(cmdline, clientfd);

    }

    close(clientfd);
    exit(0);
}

int eval(char* cmdline, int fd)
{   // handles typed commands
    char* argv[MAXARGS]; // holds the tokens
    char buf[MAXLINE];
    int i, j, args, messageIdx=1;
    char size;
    char message[MAXLINE];

    strcpy(buf,cmdline); // copies to the buffer to remove extra spaces, etc.
    args = parseline(buf,argv); // copies the toks to argv, will also return its size
    if (strncmp(argv[0],"quit",4) == 0)
    {
        return 0;
    }
    else if (strncmp(argv[0],"openRead",8) == 0)
    {
        strncpy(message, argv[0], 8);
        strncpy(message+8," ",1);
        strcpy(message+9, argv[1]);
        write(fd, message, strlen(message));
        read(fd, message, MAXLINE);
        fputs(message, stdout);
        memset(message, 0, MAXLINE);
    }
    else if (strncmp(argv[0],"openAppend",10) == 0)
    {
        strncpy(message, argv[0], 10);
        strncpy(message+10," ",1);
        strcpy(message+11, argv[1]);
        write(fd, message, strlen(message));
        read(fd, message, MAXLINE);
        fputs(message, stdout);
        memset(message, 0, MAXLINE);
    }
    else if (strncmp(argv[0],"read",4) == 0)
    {
        strncpy(message, argv[0], 4);
        strncpy(message+4," ",1);
        strcpy(message+5, argv[1]);
        write(fd, message, strlen(message));
        // memset(message, 0, MAXLINE);
        read(fd, message, MAXLINE);
        fputs(message, stdout);
        memset(message, 0, MAXLINE);
    }
    else if (strncmp(argv[0],"append",6) == 0)
    {
        strncpy(message, argv[0], 6);
        strncpy(message+6," ",1);
        strcpy(message+7, argv[1]);
        write(fd, message, strlen(message));
        read(fd, message, MAXLINE);
        fputs(message, stdout);
        memset(message, 0, MAXLINE);
    }
    else if (strncmp(argv[0],"close",5) == 0)
    {
        strncpy(message, argv[0], 5);
        strncpy(message+5," ",1);
        strcpy(message+6, argv[1]);
        write(fd, message, strlen(message));
        // read(fd, message, MAXLINE);
        // fputs(message, stdout);
        memset(message, 0, MAXLINE);
    }
    else
    {
        printf("Invalid syntax\n");
    }
    return 1;

}

int open_clientfd(char *hostname, char *port) {
    int clientfd;
    struct addrinfo hints, *listp, *p;
    /* Get a list of potential server addresses */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM; /* Open a connection */
    hints.ai_flags = AI_NUMERICSERV; /* …using numeric port arg. */
    hints.ai_flags |= AI_ADDRCONFIG; /* Recommended for connections */
    getaddrinfo(hostname, port, &hints, &listp);
    /* Walk the list for one that we can successfully connect to */
    for (p = listp; p; p = p->ai_next) {
    /* Create a socket descriptor */
        if ((clientfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) < 0)
            continue; /* Socket failed, try the next */
        /* Connect to the server */
        if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1)
            break; /* Success */
        close(clientfd); /* Connect failed, try another */
    }
    /* Clean up */
    freeaddrinfo(listp);
    if (!p) /* All connects failed */
        return -1;
    else /* The last connect succeeded */
        return clientfd;
}

int parseline(char *buf, char **argv)
{   // puts arguments into a list, returns size of arguments
    char *delim; /* Points to first space delimiter */
    int argc; /* Number of args */

    buf[strlen(buf)-1] = ' '; /* Replace trailing ’\n’ with space */
    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
    buf++;

    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, ' '))) {
        argv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;
        while (*buf && (*buf == ' ')) /* Ignore spaces */
            buf++;
    }
    return argc;
}