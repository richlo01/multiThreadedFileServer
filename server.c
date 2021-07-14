// Rionel Dmello(15028109) and Richard Lopez(69944917)
#include <unistd.h> 
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netdb.h>
#include <netinet/in.h> 
#include <string.h> 
#include <pthread.h>
#include <semaphore.h>
#define MAXLINE 256
#define NTHREADS 4

/* Global Variables */
char openFiles[4][MAXLINE];
pthread_t tids[4];
int fileCount = 0;
sem_t mutex;

/* Function prototypes */
int open_listenfd(char* port);
void* thread(void* varp);
void echo(int connfd);
int isWritten(char* filename);
int isOpen(char* filename);
int getFileIndex(char* filename);
void fixOpenFileArray(int idx);

int main(int argc, char* argv[])
{   // Command line argument: ./server 30000
    int i, listenfd, *connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;
    
    sem_init(&mutex, 0, 1);
    listenfd = open_listenfd(argv[1]);
    int serverStarted = 0;
    // for (i  = 0; i < NTHREADS; i++)  /* Create worker threads */
    //     pthread_create(&tid, NULL, thread, NULL);               
    while(1) 
    {
        clientlen= sizeof(struct sockaddr_storage);
        connfd = malloc(sizeof(int));
        *connfd = accept(listenfd, (struct sockaddr*) &clientaddr, &clientlen);
        if (serverStarted == 0)
        {
            printf("server started\n");
            serverStarted++;
        }
        pthread_create(&tid, NULL, thread, connfd);  
    }
}

void* thread(void* vargp)
{  
    int connfd = *((int*)vargp);
    pthread_detach(pthread_self());
    free(vargp);
    echo(connfd);                /* Service client*/
    close(connfd);
    return NULL;
}

void echo(int connfd)
{
    size_t n;
    int i, stringLen, hasOpen=0;
    char buf[MAXLINE];
    char result[MAXLINE];
    FILE *fp;

    while((n = read(connfd, buf, MAXLINE)) != 0)
    {   
        printf("%s\n", buf);
        if (strncmp(buf, "openRead", 8) == 0){
            sem_wait(&mutex);
            if (hasOpen == 0 && isWritten(buf+9) == 0)
            {
                fp = fopen(buf+9, "r");
                openFiles[fileCount][0] = 0;        // 0 for reading, 1 for appendinng
                tids[fileCount] = pthread_self();
                strcpy(openFiles[fileCount]+1,buf+9);
                fileCount++;
                hasOpen = 1;
                sprintf(result, "");
                write(connfd, result, MAXLINE);
            }
            else if (hasOpen == 1)
            {
                printf("A file is already open\n");
                sprintf(result, "A file is already open\n");
                write(connfd, result, MAXLINE);
            }
            else if (isWritten(buf+9) == 1)
            {
                printf("The file is open by another client\n");
                sprintf(result, "The file is open by another client\n");
                write(connfd, result, MAXLINE);
            }
            sem_post(&mutex);
        }
        else if (strncmp(buf, "openAppend", 10) == 0){
            sem_wait(&mutex);
            if (hasOpen == 0 && isOpen(buf+11) == 0)
            {
                fp = fopen(buf+11, "a");
                openFiles[fileCount][0] = 1;        // 0 for reading, 1 for appendinng
                strcpy(openFiles[fileCount]+1,buf+11);
                tids[fileCount] = pthread_self();
                fileCount++;
                hasOpen = 1;
                sprintf(result, "");
                write(connfd, result, MAXLINE);
            }
            else if (hasOpen == 1)
            {
                printf("A file is already open\n");
                sprintf(result, "A file is already open\n");
                write(connfd, result, MAXLINE);
            }
            else if (isOpen(buf+11) == 1)
            {
                printf("The file is open by another client\n");
                sprintf(result, "The file is open by another client\n");
                write(connfd, result, MAXLINE);
            }
            sem_post(&mutex);
        }
        else if (strncmp(buf, "close", 5) == 0){
            sem_wait(&mutex);
            fclose(fp);
            fixOpenFileArray(getFileIndex(buf+6));
            fileCount--;
            hasOpen = 0;
            sem_post(&mutex);
        }
        else if (strncmp(buf, "read", 4) == 0){
            size_t k; 
            if (hasOpen == 1)
            {
                k = fread(result, 1, atoi(buf+5),fp);
                if (k!=0)
                {
                    result[k] = '\n';
                    result[k+1] = '\0';
                    write(connfd, result, atoi(buf+5)+2);
                }
                else
                {
                    write(connfd, "", 1);
                }
            }
            else
            {
                printf("File not open\n");
                sprintf(result, "File not open\n");
                write(connfd, result, MAXLINE);
            }
        }
        else if (strncmp(buf, "append", 6) == 0){
            if (hasOpen == 1)
            {
                fwrite(buf+7, 1, strlen(buf+7), fp);
                write(connfd, "", 1);
            }
            else
            {
                printf("File not open\n");
                sprintf(result, "File not open\n");
                write(connfd, result, MAXLINE);
            }

        }
        memset(buf, 0, MAXLINE);
    }
}

int getFileIndex(char* filename)
{
    int i;
    for (i=0; i<fileCount; i++)
    {
        if (strcmp(openFiles[i]+1, filename) == 0 && tids[i] == pthread_self())
        {
            return i;
        }
    }
    return -1;
}

void fixOpenFileArray(int idx)
{
    int i;
    pthread_t temp;
    if (idx == 3)
    {
        tids[3] = 0;
        memset(openFiles[3],0,MAXLINE);
    }
    else
    {
        for (i=idx; i<fileCount-1; i++)
        {
            memset(openFiles[i],0,MAXLINE);
            openFiles[i][0] = openFiles[i+1][0];
            strcpy(openFiles[i]+1,openFiles[i+1]+1);
            temp = tids[i+1];
            tids[i] = temp;
            tids[i+1] = 0;
        }
        memset(openFiles[i+1],0,MAXLINE);
    }
}

int isWritten(char* filename)
{
    int i;
    for (i=0; i<fileCount; i++)
    {
        if (strcmp(openFiles[i]+1,filename) == 0 && (int) openFiles[i][0] == 1)
        {
            return 1;
        }
    }
    return 0;
}

int isOpen(char* filename)
{
    int i;
    for (i=0; i<fileCount; i++)
    {
        if (strcmp(openFiles[i]+1,filename) == 0)
        {
            return 1;
        }
    }
    return 0;
}

int open_listenfd(char* port)
{   // connects to a client
    struct addrinfo hints, *listp, *p;
    int listenfd, optval=1;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
    hints.ai_flags |= AI_NUMERICSERV;
    getaddrinfo(NULL, port, &hints, &listp);

    for (p = listp; p; p=p->ai_next)
    {
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue; // if socket failed, go to the next one
        
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));

        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
        {
            break; // signifies success
        }
        
        close(listenfd); // bind failed, try the next one
    }

    freeaddrinfo(listp);
    if (!p) // if no address worked 
        return -1;

    if (listen(listenfd,1) < 0)
    {
        close(listenfd);
        return -1;
    }

    return listenfd;
}