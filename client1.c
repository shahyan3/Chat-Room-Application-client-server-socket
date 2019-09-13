#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#define RETURNED_ERROR -1
#define SERVER_ON_CONNECT_MSG_SIZE 40

#define MAXDATASIZE 100 /* max number of bytes we can get at once */

#define ARRAY_SIZE 30

// void Send_Array_Data(int socket_id, int *myArray)
// {
//     int i = 0;
//     uint16_t statistics;
//     for (i = 0; i < ARRAY_SIZE; i++)
//     {
//         statistics = htons(myArray[i]);
//         send(socket_id, &statistics, sizeof(uint16_t), 0);
//     }
// }

// convert into function 0 - shows commands and nums
void display_menu()
{
    printf("\nChoose Your Options <0>\n");
    printf("\nType Command: ");
}

int connection_success(int sock_id) /* READ UP NETWORK TO BYTE CONVERSION NEEDED? */
{
    char buff[SERVER_ON_CONNECT_MSG_SIZE];

    int number_of_bytes;
    if ((number_of_bytes = recv(sock_id, buff, sizeof(char) * SERVER_ON_CONNECT_MSG_SIZE, 0)) == RETURNED_ERROR)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }

    printf("%s", buff);
}

int main(int argc, char *argv[])
{
    int sockfd, i = 0;
    struct hostent *he;
    struct sockaddr_in their_addr; /* connector's address information */

    if (argc != 3)
    {
        fprintf(stderr, "usage: client_hostname port_number\n");
        exit(1);
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

    their_addr.sin_family = AF_INET;            /* host byte order */
    their_addr.sin_port = htons(atoi(argv[2])); /* short, network byte order */
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    bzero(&(their_addr.sin_zero), 8); /* zero the rest of the struct */

    if (connect(sockfd, (struct sockaddr *)&their_addr,
                sizeof(struct sockaddr)) == -1)
    {
        perror("connect");
        exit(1);
    }

    /* **!**CLIENT-SERVER***!** */

    /* Receive message back from server */
    connection_success(sockfd);

    int sub;
    int *sub_ptr = &sub;

    display_menu();

    while (1)
    {
        // scan for input from client for next, live, blah
        scanf("%d", sub_ptr);

        switch (*sub_ptr)
        {
        case 1:
            /* code */
            // NEXT_FUNC_STUFF()
            printf("\nType Command: ");
            break;
        case 2:
            /* code */
            // LIVE_FEED_STUFF()
            printf("\nType Command: ");

            break;
        case 3:
            /* code */
            // printf("You choose 3\n");
            printf("\nType Command: ");

            break;
        case 4:
            /* code */
            // printf("You choose 4\n");
            printf("\nType Command: ");

            break;

        default:
            break;
        }
    }

    close(sockfd);

    return 0;
}
