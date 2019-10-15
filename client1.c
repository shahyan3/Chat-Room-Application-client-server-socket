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
#define MAX_CHANNELS 4
#define RESET_INPUT -1
#define NO_CHANNEL_ID -1

#define MAXDATASIZE 100 /* max number of bytes we can get at once */

#define ARRAY_SIZE 30

#define MAX_MESSAGE_LENGTH 1024

/* COMMANDS */
#define SUB 0
#define UNSUB 1
#define NEXT_ID 2
#define LIVEFEED_ID 3
#define SEND 4

#define NEXT 5
#define LIVEFEED 6

/* BOOLEAN */
#define LIVEFEED_TRUE 0
#define LIVEFEED_FALSE 1

/*
    STRUCTS
*/

typedef struct request request_t;

typedef struct message message_t;

struct message
{
    int messageID;
    int ownerID;
    char content[MAX_MESSAGE_LENGTH];
    // message_t *next;
};
struct request
{
    int commandID; /* integer enum representing command i.e. SUB, NEXT*/
    int channelID;
    int clientID;
    message_t client_message;
    int liveFeed;
};

typedef struct response response_t;

struct response
{
    int clientID;
    message_t message;
    int error;
    int channel_id;
    int unReadMessagesCount;
};

/*
    GLOBAL 
*/

int msgSentCount = 0;
response_t serverResponse;

/*
    FUNCTIONS
*/
void display_options();

void display_menu();

int sendRequest(request_t clientRequest, int sock_id);

request_t createRequest(int command, int channel_id, int clientID, message_t *message, int liveFeedFlag);

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

    int id_inputted = RESET_INPUT;

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

        else if ((strncmp(command_input, "SUB", strlen("SUB")) == 0))
        {
            printf("SUB %d", id_inputted);

            request_t request = createRequest(SUB, id_inputted, clientID, NULL, LIVEFEED_FALSE);

            if (sendRequest(request, sockfd) == 1)
            {
                printf("Client: Error, sub request to server failed\n");
            }
            else
            {
                printf("\nClient: Successfully sub request to server...\n");

                if (recv(sockfd, &serverResponse, sizeof(response_t), 0) == -1)
                {
                    perror("Error! Failed to receive response from server!\n");
                    printf("\n Error! Failed to receive response from server!\n");
                }
                else
                {
                    if (serverResponse.error == 0)
                    {
                        printf("\n===============================================\n");
                        printf("            SERVER RESPONSE (Success!)         \n");
                        printf(" \t\t%s\n", serverResponse.message.content);
                        printf("\n===============================================\n");
                    }
                    else
                    {
                        printf("\n ===============================================\n");
                        printf("|            SERVER RESPONSE (Error)              \n");
                        printf("|%s", serverResponse.message.content);
                        printf("\n ===============================================\n");
                    }
                }
            }

            id_inputted = RESET_INPUT;

            printf("\nType Command: ");
        }
        // else if ((strncmp(command_input, "UNSUB", strlen("UNSUB")) == 0))
        // {
        //     printf("UNSUB %d", id_inputted);

        //     request_t request = createRequest(UNSUB, id_inputted, clientID, NULL, LIVEFEED_FALSE);

        //     if (sendRequest(request, sockfd) == 1)
        //     {
        //         printf("Client: Error, UNSUB request to server failed\n");
        //     }
        //     else
        //     {
        //         // printf("\nClient: Successfully sent UNSUB request to server...\n");

        //         if (recv(sockfd, &serverResponse, sizeof(response_t), 0) == -1)
        //         {
        //             perror("Error! Failed to receive response from server!\n");
        //             printf("\n Error! Failed to receive response from server!\n");
        //         }
        //         else
        //         {
        //             if (serverResponse.error == 0)
        //             {
        //                 printf("\n===============================================\n");
        //                 printf("            SERVER RESPONSE (Success!)         \n");
        //                 printf(" \t\t%s\n", serverResponse.message.content);
        //                 printf("\n===============================================\n");
        //             }
        //             else
        //             {
        //                 printf("\n ===============================================\n");
        //                 printf("|            SERVER RESPONSE (Error)              \n");
        //                 printf("|%s", serverResponse.message.content);
        //                 printf("\n ===============================================\n");
        //             }
        //         }
        //     }

        //     id_inputted = RESET_INPUT;

        //     printf("\nType Command: ");
        // }

        // else if (strncmp(command_input, "NEXT", strlen("NEXT")) == 0)
        // { // // User enters NEXT <channel id>
        //     // printf("\n Next ID %s  %d\n", command_input, id_inputted);

        //     request_t request;

        //     if (id_inputted == NO_CHANNEL_ID)
        //     { // NEXT without id
        //         request = createRequest(NEXT, id_inputted, clientID, NULL, LIVEFEED_FALSE);

        //         if (sendRequest(request, sockfd) == 1)
        //         {
        //             printf("Client: Error, NEXT message request to server failed\n");
        //         }
        //         else
        //         {
        //             printf("\nClient: Successfully sent NEXT message request to server...\n");

        //             // printf("\n 0.0 serverResponse %d\n", serverResponse.unReadMessagesCount);

        //             while (recv(sockfd, &serverResponse, sizeof(response_t), 0))
        //             {

        //                 if (serverResponse.error == 1)
        //                 { // IF USER TYPES "NEXT" WITHOUT SUBSCRIBING TO ANY CHANNEL. Display error.
        //                     printf("\n ===============================================\n");
        //                     printf("|            SERVER RESPONSE (Error)              \n");
        //                     printf("| %s ", serverResponse.message.content);
        //                     printf("\n ===============================================\n");

        //                     break;
        //                 }

        //                 if (serverResponse.unReadMessagesCount >= 1) //Todo : error
        //                 {
        //                     if (serverResponse.error == 0)
        //                     {
        //                         printf("\n===============================================\n");
        //                         printf("            SERVER RESPONSE (Success!)         \n\n");
        //                         printf(" \t\t%d:%s\n", serverResponse.channel_id, serverResponse.message.content);
        //                         printf("===============================================\n");
        //                     }

        //                     if (serverResponse.unReadMessagesCount == 1)
        //                     { // Last unreadCount displayed, break the recieving steam loop
        //                         printf("\nONEEEEEEEEEEEEEEEE\n");
        //                         break;
        //                     }
        //                 }
        //             }
        //         }
        //     }
        //     else
        //     { // NEXT with id
        //         request = createRequest(NEXT_ID, id_inputted, clientID, NULL, LIVEFEED_FALSE);

        //         if (sendRequest(request, sockfd) == 1)
        //         {
        //             printf("Client: Error, NEXT message request to server failed\n");
        //         }
        //         else
        //         {
        //             printf("\nClient: Successfully sent NEXT message request to server...\n");

        //             if (recv(sockfd, &serverResponse, sizeof(response_t), 0))
        //             {

        //                 if (serverResponse.error == 0)
        //                 {
        //                     printf("\n===============================================\n");
        //                     printf("            SERVER RESPONSE (Success!)         \n\n");
        //                     printf(" \t\t%d:%s\n", serverResponse.channel_id, serverResponse.message.content);
        //                     printf("===============================================\n");
        //                 }
        //                 else
        //                 {
        //                     printf("\n ===============================================\n");
        //                     printf("|            SERVER RESPONSE (Error)              \n");
        //                     printf("| %s ", serverResponse.message.content);
        //                     printf("\n ===============================================\n");
        //                 }
        //             }
        //         }
        //     }

        //     id_inputted = RESET_INPUT;

        //     printf("\n => Type Command: ");
        // }

        // TOD: BUG = LIVEFEED 0, LIVEFEED1 Make this true. Fix: Trim the input? regex?
        // else if (strncmp(command_input, "LIVEFEED", strlen("LIVEFEED")) == 0)
        // { // User enters LIVEFEED only
        //     printf("\n CLIENT: LIVE FEED with ID... %s  \n", command_input);

        //     printf("id %d", id_inputted);

        //     request_t request = createRequest(LIVEFEED_ID, id_inputted, clientID, NULL, LIVEFEED_TRUE);

        //     if (sendRequest(request, sockfd) == 1)
        //     {
        //         printf("Client: Error, LIVEFEED with id request to server failed\n");
        //     }
        //     else
        //     {
        //         // printf("\nClient: Successfully sent LIVEFEED_ID request to server...\n");

        //         if (recv(sockfd, &serverResponse, sizeof(response_t), 0) == -1)
        //         {
        //             perror("Error! Failed to receive response from server!\n");
        //             printf("\n Error! Failed to receive response from server!\n");
        //         }
        //         else
        //         {
        //             if (serverResponse.error == 0)
        //             {
        //                 printf("\n===============================================\n");
        //                 printf("            SERVER RESPONSE (Success!)         \n");
        //                 printf(" \t\t%s\n", serverResponse.message.content);
        //                 printf("\n===============================================\n");
        //             }
        //             else
        //             {
        //                 printf("\n ===============================================\n");
        //                 printf("|            SERVER RESPONSE (Error)              \n");
        //                 printf("|%s", serverResponse.message.content);
        //                 printf("\n ===============================================\n");
        //             }
        //         }
        //     }

        //     id_inputted = RESET_INPUT;

        //     printf("\nType Command: ");
        // }

        // else if (strncmp(command_input, "LIVEFEED", strlen("LIVEFEED")) == 0 &&
        //          id_inputted >= MIN_CHANNELS && id_inputted <= MAX_CHANNELS)
        // { // // User enters LIVEFEED <channel id>
        //     // printf("\n WITH ID %s  %d\n", command_input, id_inputted);

        //     id_inputted = RESET_INPUT;

        //     printf("\nType Command: ");
        // }
        // else if (strncmp(command_input, "LIVEFEED", strlen("LIVEFEED")) == 0 &&
        //              id_inputted < MIN_CHANNELS ||
        //          id_inputted > MAX_CHANNELS)
        // {
        //     // printf("\n ERR! Channel Id %d not found... \n", id_inputted);
        //     id_inputted = RESET_INPUT;

        //     printf("\nType Command: ");
        // }

        // else if (strncmp(command_input, "BYE", strlen("BYE")) == 0)
        // { // // User enters BYE
        //     printf("\n %s \n", command_input);
        //     printf("\n BYE BYE! \n");
        //     id_inputted = RESET_INPUT;

        //     sleep(1);
        //     exit(1);
        // }

        else if (strncmp(command_input, "SEND", strlen("SEND")) == 0)
        { // User enters SEND<channelid><message>
            // printf("\n=> %s  %d \n", command_input, id_inputted);

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
                request_t request = createRequest(SEND, id_inputted, clientID, &client_message, LIVEFEED_FALSE);

                if (sendRequest(request, sockfd) == 1)
                {
                    printf("Client: Error, send message request to server failed\n");
                }
                else
                {
                    // printf("\nClient: Successfully sent message request to server...\n");

                    if (recv(sockfd, &serverResponse, sizeof(response_t), 0))
                    {
                        printf("\n ===============================================\n");
                        printf("|            SERVER RESPONSE                      \n");
                        printf("| %s ", serverResponse.message.content);
                        printf("\n ===============================================\n");
                    }
                }
            }

            id_inputted = RESET_INPUT;

            printf("\nType Command: ");
        }
        else
        {
            printf("\nIncorrect Input.\n");
            id_inputted = RESET_INPUT;

            printf("\nType Command: ");
        }
    }

    // close(sockfd);

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
request_t createRequest(int command, int channel_id, int clientID, message_t *message, int liveFeedFlag)
{

    request_t clientRequest;

    clientRequest.commandID = command;
    clientRequest.channelID = channel_id;
    clientRequest.clientID = clientID;
    clientRequest.liveFeed = liveFeedFlag;

    if (message != NULL)
    {
        clientRequest.client_message = *message;
    }

    // printf("\n Client Request Object:\n command id %d\n channel id %d \n client id %d\n client message %s\n",
    //        clientRequest.commandID, clientRequest.channelID, clientRequest.clientID, clientRequest.client_message.content);

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

    // printf("parsing...\n");
    // If message inputted, assign to the request struct
    if (sizeof(*message) <= MAX_MESSAGE_LENGTH && &message[0] != NULL)
    {

        // CRITICAL SECTION msgSendCount should only be incremented one by one by threads ?????********
        strncpy(client_message->content, message, MAX_MESSAGE_LENGTH);
        client_message->ownerID = clientID;

        // CRITICAL SECTION end ********

        // printf("\nTESTING: content %s  id %d\n", client_message->content, client_message->ownerID);

        return 0;
    }
    else
    {
        return 1;
    }
}