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
#include <stdbool.h>

#define RETURNED_ERROR -1
#define SERVER_ON_CONNECT_MSG_SIZE 40
#define MIN_CHANNELS 1
#define MAX_CHANNELS 5
#define RESET_TO_ZERO 0

#define MAXDATASIZE 100 /* max number of bytes we can get at once */

#define ARRAY_SIZE 30

typedef struct server_request
{
    int channelid;
    char command;

} server_request;

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

// Client requests to subscribe to channel
// PARAM: channel id
// RETURN: 0 = success, -1 failed request
int subscribeReq(server_request serverRequest, int channel_id, int sock_id)
{

    if (send(sock_id, &serverRequest, sizeof(server_request), 0) == -1)
    {
        perror("CLIENT: request to server failed [commandInput]...");
        return -1;
    }

    return 1; // requests successful.
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

    server_request serverRequest;

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

    int id_inputted = RESET_TO_ZERO;

    int *id_ptr = &id_inputted; /* Pointer to user inputted channel id*/

    char command_input[10]; /* User inputted command i.e. SUB, NEXT etc. */
    char *command_input_ptr = command_input;

    char user_input[20]; /* user input */
    char *user_input_ptr = user_input;

    display_menu();

    while (fgets(user_input, 20, stdin))
    {
        sscanf(user_input, "%s%d", command_input, id_ptr);

        // for (i = 0; i < strlen(user_input); i++)
        // {
        //     printf("->%c", user_input[i]);
        // }
        // printf("==> %s %d\n", , user_input[1]);

        // User enters OPTION:
        if (strncmp(command_input, "OPTIONS", strlen("OPTIONS")) == 0)
        {
            display_options();
            printf("\nType Command: ");
        }
        else if (strncmp(command_input, "SUB", strlen("SUB")) == 0 &&
                 id_inputted >= MIN_CHANNELS && id_inputted <= MAX_CHANNELS)
        { // User enters SUB <channel id>
            printf("\n%s  %d\n", command_input, id_inputted);

            serverRequest.channelid = id_inputted;
            // serverRequest.command = command_input;

            if (subscribeReq(serverRequest, id_inputted, sockfd) == -1)
            {
                printf("Client: Error, subscribe request to server failed\n");
            }
            else
            {
                printf("Client: Successfully send subscribe request to server...\n");
            }

            id_inputted = RESET_TO_ZERO;

            printf("\nType Command: ");
        }
        else if (strncmp(command_input, "SUB", strlen("SUB")) == 0 &&
                     id_inputted < MIN_CHANNELS ||
                 id_inputted > MAX_CHANNELS)
        {
            printf("\n ERR! Channel Id %d not found... \n", id_inputted);
            id_inputted = RESET_TO_ZERO;

            printf("\nType Command: ");
        }
        else if (strncmp(command_input, "SUB", strlen("SUB")) == 0 &&
                 id_inputted == 0)
        {
            printf("\n ERR! Channel Id %d not found... \n", id_inputted);
            id_inputted = RESET_TO_ZERO;

            printf("\nType Command: ");
        }

        else if (strncmp(command_input, "UNSUB", strlen("UNSUB")) == 0 &&
                 id_inputted >= MIN_CHANNELS && id_inputted <= MAX_CHANNELS)
        { // // User enters UNSUB <channel id>
            printf("\n%s  %d\n", command_input, id_inputted);
            id_inputted = RESET_TO_ZERO;

            printf("\nType Command: ");
        }
        else if (strncmp(command_input, "UNSUB", strlen("UNSUB")) == 0 &&
                     id_inputted < MIN_CHANNELS ||
                 id_inputted > MAX_CHANNELS)
        {
            printf("\n ERR! Channel Id %d not found... \n", id_inputted);
            id_inputted = RESET_TO_ZERO;

            printf("\nType Command: ");
        }
        else if (strncmp(command_input, "UNSUB", strlen("UNSUB")) == 0 &&
                 id_inputted == 0)
        {
            printf("\n ERR! Channel Id %d not found... \n", id_inputted);
            id_inputted = RESET_TO_ZERO;

            printf("\nType Command: ");
        }
        // TOD: BUG = LIVEFEED 0, LIVEFEED1 Make this true. Fix: Trim the input? regex?
        else if (strncmp(command_input, "LIVEFEED", strlen("LIVEFEED")) == 0 && id_inputted == 0)
        { // User enters LIVEFEED only
            printf("\n NO ID, Continuosly show msg from all channels... %s  \n", command_input);
            printf("\nType Command: ");
        }
        else if (strncmp(command_input, "LIVEFEED", strlen("LIVEFEED")) == 0 &&
                 id_inputted >= MIN_CHANNELS && id_inputted <= MAX_CHANNELS)
        { // // User enters LIVEFEED <channel id>
            printf("\n WITH ID %s  %d\n", command_input, id_inputted);
            id_inputted = RESET_TO_ZERO;

            printf("\nType Command: ");
        }
        else if (strncmp(command_input, "LIVEFEED", strlen("LIVEFEED")) == 0 &&
                     id_inputted < MIN_CHANNELS ||
                 id_inputted > MAX_CHANNELS)
        {
            printf("\n ERR! Channel Id %d not found... \n", id_inputted);
            id_inputted = RESET_TO_ZERO;

            printf("\nType Command: ");
        }

        // TOD: BUG = NEXT 0, LIVEFEED1 Make this true. Fix: Trim the input? regex?
        else if (strncmp(command_input, "NEXT", strlen("NEXT")) == 0 && id_inputted == 0)
        { //User enters NEXT only
            printf("\n NO ID - SHOW  *UNREAD* MSG FROM ALL CHANNELS, OK... %s  \n", command_input);
            printf("\nType Command: ");
        }
        else if (strncmp(command_input, "NEXT", strlen("NEXT")) == 0 &&
                 id_inputted >= MIN_CHANNELS && id_inputted <= MAX_CHANNELS)
        { // // User enters NEXT <channel id>
            printf("\n WITH ID %s  %d\n", command_input, id_inputted);
            id_inputted = RESET_TO_ZERO;

            printf("\nType Command: ");
        }
        else if (strncmp(command_input, "NEXT", strlen("NEXT")) == 0 &&
                     id_inputted < MIN_CHANNELS ||
                 id_inputted > MAX_CHANNELS)
        {
            printf("\n ERR! Channel Id %d not found... \n", id_inputted);
            id_inputted = RESET_TO_ZERO;

            printf("\nType Command: ");
        }

        else if (strncmp(command_input, "BYE", strlen("BYE")) == 0)
        { // // User enters BYE
            printf("\n %s \n", command_input);
            printf("\n BYE BYE! \n");
            id_inputted = RESET_TO_ZERO;

            sleep(1);
            exit(1);
        }
        else
        {
            printf("\nErr! Incorrect Input. Try again\n");
            id_inputted = RESET_TO_ZERO;

            printf("\nType Command: ");
        }

        // User enters SEND <channelid> <message>
        // if (strncmp(command_input, "SEND", strlen("SEND")) == 0 &&
        //     id_inputted >= MIN_CHANNELS && id_inputted <= MAX_CHANNELS)
        // {
        // correct_input = 1;
        //     printf("\n%s  %d\n", command_input, id_inputted);
        // id_inputted = -1;
        //        printf("\nType Command: ");

        // }
    }

    close(sockfd);

    return 0;
}
