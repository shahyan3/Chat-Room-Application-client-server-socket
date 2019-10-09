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

#define MAX_CLIENTS 20

#define MAX_CHANNELS 255

#define MAX_MESSAGE_LENGTH 1024

#define NO_CHANNEL 0

/* COMMANDS */
#define SUB 0
#define UNSUB 1
#define NEXT_ID 2
#define LIVEFEED_ID 3
#define SEND 4

#define NEXT 5
#define LIVEFEED 6

int sockfd;
int new_fd; /* listen on sock_fd, new connection on new_fd */

typedef struct message message_t;

struct message
{
    int messageID;
    int ownerID;
    char content[MAX_MESSAGE_LENGTH];
    message_t *next;
};

message_t client_message;

typedef struct request request_t;

struct request
{
    int commandID; /* integer enum representing command i.e. SUB, NEXT*/
    int channelID;
    int clientID;
    message_t message;
};

request_t clientRequest; /* move into main ??*/

typedef struct client client_t;

struct client
{
    int clientID;
    int readMsg;
    int unReadMsg;
    int totalMessageSent;
    int messageQueueIndex;  /* index = message that it read currently, NOT NEXT ONE */
    int entryIndexConstant; /* index of msg queue at which the client subed */
    client_t *next;
};

typedef struct channel channel_t;

struct channel
{
    int channelID;
    int totalMsg;
    message_t *messageHead;
    message_t *messageTail;
    client_t *subscriberHead;
    client_t *subscriberTail;
};

channel_t hostedChannels[MAX_CHANNELS]; /* Global array so its in stack memory where child processes can access shared mem?*/

typedef struct response response_t;

struct response
{
    int clientID;
    message_t message;
    int error;
};

/*
    CHANNEL FUNCTIONS (INCLUDE IN SEPERATE FILE LATER)
*/
// when subscribing to channel, finds out the last message in the channel, and will be used to add the message id to the subscribing client
message_t createStatusResponseMessage(char *message, int clientID);
response_t createServerErrorResponse(message_t *message);

message_t *findLastMsgInLinkedList(channel_t channel);

void subscribe(client_t *client_, int channel_id);

client_t *findSubscriberInChannel(channel_t *channel, int clientID);

void print_subscribers();

void updateUnreadMsgCountForSubscribers(channel_t *channel);

void writeToChannelReq(request_t *request, channel_t *channel);

void print_channel_messages(int channel_id);

void print_channels();

message_t searchNextMsgInList(channel_t *channel, client_t *client);

message_t readNextMsgFromChannel(channel_t *channel, client_t *client);

response_t createServerResponse(message_t *message, client_t *client);
int sendResponse(response_t serverResponse, int sock_id);

message_t readNextMsgFromAllChannel(int clientID); // next no id

channel_t *getChannel(int channelID);

int nextMessageCountInChannels(int clientID);

/*
    END OF
*/

void shut_down_handler()
{
    // close threads
    // close processes
    // close sockets
    close(new_fd);
    // shared memory regions
    // close(new_fd);
    close(sockfd);
    // free allocated memory and open files
    // sleep(1);
    write(1, "\n\nShutting down gracefully, BYE!\n\n", 35);
    exit(1);
}

int create_client();

int handleClientRequests(request_t *request, client_t *client);

int randomClientIdGenerator();

int parseRequest(int new_fd, char *clientRequest, char *user_command, char *channel_id);

int main(int argc, char *argv[])
{
    int port = 12345;

    // signal handling
    struct sigaction sa;
    sa.sa_handler = shut_down_handler;
    sigaction(SIGINT, &sa, NULL);

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
    sin_size = sizeof(struct sockaddr_in);
    while (new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size))
    {
        printf("server: got connection from %s\n",
               inet_ntoa(their_addr.sin_addr));

        /* ***Server-Client Connected*** */

        printf("\n-------------------------------------------\n");
        // Create a new client object
        client_t client;
        if (create_client(&client) == 1)
        {
            perror("SERVER: failed to create client object\n");
            printf("SERVER: failed to create client object\n");
        }
        printf("This is server\n");

        // Create a channel
        channel_t channel1;
        channel_t channel2;
        channel_t channel3;

        channel1.channelID = 1;
        channel1.totalMsg = 0;
        channel1.subscriberHead = NULL;
        channel1.subscriberTail = NULL;
        channel1.messageHead = NULL;
        channel1.messageTail = NULL;

        channel2.totalMsg = 0;
        channel2.channelID = 2;
        channel2.subscriberHead = NULL;
        channel2.subscriberTail = NULL;
        channel2.messageHead = NULL;
        channel2.messageTail = NULL;

        channel3.totalMsg = 0;
        channel3.channelID = 3;
        channel3.subscriberHead = NULL;
        channel3.subscriberTail = NULL;
        channel3.messageHead = NULL;
        channel3.messageTail = NULL;

        hostedChannels[0] = channel1;
        hostedChannels[1] = channel2;
        hostedChannels[2] = channel3;
        // printf("Hosted Channel %d\n", hostedChannels[0].channelID);

        printf("\n-------------------------------------------\n");

        // Send: Welcome Message.
        if (send(new_fd, &client.clientID, sizeof(int), 0) == -1)
        {
            perror("send");
            printf("Error: Welcome message not sent to client \n");
        }

        while (recv(new_fd, &clientRequest, sizeof(request_t), 0))
        {

            if (handleClientRequests(&clientRequest, &client) == 0)
            {
                // printf("\n SERVER: Request Object: \n command id %d\n channel id %d \n client id %d\n client message %s\n msg ID %d\n",
                //        clientRequest.commandID, clientRequest.channelID, clientRequest.clientID, clientRequest.message.content, clientRequest.message.messageID);

                printf("\n********************************************************\n");
                printf("SERVER: successfully receive client request [userInput] |\n");
                printf("********************************************************\n");
            }
            else
            {
                printf("\n***********************************************\n");
                printf("SERVER: Bad request. Failed to handle client request |\n");
                printf("\n***********************************************\n");
            }
        }
    }

    close(new_fd);
    exit(0);
}

int handleClientRequests(request_t *request, client_t *client)
{

    if (request->commandID == SUB)
    { // Subscribe to Channel request
        printf("received sub ..;.\n");
        // printf("COMMAND:  %d \t channel id %d \n", request->channel_id, request->channelID);

        channel_t *channel;
        client_t *isClient;

        if (request->channelID == 0)
        { // Client sent SUB with no id

            // create error message
            char *message = "No channel id given";

            message_t errorMessage = createStatusResponseMessage(message, request->clientID);

            if (&errorMessage != NULL)
            {
                response_t response = createServerErrorResponse(&errorMessage);
                sendResponse(response, new_fd);
            }
            return 0;
        }
        else
        {
            channel = getChannel(request->channelID);

            if (channel->channelID != request->channelID)
            { // Invalid channel id
                // printf("\n here22 %d\n", channel->channelID);

                // convert channel id given from int to string
                char str_channel_id[MAX_MESSAGE_LENGTH];
                snprintf(str_channel_id, sizeof(int), "%d", request->channelID);
                // printf("###### %s", str_channel_id);

                // create error message
                char message[MAX_MESSAGE_LENGTH];
                strcpy(message, "\t\tInvalid channel: ");

                strcat(message, str_channel_id); /* concate id and string message */
                // printf("\n %s\n", message);

                char *message_ptr = message;
                // printf("\n %s\n", message_ptr);

                message_t errorMessage = createStatusResponseMessage(message_ptr, request->clientID);

                if (&errorMessage != NULL)
                {
                    response_t response = createServerErrorResponse(&errorMessage);
                    sendResponse(response, new_fd);
                }
            }
            else
            {
                printf("\n here33 %d\n", channel->channelID);

                isClient = findSubscriberInChannel(channel, request->clientID);

                if (isClient == NULL)
                {
                    subscribe(client, request->channelID);

                    print_subscribers();

                    print_channels();

                    // convert channel id given from int to string
                    char str_channel_id[MAX_MESSAGE_LENGTH];
                    snprintf(str_channel_id, sizeof(int), "%d", request->channelID);
                    // printf("###### %s", str_channel_id);

                    // create error message
                    char message[MAX_MESSAGE_LENGTH];
                    strcpy(message, "Subscribed to channel: ");

                    strcat(message, str_channel_id); /* concate id and string message */

                    char *message_ptr = message;

                    message_t successMessage = createStatusResponseMessage(message, request->clientID);

                    if (&successMessage != NULL)
                    {
                        response_t response = createServerResponse(&successMessage, client);
                        sendResponse(response, new_fd);
                    }
                }
                else
                { // Client already subscribed
                    // printf("\n   not null\n");

                    // convert channel id given from int to string
                    char str_channel_id[MAX_MESSAGE_LENGTH];
                    snprintf(str_channel_id, sizeof(int), "%d", request->channelID);
                    // printf("###### %s", str_channel_id);

                    // create error message
                    char message[MAX_MESSAGE_LENGTH];
                    strcpy(message, "Already subscribed to channel: ");

                    strcat(message, str_channel_id); /* concate id and string message */

                    char *message_ptr = message;

                    message_t errorMessage = createStatusResponseMessage(message, request->clientID);

                    if (&errorMessage != NULL)
                    {
                        response_t response = createServerErrorResponse(&errorMessage);
                        sendResponse(response, new_fd);
                    }
                }
            }
        }

        return 0;
    }

    if (request->commandID == SEND && request->channelID)
    { // SEND message to channel
        // printf("$Command id %d and channel id %d\n", request->commandID, request->channelID);
        int i;
        channel_t *channel;

        channel = getChannel(request->channelID);

        if (channel != NULL)
        {
            // printf("SEND: channel id is... found %d \n", channel->channelID);

            client = findSubscriberInChannel(channel, request->clientID);

            if (client != NULL && client->clientID == request->clientID)
            { // client is subscribed to channel

                writeToChannelReq(request, channel);

                // send client response
                char *message = "Message successfully sent";
                message_t successMessage = createStatusResponseMessage(message, request->clientID);

                if (&successMessage != NULL)
                {
                    response_t response = createServerErrorResponse(&successMessage);
                    sendResponse(response, new_fd);
                }
            }
            else
            {
                char *message = "Error: no client subscribed to the channel.";
                printf("\n %s\n", message);

                // CREATE A response.errorMessage property to client with the message in printf? TODO: bug.
                message_t errorMessage = createStatusResponseMessage(message, request->clientID);

                if (&errorMessage != NULL)
                {
                    response_t response = createServerErrorResponse(&errorMessage);
                    sendResponse(response, new_fd);
                }
            }
        }
        else
        {
            char *message = "Channel id given not found!";
            printf("\n %s\n", message);

            // CREATE A response.errorMessage property to client with the message in printf? TODO: bug.
            message_t errorMessage = createStatusResponseMessage(message, request->clientID);

            if (&errorMessage != NULL)
            {
                response_t response = createServerErrorResponse(&errorMessage);
                sendResponse(response, new_fd);
            }
        }

        print_channels();
        print_channel_messages(request->channelID);
        // print_subscribers();

        return 0;
    }

    if (request->commandID == NEXT_ID && request->channelID)
    { // READ message to channel
        printf("\nServer: NEXT id %d request recevied.\n", request->channelID);

        channel_t *channel;

        channel = getChannel(request->channelID); // find the channel given channel id

        if (channel != NULL)
        {
            client_t *client = findSubscriberInChannel(channel, request->clientID);

            if (client != NULL)
            {
                if (client->unReadMsg > 0)
                {
                    message_t unreadMessage = readNextMsgFromChannel(channel, client);

                    if (&unreadMessage != NULL)
                    { // MOTE: NEXT NULL BECAUSE OUTER IF STATEMENT IS TRUE i.e. client->unReadMsg > 0

                        // create server response to client WITH MESSAGE
                        response_t response = createServerResponse(&unreadMessage, client);

                        if (response.error == 0)
                        {
                            printf("\n________________________ \n");
                            printf("NEXT %d Message Sending...|\n", request->channelID);
                            printf("__________________________ \n");

                            printf("\nClientID: %d\n", response.clientID);
                            printf("Next message : %s\n", response.message.content);
                            printf("Error status : %d\n", response.error);
                            printf("\n");
                        }

                        // send response to client
                        sendResponse(response, new_fd);
                    }
                }
                else
                {

                    char *message = "No new messages found in channel!";
                    printf("\n %s\n", message);

                    // CREATE A response.errorMessage property to client with the message in printf? TODO: bug.
                    message_t noMsgErrorMessage = createStatusResponseMessage(message, request->clientID);

                    if (&noMsgErrorMessage != NULL)
                    {
                        response_t response = createServerErrorResponse(&noMsgErrorMessage);
                        sendResponse(response, new_fd);
                    }
                }
            }
            else
            {
                char *message = "Err. Client not subscribed to given channel";
                printf("\n %s\n", message);

                // CREATE A response.errorMessage property to client with the message in printf? TODO: bug.
                message_t noClientSubMsg = createStatusResponseMessage(message, request->clientID);

                if (&noClientSubMsg != NULL)
                {
                    response_t response = createServerErrorResponse(&noClientSubMsg);
                    sendResponse(response, new_fd);
                }
            }
        }
        else
        {
            char *message = "Channel id given not found!";
            printf("\n %s\n", message);

            // CREATE A response.errorMessage property to client with the message in printf? TODO: bug.
            message_t errorMessage = createStatusResponseMessage(message, request->clientID);

            if (&errorMessage != NULL)
            {
                response_t response = createServerErrorResponse(&errorMessage);
                sendResponse(response, new_fd);
            }
        }

        print_subscribers();

        return 0;
    }

    if (request->commandID == NEXT && request->channelID == NO_CHANNEL)
    {
        printf("\nSERVER: Next without <id> request received.\n");

        // int totalUnreadNextMsg = 0;
        // totalUnreadNextMsg = nextMessageCountInChannels(client->clientID);

        // printf("\n@ total unreadMsgCount %d\n\n", totalUnreadNextMsg);

        // if (client->unReadMsg > 0)
        // {
        //     while (totalUnreadNextMsg != 0)
        //     {
        //         message_t unReadMessage = readNextMsgFromAllChannel(request->clientID);

        //         if (&unReadMessage != NULL)
        //         {
        //             response_t response = createServerResponse(&unReadMessage, client);

        //             // send response to client
        //             if (sendResponse(response, new_fd) == 1)
        //             {
        //                 printf("\nSERVER: Error, failed to sent response to server [NEXT_ID]!\n");
        //             }
        //             else
        //             {
        //                 printf("\nClient: Successfully sent response to client [NEXT_ID]... \n");
        //             }
        //         }

        //         totalUnreadNextMsg -= 1; // decrement next unread message sent count;
        //     }
        // }
        // else
        // {
        //     response_t response = createServerResponse(NULL, client);
        //     if (sendResponse(response, new_fd) == 1)
        //     {
        //         printf("\nSERVER: Error, failed to sent response to server [NEXT NO ID]!\n");
        //     }
        //     else
        //     {
        //         printf("\nClient: Successfully sent response to client [NEXT NO ID]... \n");
        //     }
        // }
    }
}

// don't need it?
int parseRequest(int new_fd, char *clientRequest, char *user_command, char *channel_id)
{
    char *string_ptr;
    char delim[] = " ";

    string_ptr = strtok(clientRequest, delim);

    strcpy(user_command, string_ptr);
    // printf("%s", user_command);
    string_ptr = strtok(NULL, delim);

    strcpy(channel_id, string_ptr);
    // printf("%s", channel_id);
    string_ptr = strtok(NULL, delim);

    return 0;
}

char parseMessage(char *msg, int channel_id)
{

    // convert channel id given from int to string
    char str_channel_id[MAX_MESSAGE_LENGTH];
    snprintf(str_channel_id, sizeof(int), "%d", channel_id);
    // printf("###### %s", str_channel_id);

    // create error message
    char message[MAX_MESSAGE_LENGTH];
    strcpy(message, msg);

    strcat(message, str_channel_id); /* concate id and string message */
}

channel_t *getChannel(int channelID)
{
    int i;
    channel_t *channel;

    for (i = 0; i < MAX_CHANNELS; i++)
    {
        if (hostedChannels[i].channelID == channelID)
        {
            channel = &hostedChannels[i];
        }
    }

    return channel;
}

int sendResponse(response_t serverResponse, int sock_id)
{

    // send request to server
    if (send(sock_id, &serverResponse, sizeof(response_t), 0) == -1)
    {
        perror("SERVER: response to server failed...\n");
        return 1;
    }

    return 0; // requests successful.
}

response_t createServerErrorResponse(message_t *message)
{
    response_t *response = malloc(sizeof(response_t));

    response->message = *message;
    response->error = 1;

    return *response;
}

// Should i return a pointer response instead since using malloc? ask tutor
response_t createServerResponse(message_t *message, client_t *client)
{
    response_t *response = malloc(sizeof(response_t));

    if (message != NULL)
    {
        // printf("\n\n CreateResponseServer: Message given not NULL\n");

        response->clientID = client->clientID;
        response->message = *message;
        response->error = 0;
    }
    else
    {
        // printf("\n\n CreateResponseServer: Message given NULL\n");
        response->clientID = client->clientID;
        response->error = 1;
    }

    // printf("\n$MAS: %s\n", response->message.content);

    return *response;
}

int randomClientIdGenerator()
{
    int num = rand() % MAX_CLIENTS;

    return num;
}

int create_client(client_t *client)
{

    int clientID; /* client id given to each connected client */
    clientID = randomClientIdGenerator();

    client->clientID = clientID;
    client->readMsg = 0;
    client->unReadMsg = 0;
    client->totalMessageSent = 0;
    client->messageQueueIndex = 0;
    client->entryIndexConstant = 0;
    client->next = NULL;

    if (&client->clientID != NULL)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}
void print_subscribers()
{
    int i = 0;
    channel_t *channel;

    printf("==== Subscribers =====\n");

    for (i = 0; i < MAX_CHANNELS; i++)
    {
        channel_t *channel = &hostedChannels[i];

        if (channel->subscriberHead != NULL)
        {
            client_t *subHead = channel->subscriberHead;
            printf("\n CHANNEL: %d \n", channel->channelID);
            while (subHead != NULL)
            {
                printf("\t Subscriber id %d \n", subHead->clientID);
                printf("\t Subscriber messageQueueIndex %d \n", subHead->messageQueueIndex);
                printf("\t Subscriber entryIndexConstant %d \n", subHead->entryIndexConstant);
                printf("\t Subscriber readMsg %d \n", subHead->readMsg);
                printf("\t Subscriber unReadMsg %d \n", subHead->unReadMsg);
                printf("\t Subscriber totalMessageSent %d \n", subHead->totalMessageSent);
                printf("\n -------------------------- \n");

                subHead = subHead->next;
            }
        }
    }

    // printf("TAIL: subscriber id %d\n\n", channel->subscriberTail->clientID);
    // printf("HEAD: subscriber id %d\n\n", channel->subscriberHead->clientID);
    printf("====================== \n");
}

void updateUnreadMsgCountForSubscribers(channel_t *channel)
{
    client_t *currentNode = channel->subscriberHead;
    if (currentNode == NULL)
    {
        printf("Error, no subscribers found in channel");
    }
    while (currentNode != NULL)
    {
        currentNode->unReadMsg += 1;
        currentNode = currentNode->next;
    }
}

client_t *findSubscriberInChannel(channel_t *channel, int clientID)
{
    // printf("\n\n *** clientid %d, CHANNEL ID %d\n", clientID, channel->channelID);

    client_t *client = NULL;

    client_t *subHead = channel->subscriberHead;

    if (subHead == NULL)
    {
        // printf("\n **no subscriber in the list\n");
        return subHead;
    }
    else
    {
        while (subHead != NULL)
        {
            if (subHead->clientID == clientID)
            {
                client = subHead;
                break;
            }
            subHead = subHead->next;
        }
        return subHead;
    }
}

message_t *findLastMsgInLinkedList(channel_t channel)
{

    message_t *lastMsgInList = NULL;
    message_t *msgHead = channel.messageHead;

    if (channel.messageHead == NULL)
    {
        // no messages in linkedlist
        lastMsgInList = NULL;
    }

    while (msgHead != NULL)
    {
        lastMsgInList = msgHead;
        msgHead = msgHead->next;
    }

    return lastMsgInList;
}

void subscribe(client_t *client_, int channel_id)
{
    client_t *client = malloc(sizeof(client_t));
    *client = *client_;

    int i = 0;
    channel_t *channel;
    client_t *currentNode;
    message_t *message;

    for (i = 0; i < MAX_CHANNELS; i++)
    {
        if (hostedChannels[i].channelID == channel_id)
        {
            // printf("\n\nChannel selected found: %d\n", hostedChannels[i].channelID);
            channel = &hostedChannels[i];
            currentNode = channel->subscriberHead;

            if (channel->subscriberHead == NULL)
            {
                // printf("Selected channels subscriberHead is NULL\n");

                // update the client message queue to current message count in channel
                client->entryIndexConstant = channel->totalMsg;
                client->messageQueueIndex = channel->totalMsg;

                // push to linked list
                channel->subscriberHead = client;
                channel->subscriberTail = channel->subscriberHead; // track the last node
                // printf("1. CHANNEL SUBSCRIBER HEAD clinet id = %d\n", channel->subscriberHead->clientID);
            }
            else
            {
                // printf("Selected channels subscriberHead is NOT NULL\n");

                // update the client message queue to current message count in channel
                client->entryIndexConstant = channel->totalMsg;
                client->messageQueueIndex = channel->totalMsg;

                while (currentNode->next != NULL)
                {
                    // if the next value null = last node
                    currentNode = currentNode->next;
                }

                // push to linked list
                currentNode->next = client;                  // on the last node add next client
                channel->subscriberTail = currentNode->next; // track the last node
            }
        }
        else
        {
            // printf("Channel id not found\n");
        }
    }
}

void writeToChannelReq(request_t *request, channel_t *channel)
{ // critical section **&^%$%^&&^%

    client_t *client;

    message_t *client_message = malloc(sizeof(message_t));
    *client_message = request->message;

    client_message->next = NULL;

    message_t *currentNode;

    if (channel->messageHead == NULL)
    {
        // 1. update total message in channel (before adding message to ensure correct message id given)
        channel->totalMsg += 1;

        // 2. Update Message ID: message id is equal to current total message count in channel
        client_message->messageID = channel->totalMsg;

        // 3. Add message to channel
        channel->messageHead = client_message;

        channel->messageTail = channel->messageHead; /* point the tail to the head */
    }
    else
    {

        currentNode = channel->messageHead;

        while (currentNode->next != NULL)
        {
            currentNode = currentNode->next;
        }
        // 1. create memory for last node.next pointer
        currentNode->next = malloc(sizeof(message_t));

        // 2. update total msg count for channel.totalMessages
        channel->totalMsg += 1;

        // 3. Update Message ID: message id is equal to current total message count in channel
        client_message->messageID = channel->totalMsg;

        // 4. Add message to the end of the messageHead list
        *currentNode->next = *client_message;

        channel->messageTail = currentNode->next;
    }

    // works: update unreadMsg property for all subcribed clients except client with this clientID
    updateUnreadMsgCountForSubscribers(channel);

    // end of critical section ?
}

void print_channel_messages(int channel_id)
{
    int i = 0;
    printf("\n==== All Channels Messages ===============================================\n");

    for (i = 0; i < MAX_CHANNELS; i++)
    {

        channel_t *channel = &hostedChannels[i];
        message_t *msgHead = channel->messageHead;

        if (msgHead == NULL)
        {
            // printf("No Messages FOUND!");
        }
        while (msgHead != NULL)
        {
            printf("ChannelID: %d, MessageID: %d | ClientID %d, Content: %s \n", channel->channelID, msgHead->messageID, msgHead->ownerID, msgHead->content);
            msgHead = msgHead->next;
        }
    }
}

void print_channels()
{
    int i = 0;
    printf("\n-----CHANNELS WITH SUBSCRIBER(s) ---------\n");
    for (i = 0; i < MAX_CHANNELS; i++)
    {
        if (hostedChannels[i].subscriberHead != NULL)
        {
            printf("Channel id %d\n", hostedChannels[i].channelID);
            printf("Channel total msgs: %d\n", hostedChannels[i].totalMsg);
            printf("\n");
        }
    }
}

message_t searchNextMsgInList(channel_t *channel, client_t *client)
{

    message_t unreadMessage;
    message_t *currentNode = channel->messageHead;
    int clientsNextMessageToReadIndex = client->messageQueueIndex + 1;

    printf("\nSELECTED CHANNEL unReads ### %d\n", client->unReadMsg);

    if (currentNode == NULL)
    {
        // no messages in channel
        // unreadMessage = NULL;
        printf("\n-->no messages in channel!\n");
        return unreadMessage;
    }
    else
    {

        printf("\n-->YES 111111111111\n");

        while (currentNode != NULL)
        {

            if (currentNode->messageID == clientsNextMessageToReadIndex)
            {
                printf("\n-->YES 22222\n"); // MESSAGE ID is starting at 3 for channel 2, therefore doesn't read. #bug

                // if messageID is the next in messageQueueIndex for client
                unreadMessage = *currentNode;
                client->messageQueueIndex += 1; // update messageQueueIndex

                client->readMsg += 1;   // update read messages count
                client->unReadMsg -= 1; // update unread messages count
            }
            currentNode = currentNode->next;
        }

        printf("\n CLIENT UPDATED unreads??? %d\n", client->unReadMsg);
        return unreadMessage;
    }
}

message_t readNextMsgFromChannel(channel_t *channel, client_t *client)
{ // &&&&&&&&&&&&&&& FIX LIKE BLOODY SEND ONE - REMOVE FOR LOOP WITH FUCNTION

    printf("SELECTED CHANNEL unReads $$ %d", client->unReadMsg);
    message_t unreadMessage = searchNextMsgInList(channel, client);

    // printf("\n 2.Next msg: %s\n", unreadMessage.content);

    return unreadMessage;
}

// NEXT: returns unread message from all channels one by one
message_t readNextMsgFromAllChannel(int clientID)
{
    // int i = 0;
    // client_t *foundClient;
    // message_t unreadMessage;
    // channel_t *channel;

    // for (i = 0; i < MAX_CHANNELS; i++)
    // {
    //     channel = &hostedChannels[1];
    //     foundClient = findSubscriberInChannel(channel, clientID);

    //     if (foundClient != NULL && foundClient->clientID == clientID)
    //     { // Found channel client is subscribed too!
    //         unreadMessage = readNextMsgFromChannel(channel, foundClient);

    //         return unreadMessage;
    //     }
    // }
}

// counts the nextUnread message from all channels, returns the count
int nextMessageCountInChannels(int clientID)
{
    // int i = 0;
    // int totalUnreadMsg = 0;

    // client_t *foundClient;
    // message_t unreadMessage;
    // channel_t *channel;

    // for (i = 0; i < MAX_CHANNELS; i++)
    // {
    //     channel = &hostedChannels[i];
    //     if (channel != NULL)
    //     {
    //         foundClient = findSubscriberInChannel(channel, clientID);

    //         // test below
    //         if (foundClient != NULL)
    //         {
    //             printf("\n\nclientid %d, CHANNEL ID %d given. \n", clientID, channel->channelID);
    //             printf("\n Result:FOUND client ID: %d \n", foundClient->clientID);
    //         }
    //         // test above

    //         if (foundClient != NULL && foundClient->clientID == clientID)
    //         { // Found channel client is subscribed too!

    //             printf("\n #Subscriber found in channel \n");

    //             unreadMessage = readNextMsgFromChannel(channel, foundClient);

    //             if (&unreadMessage != NULL)
    //             {
    //                 totalUnreadMsg += 1;
    //                 printf("\nmessage FOUND ID %d %s\n", unreadMessage.ownerID, unreadMessage.content);
    //                 printf("total msg num: %d\n", totalUnreadMsg);
    //             }
    //         }
    //     }
    //     // printf("for loop i = %d", i);
    // }

    // printf("()()() count final: %d\n\n", totalUnreadMsg);

    // return totalUnreadMsg;
}

message_t createStatusResponseMessage(char *message, int clientID)
{

    message_t statusMessage;

    statusMessage.messageID = 1;
    statusMessage.ownerID = clientID;

    strcpy(statusMessage.content, message);

    return statusMessage;
}