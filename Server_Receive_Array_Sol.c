/*
*  Materials downloaded from the web.
*  Collected and modified for teaching purpose only.
*/

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
#include <stdbool.h>

#define ARRAY_SIZE 90 /* Size of array to receive */

#define BACKLOG 10 /* how many pending connections queue will hold */

#define RETURNED_ERROR -1

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

int main(int argc, char *argv[])
{
    int sockfd, new_fd;            /* listen on sock_fd, new connection on new_fd */
    struct sockaddr_in my_addr;    /* my address information */
    struct sockaddr_in their_addr; /* connector's address information */
    socklen_t sin_size;
    int i = 0;

    /* Get port number for server to listen on */
    if (argc != 2)
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
    my_addr.sin_family = AF_INET;            /* host byte order */
    my_addr.sin_port = htons(atoi(argv[1])); /* short, network byte order */
    my_addr.sin_addr.s_addr = INADDR_ANY;    /* auto-fill with my IP */

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

    printf("server starts listening ...\n");

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

                if (num == 4)
                {
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

                            printf("%s :Attempting to execute %s with argument %s.\n", buffer, msg1, msg2);

                            childpid = fork();

                            //  printf("\nchild pid is: %d\n", childpid);
                            if (childpid < 0)
                            {
                                perror("fork");
                            }
                            else if (childpid == 0) /* fork() returns 0 to the child process */
                            {

                                
                                // char *file = ">" + *msg2;
                                // we are the child
                                char string[100] = "";
                                char *string1 = msg3;
                                char *string2 = ">";
                                char *string3 = "THISISBAD.txt";


                                strcat(string,string1);
                                strcat(string, " ");
                                strcat(string,string2);
                                strcat(string, " ");
                                strcat(string, string3);
                                

                                printf("%s", string);
                                system(string);
                            }
                            else
                            {
                                wait(&status);
                                if (canbeexecute)
                                {
                                    printf("%s - %s has been executed with pid %d\n", buffer, msg1, childpid);
                                    printf("%s - %d has terminated with status code %d\n", buffer, childpid, WEXITSTATUS(status));
                                }
                                exit(0);
                            }
                        }
                    }
                    else if (num == 7)
                    //./client loc 200 -log logfilename filename arg
                    //./client loc 200 -o outputfile filename arg
                    {

                        char *msg4 = receiveMessage(new_fd);
                    }
                    else if (num == 8)
                    //./client loc 200 -o o.txt -log logfilename filename

                    {
                        char *msg4 = receiveMessage(new_fd);
                        char *msg5 = receiveMessage(new_fd);
                    }
                    else if (num == 9)
                    //./client loc 200 -o o.txt -log logfilename filename arg
                    {
                        char *msg4 = receiveMessage(new_fd);
                        char *msg5 = receiveMessage(new_fd);
                        char *msg6 = receiveMessage(new_fd);
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
                    bool canbeexecute = true;

                    printf("%s :Attempting to execute %s with argument %s.\n", buffer, msg1, msg2);

                    childpid = fork();

                    //  printf("\nchild pid is: %d\n", childpid);
                    if (childpid < 0)
                    {
                        perror("fork");
                    }
                    else if (childpid == 0) /* fork() returns 0 to the child process */
                    {
                        // we are the child
                        if (execl(msg1, msg1, msg2, NULL) == -1) //./test arg
                        {
                            canbeexecute = false;
                            printf("%s - could not execute the file '%s' with argument '%s'.\n", buffer, msg1, msg2);
                            exit(retval);
                        }
                    }
                    else
                    {
                        wait(&status);
                        if (canbeexecute)
                        {
                            printf("%s - %s has been executed with pid %d\n", buffer, msg1, childpid);
                            printf("%s - %d has terminated with status code %d\n", buffer, childpid, WEXITSTATUS(status));
                        }
                        exit(0);
                    }
                    //
                    free(msg1);
                    free(msg2);
                }

                if (send(new_fd, "All of array data received by server\n", 40, 0) == -1)
                    perror("send");

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

void makeLogFile(char *name)
{
    FILE *fp;
    fp = fopen(name, "w");

    // Generate whatever you want logged here, "data" is just an example
    char *data = "The data to be logged...";

    // This lines writes the info in "data" to the file pointer specified
    fputs(data, fp);

    // Always remember to close your files
    fclose(fp);
}
