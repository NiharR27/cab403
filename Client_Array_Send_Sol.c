#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAXDATASIZE 100 /* max number of bytes we can get at once */

#define ARRAY_SIZE 30


void sendMessage(int socket_id, char *msg) {
    int len = strlen(msg);
    int netLen = htonl(len);
    //send the string length
    send(socket_id,&netLen,sizeof(netLen), 0);
    //send the actual message
    if(send(socket_id,msg,len, 0) != len) {
        fprintf(stderr,"send did not send all");
        exit(1);
    }
}

void sendLength(int socket_id,int length)
{
    int num = htonl(length);
   send(socket_id,&num,sizeof(num),0); 
}


int main(int argc, char *argv[])
{
    int sockfd, numbytes, i = 0;
    char buf[MAXDATASIZE];
    struct hostent *he;
    struct sockaddr_in their_addr; /* connector's address information */

    if (argc < 3)
    {
        fprintf(stderr, "usage: controller <address> <port> {[-o out_file] [-log log_file] [-t seconds] <file> [arg...] | mem [pid] | memkill <percent>} \n");
        
        exit(1);
    }
    
    if(argc > 3) {
        for(int i =3; i < argc; i++) {
            printf("%s\n",argv[i]);
        }
    }

    if ((he = gethostbyname(argv[1])) == NULL)
    { /* get the host info */
        herror("gethostbyname");
        exit(1);
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    /* clear address struct */
    memset(&their_addr, 0, sizeof(their_addr));

    their_addr.sin_family = AF_INET;            /* host byte order */
    their_addr.sin_port = htons(atoi(argv[2])); /* short, network byte order */
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);

    if (connect(sockfd, (struct sockaddr *)&their_addr,
                sizeof(struct sockaddr)) == -1)
    {
        printf("Couldn't connect to Server at %s %s.\n",argv[1],argv[2]);
        exit(1);
    }


    sendLength(sockfd,argc);


    if(argc == 5) 
    {
        printf("5 arguments passed");
        sendMessage(sockfd, argv[3]);
        sendMessage(sockfd, argv[4]);

       // Send_Array_Data(sockfd, argc, argv);
    }

    

    /* Receive message back from server */
    if ((numbytes = recv(sockfd, buf, MAXDATASIZE, 0)) == -1)
    {
        perror("recv");
        exit(1);
    }

    buf[numbytes] = '\0';

    buf[numbytes] = '\0';

    printf("Received: %s", buf);

    close(sockfd);

    return 0;
}
