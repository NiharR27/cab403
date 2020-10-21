/*
*  Materials downloaded from the web.
*  Collected and modified for teaching purpose only.
*/
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include "time.h"
#include <time.h>
#include <sys/time.h>
#include <stdbool.h>
#include <pthread.h> /* pthread functions and data structures     */

#define ARRAY_SIZE 90 /* Size of array to receive */

#define BACKLOG 10 /* how many pending connections queue will hold */

#define RETURNED_ERROR -1

#define NUM_HANDLER_THREADS 5

void oCommand(int argc, char *arg, char *destFile, char *filepath);
void logCommand(int argc, char *arg, char *destFile, char *filepath, char *buffer, struct tm *tm_info, pid_t pid, time_t tick, int status);
void echo(char *str, char *destFile);

/* global mutex for our program. */
pthread_mutex_t request_mutex;
pthread_mutex_t quit_mutex;
char *argv1[100];
int argc1;
int quit = 0;

/* global condition variable for our program. */
pthread_cond_t got_request;

int num_requests = 0; /* number of pending requests, initially none */

/* format of a single request. */
struct request
{
    void (*func)(void *);
    void *data;
    struct request *next; /* pointer to next request, NULL if none. */
};

struct request *requests = NULL;     /* head of linked list of requests. */
struct request *last_request = NULL; /* pointer to last request.         */

void add_request(void (*func)(void *),
                 void *data,
                 pthread_mutex_t *p_mutex,
                 pthread_cond_t *p_cond_var)
{
    struct request *a_request; /* pointer to newly added request.     */

    /* create structure with new request */
    a_request = (struct request *)malloc(sizeof(struct request));
    if (!a_request)
    { /* malloc failed?? */
        fprintf(stderr, "add_request: out of memory\n");
        exit(1);
    }
    a_request->func = func;
    a_request->data = data;
    a_request->next = NULL;

    /* lock the mutex, to assure exclusive access to the list */
    pthread_mutex_lock(p_mutex);

    /* add new request to the end of the list, updating list */
    /* pointers as required */
    if (num_requests == 0)
    { /* special case - list is empty */
        requests = a_request;
        last_request = a_request;
    }
    else
    {
        last_request->next = a_request;
        last_request = a_request;
    }

    /* increase total number of pending requests by one. */
    num_requests++;

    /* unlock mutex */
    pthread_mutex_unlock(p_mutex);

    /* signal the condition variable - there's a new request to handle */
    pthread_cond_signal(p_cond_var);
}

struct request *get_request()
{
    struct request *a_request; /* pointer to request.                 */

    if (num_requests > 0)
    {
        a_request = requests;
        requests = a_request->next;
        if (requests == NULL)
        { /* this was the last request on the list */
            last_request = NULL;
        }
        /* decrease the total number of pending requests */
        num_requests--;
    }
    else
    { /* requests list is empty */
        a_request = NULL;
    }

    /* return the request to the caller. */
    return a_request;
}

void handle_request(struct request *a_request, int thread_id)
{
    if (a_request)
    {
        a_request->func(a_request->data);
    }
}

void *handle_requests_loop(void *data)
{
    struct request *a_request;      /* pointer to a request.               */
    int thread_id = *((int *)data); /* thread identifying number           */

    /* lock the mutex, to access the requests list exclusively. */
    pthread_mutex_lock(&request_mutex);

    /* do forever.... */
    int running = 1;
    while (running)
    {

        if (num_requests > 0)
        { /* a request is pending */
            a_request = get_request();
            if (a_request)
            { /* got a request - handle it and free it */
                /* unlock mutex - so other threads would be able to handle */
                /* other reqeusts waiting in the queue paralelly.          */
                pthread_mutex_unlock(&request_mutex);
                handle_request(a_request, thread_id);
                free(a_request);
                /* and lock the mutex again. */
                pthread_mutex_lock(&request_mutex);
            }
        }
        else
        {
            /* wait for a request to arrive. note the mutex will be */
            /* unlocked here, thus allowing other threads access to */
            /* requests list.                                       */

            pthread_cond_wait(&got_request, &request_mutex);
            /* and after we return from pthread_cond_wait, the mutex  */
            /* is locked again, so we don't need to lock it ourselves */
        }
        pthread_mutex_lock(&quit_mutex);
        if (quit)
        {
            running = 0;
        }
        pthread_mutex_unlock(&quit_mutex);
    }
    pthread_mutex_unlock(&request_mutex);
    return NULL;
}

//socket funcs (receive message, receive length and receive array_int data)
char *receiveMessage(int socket_id)
{

    char *msg;
    int len;
    int recvLen = recv(socket_id, &len, sizeof(len), 0);

    if (recvLen != sizeof(len))
    {
        fprintf(stderr, "recv got invalid value ");
        exit(1);
    }

    len = ntohl(len);
    msg = malloc(len + 1); //+1 for null char.
    if (recv(socket_id, msg, len, 0) != len)
    {
        fprintf(stderr, "not all meessage receive");
        exit(1);
    }

    msg[len] = '\0';

    return msg;
}

int receiveLength(int socket_id)
{
    int num;
    int byes_received = recv(socket_id, &num, sizeof(num), 0);
    if (byes_received == -1)
    {
        perror("recv()");
        return 1;
    }
    num = ntohl(num);

    return num;
}

int *Receive_Array_Int_Data(int socket_identifier, int size)
{
    int number_of_bytes, i = 0;
    //   u_char statistics;
    char *buff[1024];
    int *results = malloc(sizeof(int) * size);

    //  for (i = 0; i < size; i++)
    //  {
    if ((number_of_bytes = recv(socket_identifier, buff, 1023, 0)) == RETURNED_ERROR)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buff[number_of_bytes] = '\0';
    printf("Received from server %s", buff);
    // }
    return results;
}

void test_func(void *len)
{

    int sockfd, new_fd;            /* listen on sock_fd, new connection on new_fd */
    struct sockaddr_in my_addr;    /* my address information */
    struct sockaddr_in their_addr; /* connector's address information */
    socklen_t sin_size;
    int i = 0;

    /* Get port number for server to listen on */

    if (argc1 != 2)
    {
        fprintf(stderr, "usage: port_number\n");
        exit(1);
    }

    /* generate the socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    /* Enable address/port reuse, useful for server development */
    int opt_enable = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt_enable, sizeof(opt_enable));
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &opt_enable, sizeof(opt_enable));

    /* clear address struct */
    memset(&my_addr, 0, sizeof(my_addr));

    /* generate the end point */
    my_addr.sin_family = AF_INET;             /* host byte order */
    my_addr.sin_port = htons(atoi(argv1[1])); /* short, network byte order */
    my_addr.sin_addr.s_addr = INADDR_ANY;     /* auto-fill with my IP */

    /* bind the socket to the end point */
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("bind");
        exit(1);
    }

    /* start listnening */
    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }

    /* repeat: accept, send, close the connection */
    /* for every accepted connection, use a sepetate process or thread to serve it */
    while (1)
    { /* main accept() loop */
        sin_size = sizeof(struct sockaddr_in);
        if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr,
                             &sin_size)) == -1)
        {
            perror("accept");
            continue;
        }
        time_t tick;

        char buffer[26];
        struct tm *tm_info;

        tick = time(NULL);
        tm_info = localtime(&tick);

        strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);

        printf("%s server: got connection from %s\n", buffer,
               inet_ntoa(their_addr.sin_addr));

        if (!fork())
        {
            {
                int num = receiveLength(new_fd);
                printf("%d number of arguments is typed by the client.\n", num);
                if (num == 4)
                {
                    char *path = receiveMessage(new_fd);
                    pid_t childpid; /* variriable to store the child's pid */
                    int retval;     /* child process: user-provided return code */
                    int status;     /* parent process: child's exit status */
                    char *binaryPath = "./test";
                    char *arg1 = "haha";
                    char *arg2 = "/home";
                    bool canbeexecute = true;
                    printf("%s :Attempting to execute %s\n", buffer, path);

                    childpid = fork();

                    //  printf("\nchild pid is: %d\n", childpid);
                    if (childpid < 0)
                    {
                        perror("fork");
                    }
                    else if (childpid == 0) /* fork() returns 0 to the child process */
                    {

                        tick = time(NULL);
                        tm_info = localtime(&tick);

                        strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
                        printf("%s - %s has been executed with pid %d\n", buffer, path, childpid);
                        system(path);
                        wait(&status);
                        tick = time(NULL);
                        tm_info = localtime(&tick);
                        strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
                        printf("%s - %d has terminated with status code %d\n", buffer, childpid, WEXITSTATUS(status));
                    }
                    else
                    {
                        exit(0);
                    }
                }

                if (num >= 6)
                {
                    char *msg1 = receiveMessage(new_fd);
                    //   printf("msg1: %s\n", msg1);
                    char *msg2 = receiveMessage(new_fd);
                    char *msg3 = receiveMessage(new_fd);

                    if (num == 6)
                    //./client loc 200 -log logfilename filename
                    //./client loc 200 -o outputfile filename
                    {
                        printf("\n%s,%s,%s\n", msg1, msg2, msg3);

                        if (!strcmp(msg1, "-o"))
                        {
                            pid_t childpid; /* variriable to store the child's pid */
                            int retval;     /* child process: user-provided return code */
                            int status;     /* parent process: child's exit status */

                            char *binaryPath = "./test";
                            char *arg1 = "haha";
                            char *arg2 = "/home";
                            bool canbeexecute = true;

                            char buffer1[26];
                            time_t tick1 = time(NULL);
                            tm_info = localtime(&tick1);
                            ;
                            strftime(buffer1, 26, "%Y-%m-%d %H:%M:%S", tm_info);

                            printf("%s :Attempting to execute %s with argument %s.\n", buffer1, msg1, msg2);

                            childpid = fork();

                            //  printf("\nchild pid is: %d\n", childpid);
                            if (childpid < 0)
                            {
                                perror("fork");
                            }
                            else if (childpid == 0) /* fork() returns 0 to the child process */
                            {
                                oCommand(num, NULL, msg2, msg3);
                            }
                            else
                            {
                                wait(&status);
                                if (canbeexecute)
                                {
                                    char buffer2[26];
                                    time_t tick2 = time(NULL);
                                    tm_info = localtime(&tick2);
                                    strftime(buffer2, 26, "%Y-%m-%d %H:%M:%S", tm_info);

                                    printf("%s - %s has been executed with pid %d\n", buffer2, msg1, childpid);
                                    printf("%s - %d has terminated with status code %d\n", buffer2, childpid, WEXITSTATUS(status));
                                }
                                exit(0);
                            }
                        }
                        //client loc 200 -o outputfile filename
                        else if (!strcmp(msg1, "-log"))
                        {
                            pid_t childpid; /* variriable to store the child's pid */
                            int retval;     /* child process: user-provided return code */
                            int status;     /* parent process: child's exit status */
                            childpid = fork();

                            //  printf("\nchild pid is: %d\n", childpid);
                            if (childpid < 0)
                            {
                                perror("fork");
                            }
                            else if (childpid == 0) /* fork() returns 0 to the child process */
                            {

                                logCommand(num, NULL, msg2, msg3, buffer, tm_info, childpid, tick, status);
                            }
                            else
                            {
                                exit(0);
                            }
                        }
                    }
                    else if (num == 7)
                    //./client loc 200 -log logfilename filename arg
                    //./client loc 200 -o outputfile filename arg
                    {
                        char *msg4 = receiveMessage(new_fd);

                        if (!strcmp(msg1, "-o"))
                        {
                            pid_t childpid; /* variriable to store the child's pid */
                            int retval;     /* child process: user-provided return code */
                            int status;     /* parent process: child's exit status */

                            printf("%s :Attempting to execute %s with argument %s.\n", buffer, msg1, msg2);

                            childpid = fork();
                            //  printf("\nchild pid is: %d\n", childpid);
                            if (childpid < 0)
                            {
                                perror("fork");
                            }
                            else if (childpid == 0) /* fork() returns 0 to the child process */
                            {
                                oCommand(num, msg4, msg2, msg3);
                            }
                            else
                            {
                                wait(&status);
                                printf("%s - %s has been executed with pid %d\n", buffer, msg1, childpid);
                                printf("%s - %d has terminated with status code %d\n", buffer, childpid, WEXITSTATUS(status));
                                exit(0);
                            }
                        }
                        //./client loc 200 -log logfilename filename arg
                        else if (!strcmp(msg1, "-log"))
                        {

                            pid_t childpid; /* variriable to store the child's pid */
                            int retval;     /* child process: user-provided return code */
                            int status;     /* parent process: child's exit status */

                            childpid = fork();
                            //  printf("\nchild pid is: %d\n", childpid)  ;
                            if (childpid < 0)
                            {
                                perror("fork");
                            }
                            else if (childpid == 0) /* fork() returns 0 to the child process */
                            {

                                logCommand(num, msg4, msg2, msg3, buffer, tm_info, childpid, tick, status);
                            }

                            else
                            {

                                exit(0);
                            }
                        }
                    }
                    else if (num == 8)
                    //./client loc 200 -o o.txt -log logfilename filename

                    {
                        char *msg4 = receiveMessage(new_fd);
                        char *msg5 = receiveMessage(new_fd);

                        if (!strcmp(msg1, "-o"))
                        {
                            if (!strcmp(msg3, "-log"))
                            {
                                pid_t childpid; /* variriable to store the child's pid */
                                int retval;     /* child process: user-provided return code */
                                int status;     /* parent process: child's exit status */
                                printf("%s :Attempting to execute %s with argument %s.\n", buffer, msg1, msg2);

                                childpid = fork();

                                //  printf("\nchild pid is: %d\n", childpid);
                                if (childpid < 0)
                                {
                                    perror("fork");
                                }
                                else if (childpid == 0) /* fork() returns 0 to the child process */
                                {
                                    oCommand(num, NULL, msg2, msg3);
                                    logCommand(num, NULL, msg4, msg5, buffer, tm_info, childpid, tick, status);
                                }
                                else
                                {
                                    wait(&status);
                                    printf("%s - %s has been executed with pid %d\n", buffer, msg1, childpid);
                                    printf("%s - %d has terminated with status code %d\n", buffer, childpid, WEXITSTATUS(status));
                                    exit(0);
                                }
                            }
                        }
                    }
                    else if (num == 9)
                    //./client loc 200 -o o.txt -log logfilename filename arg
                    {
                        char *msg4 = receiveMessage(new_fd);
                        char *msg5 = receiveMessage(new_fd);
                        char *msg6 = receiveMessage(new_fd);

                        if (!strcmp(msg1, "-o"))
                        {
                            if (!strcmp(msg3, "-log"))
                            {
                                pid_t childpid; /* variriable to store the child's pid */
                                int retval;     /* child process: user-provided return code */
                                int status;     /* parent process: child's exit status */
                                printf("%s :Attempting to execute %s with argument %s.\n", buffer, msg1, msg2);

                                childpid = fork();

                                //  printf("\nchild pid is: %d\n", childpid);
                                if (childpid < 0)
                                {
                                    perror("fork");
                                }
                                else if (childpid == 0) /* fork() returns 0 to the child process */
                                {
                                    oCommand(num, msg6, msg2, msg3);
                                    logCommand(num, msg6, msg2, msg3, buffer, tm_info, childpid, tick, status);
                                }
                                else
                                {
                                    wait(&status);

                                    printf("%s - %s has been executed with pid %d\n", buffer, msg1, childpid);
                                    printf("%s - %d has terminated with status code %d\n", buffer, childpid, WEXITSTATUS(status));

                                    exit(0);
                                }
                            }
                        }
                    }

                    free(msg1);
                    free(msg2);
                    free(msg3);
                }

                //./client loc 200 filename arg..
                if (num == 5)
                {
                    char *msg1 = receiveMessage(new_fd);
                    //   printf("msg1: %s\n", msg1);
                    char *msg2 = receiveMessage(new_fd);
                    //   printf("msg2: %s\n", msg2);

                    //

                    pid_t childpid; /* variriable to store the child's pid */
                    int retval;     /* child process: user-provided return code */
                    int status;     /* parent process: child's exit status */

                    char *binaryPath = "./test";
                    char *arg1 = "haha";
                    char *arg2 = "/home";

                    printf("%s :Attempting to execute %s with argument %s.\n", buffer, msg1, msg2);

                    childpid = fork();

                    //  printf("\nchild pid is: %d\n", childpid);
                    if (childpid < 0)
                    {
                        kill(getpid(), SIGTERM);
                        perror("fork");
                        exit(EXIT_FAILURE);
                    }
                    else if (childpid == 0) /* fork() returns 0 to the child process */
                    {

                        //  kill(getpid(),SIGTERM);
                        // we are the child
                        if (execl(msg1, msg1, msg2, NULL) == -1) //./test arg
                        {
                            printf("%s - could not execute the file '%s' with argument '%s'.\n", buffer, msg1, msg2);
                            exit(retval);
                        }
                    }
                    else
                    {

                        // wait(&status);
                        printf("%s - %s has been executed with pid %d\n", buffer, msg1, childpid);
                        sleep(10);
                        printf("%s - %d has terminated with status code %d\n", buffer, childpid, WEXITSTATUS(status));
                        kill(getpid(), SIGTERM);
                        // kill(0,SIGTERM);
                    }
                    //
                    free(msg1);
                    free(msg2);
                }

                // if (send(new_fd, "All of array data received by server\n", 40, 0) == -1)
                //     perror("send");

                close(new_fd);
                exit(0);
            }
        }
        else
        {
            close(new_fd); /* parent doesn't need this */
        }
        while (waitpid(-1, NULL, WNOHANG) > 0)
            ; /* clean up child processes */
    }
}

int main(int argc, char *argv[])
{
    printf("server starts listening ...\n");

    argc1 = argc;
    for (int i = 0; i < argc; i++)
    {
        argv1[i] = argv[i];
    }

    int i;                                    /* loop counter          */
    int thr_id[NUM_HANDLER_THREADS];          /* thread IDs            */
    pthread_t p_threads[NUM_HANDLER_THREADS]; /* thread's structures   */
    struct timespec delay;                    /* used for wasting time */

    pthread_mutex_init(&request_mutex, NULL);
    pthread_mutex_init(&quit_mutex, NULL);
    pthread_cond_init(&got_request, NULL);

    /* create the request-handling threads */
    for (i = 0; i < NUM_HANDLER_THREADS; i++)
    {
        thr_id[i] = i;
        pthread_create(&p_threads[i], NULL, handle_requests_loop, (void *)&thr_id[i]);
    }

    /* run a loop that generates requests */
    for (i = 0; i < 600; i++)
    {
        add_request(test_func, NULL, &request_mutex, &got_request);
        /* pause execution for a little bit, to allow      */
        /* other threads to run and handle some requests.  */
        if (rand() > 3 * (RAND_MAX / 4))
        { /* this is done about 25% of the time */
            delay.tv_sec = 0;
            delay.tv_nsec = 10;
            nanosleep(&delay, NULL);
        }
    }
    pthread_mutex_lock(&quit_mutex);
    quit = 1;
    pthread_mutex_unlock(&quit_mutex);

    pthread_cond_broadcast(&got_request);

    for (i = 0; i < NUM_HANDLER_THREADS; i++)
    {
        pthread_join(p_threads[i], NULL);
    }

    return 0;
}

void oCommand(int argc, char *arg, char *destFile, char *filepath)
{
    char cmd[500] = "";

    char *sign = ">";

    strcat(cmd, filepath);
    strcat(cmd, " ");
    strcat(cmd, sign);
    strcat(cmd, " ");
    strcat(cmd, destFile);
    printf("this is to be sent: %s", cmd);
    if (argc == 6)
    {
        system(cmd);
    }
    else if (argc == 7)
    {
        char cmd[100] = "";
        strcat(cmd, filepath);
        strcat(cmd, " ");
        strcat(cmd, arg);
        system(cmd);
    }
}

void logCommand(int argc, char *arg, char *destFile, char *filepath, char *buffer, struct tm *tm_info, pid_t pid, time_t tick, int status)
{

    char clear[50] = "> ";
    strcat(clear, destFile);
    system(clear);

    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    char sen1[100] = "";
    char *str1_1 = " :Attempting to execute ";
    char *str1_2 = " with argument ";
    char *str1_3 = "\n";
    strcat(sen1, buffer);
    strcat(sen1, str1_1);
    strcat(sen1, "-log");
    strcat(sen1, str1_2);
    strcat(sen1, filepath);
    strcat(sen1, str1_3);
    echo(sen1, destFile);

    tick = time(NULL);
    tm_info = localtime(&tick);
    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    char sen2[100] = "";
    strcat(sen2, buffer);
    char *str2_1 = " - ";
    strcat(sen2, str2_1);
    strcat(sen2, filepath);
    strcat(sen2, " has been executed with pid ");
    char *pidStr = malloc(1000);
    sprintf(pidStr, "%i", pid);
    strcat(sen2, pidStr);
    strcat(sen2, "\n");

    //Echo the "has been executed" line
    echo(sen2, destFile);
    //run the code
    if (argc == 6 | argc == 8)
    {
        system(filepath);
    }
    else if (argc == 7 | argc == 9)
    {
        char cmd[100] = "";
        strcat(cmd, filepath);
        strcat(cmd, " ");
        strcat(cmd, arg);
        system(cmd);
    }
    tick = time(NULL);
    tm_info = localtime(&tick);
    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    wait(&status);
    char sen3[100] = "";
    strcat(sen3, buffer);
    strcat(sen3, " - ");
    strcat(sen3, pidStr);
    strcat(sen3, " has terminated with the status code ");

    char *statusStr = malloc(1000);
    sprintf(statusStr, "%d", WEXITSTATUS(status));
    strcat(sen3, statusStr);

    //Echo the " has terminated with the status code" line
    echo(sen3, destFile);
}

void echo(char *str, char *destFile)
{
    char echo[100] = "echo \"";
    strcat(echo, str);
    strcat(echo, "\" >> ");
    strcat(echo, destFile);
    printf("s", echo);
    system(echo);
}
