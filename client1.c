#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <ctype.h>

#define RETURNED_ERROR -1
#define SERVER_ON_CONNECT_MSG_SIZE 40
#define MIN_CHANNELS 1
#define MAX_CHANNELS 5

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

void display_options()
{
    printf("\nUsage: \n");
    printf("\n SUB <channelid>               :  Subcribe to chanenel \n");
    printf("\n UNSUB <channelid>             :  Unsubcribe to chanenel \n");
    printf("\n NEXT <channelid>              :  Display the next unread message with given ID \n");
    printf("\n LIVEFEED <channelid>          :  Display all unread messages continuously with given ID \n");
    printf("\n NEXT                          :  Display the next unread message from all channels \n");
    printf("\n LIVEFEED <channelid>          :  Display all unread messages continuously from all channels \n");
    printf("\n NEXT <channelid>              :  Display the next unread message with given ID \n");
    printf("\n SEND <channelid> <message>    :  Send a new message to given channel ID \n");
    printf("\n BYE                           :  Shutdown connection \n");
}
void display_menu()
{
    printf("\n See Usage type [OPTIONS]\n");
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

    printf("\n%s", buff);
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

    int id_inputted;
    int *id_ptr = &id_inputted; /* Pointer to user inputted channel id*/

    char command_input[10]; /* User inputted command i.e. SUB, NEXT etc. */

    char user_input[20]; /* user input */

    display_menu();

    while (fgets(user_input, 20, stdin))
    {
        sscanf(user_input, "%s %d", command_input, id_ptr);
        // printf("%s %d\n", command_input, id_inputted);

        // User enters OPTION:
        if (strncmp(command_input, "OPTIONS", strlen("OPTIONS")) == 0)
        {
            display_options();
        }

        // User enters SUB <channel id>
        if (strncmp(command_input, "SUB", strlen("SUB")) == 0 &&
            id_inputted >= MIN_CHANNELS && id_inputted <= MAX_CHANNELS)
        {
            printf("\n%s  %d\n", command_input, id_inputted);
        }

        else
        {
            printf("\nNOPE\n");
        }
    }

    close(sockfd);

    return 0;
}
