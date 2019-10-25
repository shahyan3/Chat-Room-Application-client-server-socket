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
#include <pthread.h> /* pthread */

#include "client_functions.h"

#define _XOPEN_SOURCE // sigaction obj needs this to work...man 7 feature_test_macros

#include <signal.h>

#define RETURNED_ERROR -1
#define SERVER_ON_CONNECT_MSG_SIZE 40
#define MIN_CHANNELS 1
#define MAX_CHANNELS 4
#define RESET_INPUT -1
#define NO_CHANNEL_ID -1

#define MAXDATASIZE 100 /* max number of bytes we can get at once */

#define ARRAY_SIZE 30

// #define MAX_MESSAGE_LENGTH 1024

/* COMMANDS */
#define SUB 0
#define UNSUB 1
#define NEXT_ID 2
#define LIVEFEED_ID 3
#define SEND 4

#define NEXT 5
#define LIVEFEED 6

#define BYE 100

/* BOOLEAN */
#define LIVEFEED_TRUE 0
#define LIVEFEED_FALSE 1

/*
    GLOBAL 
*/
response_t serverResponse;

void shut_down_handler()
{
    printf("\n\nBYE! Closing application now.... \n");

    sleep(1);
    exit(1);
}

int main(int argc, char *argv[])
{
    // Threads to be used for Next/Livefeed
    pthread_t thread1;
    pthread_t thread2;

    thdata data1;

    // signal handling  // TODO: ctlr c should shutdown gracefully !!!!
    struct sigaction sa;
    sa.sa_handler = shut_down_handler;
    sigaction(SIGINT, &sa, NULL);

    int sockfd = 0;

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

    int id_inputted = RESET_INPUT;

    int *id_ptr = &id_inputted; /* Pointer to user inputted channel id*/

    char command_input[10]; /* User inputted command i.e. SUB, NEXT etc. */

    char user_input[20]; /* user input */

    char *message = (char *)malloc(sizeof(char) * MAX_MESSAGE_LENGTH);

    display_menu(); // Client menu

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

            request_t request = createRequest(SUB, id_inputted, clientID, NULL, LIVEFEED_FALSE);

            if (sendRequest(request, sockfd) == 1)
            {
                printf("Client: Error, sub request to server failed\n");
            }
            else
            {

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
        else if ((strncmp(command_input, "UNSUB", strlen("UNSUB")) == 0))
        {

            request_t request = createRequest(UNSUB, id_inputted, clientID, NULL, LIVEFEED_FALSE);

            if (sendRequest(request, sockfd) == 1)
            {
                printf("Client: Error, UNSUB request to server failed\n");
            }
            else
            {

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

        else if (strncmp(command_input, "NEXT", strlen("NEXT")) == 0)
        { // // User enters NEXT <channel id>

            request_t request;

            if (id_inputted == NO_CHANNEL_ID)
            { // NEXT without id

                data1.clientID = clientID; /* threads data */
                data1.channel_id = id_inputted;
                data1.sockfd = sockfd;

                // Thread1: handles sending NEXT request to server
                pthread_create(&thread1, NULL, (void *(*)(void *))handleNextSendClient, (void *)&data1);

                pthread_attr_t attr;
                pthread_attr_init(&attr);
                // Thread2: handles receiving responses from server
                pthread_create(&thread2, &attr, (void *)(void *)handleNextReceiveClient, (void *)&data1);

                pthread_join(thread1, NULL);
                pthread_join(thread2, NULL);
            }
            else
            { // NEXT with id
                request = createRequest(NEXT_ID, id_inputted, clientID, NULL, LIVEFEED_FALSE);

                if (sendRequest(request, sockfd) == 1)
                {
                    printf("Client: Error, NEXT message request to server failed\n");
                }
                else
                {
                    data1.clientID = clientID; /* threads data */
                    data1.channel_id = id_inputted;
                    data1.sockfd = sockfd;

                    pthread_attr_t attr;
                    pthread_attr_init(&attr);
                    // Thread: handles receiving responses from server
                    pthread_create(&thread2, &attr, (void *)(void *)handleMessageReceiveClient, (void *)&data1);

                    pthread_join(thread2, NULL);
                }
            }

            id_inputted = RESET_INPUT;

            printf("\n Type Command: ");
        }
        // Livefeed
        else if (strncmp(command_input, "LIVEFEED", strlen("LIVEFEED")) == 0)
        { // LIVEFEED

            if (id_inputted == NO_CHANNEL_ID)
            { // LIVEFEED no ID
                printf("\n Livefeed on...\n");

                while (1)
                {
                    request_t request = createRequest(LIVEFEED, id_inputted, clientID, NULL, LIVEFEED_TRUE);

                    if (sendRequest(request, sockfd) == 1)
                    {
                        printf("Client: Error, LIVEFEED with id request to server failed\n");
                    }
                    else
                    {
                        data1.clientID = clientID; /* threads data */
                        data1.channel_id = id_inputted;
                        data1.sockfd = sockfd;

                        pthread_attr_t attr;
                        pthread_attr_init(&attr);
                        // Thread: handles receiving responses from server
                        pthread_create(&thread2, &attr, (void *)(void *)handleMessageReceiveClient, (void *)&data1);

                        pthread_join(thread2, NULL);

                        id_inputted = RESET_INPUT;

                        printf("\nType Command: ");
                    }

                    sleep(1);
                }
            }
            else
            { // Livefeed with ID
                printf("\n Livefeed on...\n");

                request_t request = createRequest(LIVEFEED_ID, id_inputted, clientID, NULL, LIVEFEED_TRUE);

                if (sendRequest(request, sockfd) == 1)
                {
                    printf("Client: Error, LIVEFEED with id request to server failed\n");
                }
                else
                {
                    while (1)
                    {

                        data1.clientID = clientID; /* threads data */
                        data1.channel_id = id_inputted;
                        data1.sockfd = sockfd;

                        pthread_attr_t attr;
                        pthread_attr_init(&attr);
                        // Thread: handles receiving responses from server
                        pthread_create(&thread2, &attr, (void *)(void *)handleMessageReceiveClient, (void *)&data1);

                        pthread_join(thread2, NULL);

                        id_inputted = RESET_INPUT;

                        printf("\nType Command: ");

                        sleep(1);
                    }
                }
            }

            id_inputted = RESET_INPUT;

            printf("\nType Command: ");
        }

        else if (strncmp(command_input, "SEND", strlen("SEND")) == 0)
        { // User enters SEND<channelid><message>

            // Ask for message input.
            printf("Enter Your Message: ");
            fgets(message, MAX_MESSAGE_LENGTH, stdin);

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
        else if (strncmp(command_input, "BYE", strlen("BYE")) == 0)
        { // // User enters BYE

            bye(BYE, id_inputted, clientID, client_message, LIVEFEED_FALSE, sockfd);
        }
        else
        {
            printf("\nIncorrect Input.\n");
            id_inputted = RESET_INPUT;

            printf("\nType Command: ");
        }
    }

    return 0;
}

/***********************************************************************
 * func:            Thread function used to handle incoming response 
 *                  messages from server. Generic implementation used for 
 *                  LIVEFEED
 *                 
 * param ptr:      void pointer that points to thdata struct to access 
 *                  socket fd variable
***********************************************************************/
void handleMessageReceiveClient(void *ptr)
{
    thdata *data;
    data = (thdata *)ptr; /* type cast to a pointer to thdata */

    if (recv(data->sockfd, &serverResponse, sizeof(response_t), 0))
    {

        if (serverResponse.error == 1)
        { // IF USER TYPES "NEXT" WITHOUT SUBSCRIBING TO ANY CHANNEL. Display error.
            printf("\n ===============================================\n");
            printf("|            SERVER RESPONSE (Error)              \n");
            printf("| %s ", serverResponse.message.content);
            printf("\n ===============================================\n");

            return;
        }

        if (serverResponse.error == 0)
        {
            printf("\n===============================================\n");
            printf("            SERVER RESPONSE (Success!)         \n\n");
            printf(" \t\t%d:%s\n", serverResponse.channel_id, serverResponse.message.content);
            printf("===============================================\n");
        }

        if (serverResponse.unReadMessagesCount == 1)
        { // Last unreadCount displayed, break the recieving steam loop

            return;
        }
    }
}

/***********************************************************************
 * func:            Thread function used to handle incoming response 
 *                  messages from server. This function receives multiple
 *                  incoming data stream in a while loop from sever; used for NEXT command
 *                  specifically.
 *                 
 * param ptr:      void pointer that points to thdata struct to access 
 *                 socket fd variable
***********************************************************************/
void handleNextReceiveClient(void *ptr)
{

    thdata *data;
    data = (thdata *)ptr; /* type cast to a pointer to thdata */

    while (recv(data->sockfd, &serverResponse, sizeof(response_t), 0) >= 1)
    {

        if (serverResponse.error == 1)
        { // Display error.
            printf("\n ===============================================\n");
            printf("|            SERVER RESPONSE (Error)              \n");
            printf("| %s ", serverResponse.message.content);
            printf("\n ===============================================\n");

            return;
        }

        if (serverResponse.error == 0)
        {
            printf("\n===============================================\n");
            printf("            SERVER RESPONSE (Success!)         \n\n");
            printf(" \t\t%d:%s\n", serverResponse.channel_id, serverResponse.message.content);
            printf("===============================================\n");
        }

        if (serverResponse.unReadMessagesCount == 1)
        { // Last unreadCount displayed, break the recieving steam loop

            return;
        }
    }
    if (recv(data->sockfd, &serverResponse, sizeof(response_t), 0) == 0)
    { // end of recv stream
        return;
    }
}
/***********************************************************************
 * func:            Thread function used to send request to server for 
 *                  NEXT command specifically. 
 *                 
 * param ptr:      void pointer that points to thdata struct to access 
 *                  channnel id and client id
***********************************************************************/
void handleNextSendClient(void *ptr)
{

    thdata *data;
    data = (thdata *)ptr; /* type cast to a pointer to thdata */

    request_t request = createRequest(NEXT, data->channel_id, data->clientID, NULL, LIVEFEED_FALSE);

    if (sendRequest(request, data->sockfd) == 1)
    {
        printf("Client: Error, NEXT message request to server failed\n");
    }
    else
    {
        // printf("\nClient: Successfully sent NEXT message request to server...\n");
    }
}

/**************************************************************************************
 * func:                    Handles gracefully shutting down the client server
 *                          by notifying the server with a request and shutting down.    
 *                 
 * param userCommand:       The numerical value of the userCommand i.e. NEXT, SUB etc.
 * 
 * param id_inputted:       Channel id inputted by user            
 *  
 * 
 * param clientID:          Client id of the given user
 * 
 * param client_message:    Client message struct to send server before shutting down      
 * 
 * param liveFeedFlag:      Livefeed boolean flag i.e. is livefeed on / off
 * 
 * param sockfd:            Socket file descripter
****************************************************************************************/
void bye(int userCommand, int id_inputted, int clientID, message_t client_message, int liveFeedFlag, int sockfd)
{
    request_t request = createRequest(BYE, id_inputted, clientID, &client_message, liveFeedFlag);

    if (sendRequest(request, sockfd) == 1)
    {
        printf("Client: Error, send Bye request to server failed\n");
    }
    else
    { // successful send client request. Gracefully shutdown.

        printf("\n\nBYE! Closing application now.... \n");

        id_inputted = RESET_INPUT;

        close(sockfd);
        sleep(1);
        exit(1);
    }
}

/************************************************************************************************
 * func:            Upon successfully connecting to the server receives 
 *                  returned client id (i.e.process id) which this function displays in client UI 
 *                 
 * param sock_id:   Socket file descripter
**************************************************************************************************/
int connection_success(int sock_id)
{
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

/************************************************************************************************
 * func:            Prints user commands and their descriptions to the screen
 * 
 *                 
**************************************************************************************************/
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

/************************************************************************************************
 * func:            Prints instructions to access OPTIONS 
 * 
 *                 
**************************************************************************************************/
void display_menu()
{
    printf("\n See Usage type [OPTIONS]\n");
    printf("\nType Command: ");
}

/************************************************************************************************
 * func:               Creates a request object for sending to the server
 *                   
 *                 
 * param command:      Numerical value of NEXT, SUB etc. command
 * 
 * param channel_id:   Channel id user inputs on the screen
 * 
 * param clientID:     User's client id
 * 
 * param *message:     message_t struct for if user is sending a message to the server 
 * 
 * param liveFeedFlag:     bool numerical value to indicate if user requesting livefeed on/off 
**************************************************************************************************/
request_t createRequest(int command, int channel_id, int clientID, message_t *message, int liveFeedFlag)
{

    request_t clientRequest;

    clientRequest.commandID = command;
    clientRequest.channelID = channel_id;
    clientRequest.clientID = clientID;
    clientRequest.liveFeedFlag = liveFeedFlag;

    if (message != NULL)
    {
        clientRequest.client_message = *message;
    }

    return clientRequest;
}

/************************************************************************************************
 * func:                    Sends the request object to the server using socket's internal send()
 *                   
 *                 
 * param clientRequest:     request_t struct i.e. the request object holds information on user
 *                          inputs/command that server will process.
 * 
 * param sock_fd:            Socket file descriptor
**************************************************************************************************/
int sendRequest(request_t clientRequest, int sock_fd)
{

    // send request to server
    if (send(sock_fd, &clientRequest, sizeof(struct request), 0) == -1)
    {
        perror("CLIENT: request to server failed [commandInput]...\n");
        return 1;
    }

    return 0; // requests successful.
}

/************************************************************************************************
 * func:                    Parses user's message checks if the length is less or equal to 1024
 *                          copies into message_t struct.
 *                   
 *                 
 * param *client_message:   Message message_t struct 
 * 
 * param *message:          Char message pointer to the message itself
 * 
 * param *clientID:         User's client id
**************************************************************************************************/
int parseUserMessage(message_t *client_message, char *message, int clientID)
{

    // If message inputted, assign to the request struct
    if (sizeof(*message) <= MAX_MESSAGE_LENGTH && &message[0] != NULL)
    {

        strncpy(client_message->content, message, MAX_MESSAGE_LENGTH);
        client_message->ownerID = clientID;

        return 0;
    }
    else
    {
        return 1;
    }
}