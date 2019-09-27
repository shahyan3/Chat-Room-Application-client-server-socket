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

#define MAX_MESSAGE_LENGTH 1024

/* COMMANDS */
#define SUB 0
#define UNSUB 1
#define NEXT 2
#define LIVEFEED 3
#define SEND 4

/*
    STRUCTS
*/

typedef struct request request_t;

typedef struct message message_t;

struct message
{
    int ownerID;
    char content[MAX_MESSAGE_LENGTH];
    message_t *next;
};
struct request
{
    int commandID; /* integer enum representing command i.e. SUB, NEXT*/
    int channelID;
    int clientID;
    message_t client_message;
};

/*
    FUNCTIONS
*/
void display_options();

void display_menu();

int sendRequest(request_t clientRequest, int sock_id);

request_t createRequest(int command, int channel_id, int clientID, message_t *message);

int connection_success(int sock_id);

int parseUserMessage(message_t *client_message, char *user_msg_ptr, int clientID);

int main(int argc, char *argv[])
{

    int sockfd, i = 0;
    int *client;
    request_t *clientRequest;
    message_t client_message;

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
    int clientID = connection_success(sockfd);
    // printf("checking client id %d", clientID);

    int id_inputted = RESET_TO_ZERO;

    int *id_ptr = &id_inputted; /* Pointer to user inputted channel id*/

    char command_input[10]; /* User inputted command i.e. SUB, NEXT etc. */
    char *command_input_ptr = command_input;

    char user_input[20]; /* user input */
    char *user_input_ptr = user_input;

    // char message[MAX_MESSAGE_LENGTH];
    char *message = (char *)malloc(sizeof(char) * MAX_MESSAGE_LENGTH);

    // char *user_msg_ptr = message;

    display_menu();

    while (fgets(user_input, 20, stdin))
    {
        sscanf(user_input, "%s%d", command_input, id_ptr);

        // User enters OPTION:
        if (strncmp(command_input, "OPTIONS", strlen("OPTIONS")) == 0)
        {
            display_options();
            printf("\nType Command: ");
        }
        else if (strncmp(command_input, "SUB", strlen("SUB")) == 0 &&
                 id_inputted >= MIN_CHANNELS && id_inputted <= MAX_CHANNELS)
        { // User enters SUB <channel id>
            printf("subccc");
            request_t request = createRequest(SUB, id_inputted, clientID, NULL);

            if (sendRequest(request, sockfd) == 1)
            {
                printf("Client: Error, subscribe request to server failed\n");
            }
            else
            {
                printf("\nClient: Successfully send subscribe request to server...\n");
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

        else if (strncmp(command_input, "SEND", strlen("SEND")) == 0 &&
                 id_inputted >= MIN_CHANNELS && id_inputted <= MAX_CHANNELS)
        { // User enters SEND<channelid><message>
            // printf("\n%s  %d \n%s\n", command_input, id_inputted);

            // Ask for message input.
            printf("Enter Your Message: ");
            fgets(message, MAX_MESSAGE_LENGTH, stdin);
            // printf("\nMessage by user:%s\n", message);

            // Create a message obect

            if (parseUserMessage(&client_message, message, clientID) == 1)
            {
                perror("CLIENT: Parse Error: Invalid or no message found.\n");
                printf("CLIENT: no message found\n");
            }
            else
            {
                request_t request = createRequest(SEND, id_inputted, clientID, &client_message);

                if (sendRequest(request, sockfd) == 1)
                {
                    printf("Client: Error, send message request to server failed\n");
                }
                else
                {
                    printf("\nClient: Successfully sent message request to server...\n");
                }
            }

            printf("\nType Command: ");
        }
        else
        {
            printf("\nErr! Incorrect Input. Try again\n");
            id_inputted = RESET_TO_ZERO;

            printf("\nType Command: ");
        }
    }

    close(sockfd);

    return 0;
}

int connection_success(int sock_id) /* READ UP NETWORK TO BYTE CONVERSION NEEDED? */
{
    // char buff[SERVER_ON_CONNECT_MSG_SIZE];
    int clientID;

    int number_of_bytes;
    if ((number_of_bytes = recv(sock_id, &clientID, sizeof(int), 0)) == RETURNED_ERROR)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }

    printf("Welcome! Your client ID is %d\n", clientID);

    return clientID;
}

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

// Creates a request object and checks if message included or not
request_t createRequest(int command, int channel_id, int clientID, message_t *message)
{

    request_t clientRequest;

    clientRequest.commandID = command;
    clientRequest.channelID = channel_id;
    clientRequest.clientID = clientID;
    if (message != NULL)
    {
        clientRequest.client_message = *message;
    }

    printf("\n Client Request Object:\n command id %d\n channel id %d \n client id %d\n client message %s\n",
           clientRequest.commandID, clientRequest.channelID, clientRequest.clientID, clientRequest.client_message.content);

    return clientRequest;
}

// Client requests to subscribe to channel
int sendRequest(request_t clientRequest, int sock_id)
{

    // send request to server
    if (send(sock_id, &clientRequest, sizeof(struct request), 0) == -1)
    {
        perror("CLIENT: request to server failed [commandInput]...\n");
        return 1;
    }

    return 0; // requests successful.
}
int parseUserMessage(message_t *client_message, char *message, int clientID)
{
    printf("parsing...\n");
    // If message inputted, assign to the request struct
    if (sizeof(*message) <= MAX_MESSAGE_LENGTH && &message[0] != NULL)
    {
        strncpy(client_message->content, message, MAX_MESSAGE_LENGTH);
        client_message->ownerID = clientID;
        client_message->next = NULL;

        // printf("\nTESTING: content %s  id %d\n", client_message->content, client_message->ownerID);

        return 0;
    }
    else
    {
        return 1;
    }
}
