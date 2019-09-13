/*
*  Materials downloaded from the web. See relevant web sites listed on OLT
*  Collected and modified for teaching purpose only by Jinglan Zhang, Aug. 2006
*/

#define _XOPEN_SOURCE // sigaction obj needs this to work...man 7 feature_test_macros
#define STDOUT_FILENO 1;

#include <signal.h>

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

#define ARRAY_SIZE 30 /* Size of array to receive */

#define BACKLOG 10 /* how many pending connections queue will hold */

#define RETURNED_ERROR -1

/*
    FUNCTION: deal elegantly with any threads that have been created as well as any open sockets, shared memory regions,
dynamically allocated memory and/or open files. 
s */

void shut_down_handler()
{
    // close threads
    // close processes
    // close sockets
    // close(new_fd);
    // shared memory regions
    // free allocated memory and open files
    sleep(1);
    write(1, "\nShutting down gracefully, BYE!\n", 29);
    exit(1);
}

int *Receive_Array_Int_Data(int socket_identifier, int size)
{
    int number_of_bytes, i = 0;
    uint16_t statistics;

    int *results = malloc(sizeof(int) * size);
    for (i = 0; i < size; i++)
    {
        if ((number_of_bytes = recv(socket_identifier, &statistics, sizeof(uint16_t), 0)) == RETURNED_ERROR)
        {
            perror("recv");
            exit(EXIT_FAILURE);
        }
        results[i] = ntohs(statistics);
    }
    return results;
}

int main(int argc, char *argv[])
{
    int port = 12345;

    // signal handling
    struct sigaction sa;
    sa.sa_handler = shut_down_handler;
    sigaction(SIGINT, &sa, NULL);

    int sockfd, new_fd;            /* listen on sock_fd, new connection on new_fd */
    struct sockaddr_in my_addr;    /* my address information */
    struct sockaddr_in their_addr; /* connector's address information */
    socklen_t sin_size;
    int i = 0;

    /* Get port number for server to listen on */
    if (argc == 2)
    {
        // fprintf(stderr, "usage: client port_number\n");
        // exit(1);
        port = atoi(argv[1]);
    }

    /* generate the socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    /* generate the end point */
    my_addr.sin_family = AF_INET;             /* host byte order */
    my_addr.sin_port = htons(port);           /* short, network byte order */
    my_addr.sin_addr.s_addr = INADDR_ANY;     /* auto-fill with my IP */
    /* bzero(&(my_addr.sin_zero), 8);   ZJL*/ /* zero the rest of the struct */

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

    printf("server starts listening on %d...\n", port);

    /* repeat: accept, send, close the connection */
    while (1)
    { /* main accept() loop */
        sin_size = sizeof(struct sockaddr_in);
        if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr,
                             &sin_size)) == -1)
        {
            perror("accept");
            continue;
        }
        printf("server: got connection from %s\n",
               inet_ntoa(their_addr.sin_addr));

        /* ***Server-Client Connected*** */

        // Send: Welcome Message.
        if (send(new_fd, "Welcome! Your client ID is <client id>.\n", 40, 0) == -1)
            perror("send");

        close(new_fd);
        exit(0);

        // close(new_fd); /* parent doesn't need this */

        // while (waitpid(-1, NULL, WNOHANG) > 0)
        //     ; /* clean up child processes */
    }
}
