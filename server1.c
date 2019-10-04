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

#define MAX_CHANNELS 5

#define MAX_MESSAGE_LENGTH 1024

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
message_t *findLastMsgInLinkedList(channel_t channel);

void subscribe(client_t *client, int channel_id);

client_t *findSubsriberInList(channel_t *channel, int clientID);

void print_subscribers();

void updateUnreadMsgCountForSubscribers(channel_t *channel);

void writeToChannelReq(request_t *request);

void print_channel_messages(int channel_id);

message_t *searchNextMsgInList(channel_t *channel, client_t *client);

message_t readNextMsgFromChannel(int clientID, int channelID);

response_t createServerResponse(message_t *message, client_t *client);
int sendResponse(response_t serverResponse, int sock_id);
/*
    END OF
*/

void shut_down_handler()
{
    // close threads
    // close processes
    // close sockets
    // close(new_fd);
    // shared memory regions
    // close(new_fd);
    close(sockfd);
    // free allocated memory and open files
    sleep(1);
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

        channel1.channelID = 1;
        channel1.totalMsg = 0;
        channel1.subscriberHead = NULL;
        channel1.subscriberTail = NULL;
        channel1.messageHead = NULL;
        channel1.messageTail = NULL;

        hostedChannels[0] = channel1;
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

                printf("SERVER: successfully receive client request [userInput]!\n");
            }
            else
            {
                printf("SERVER: Error, failed to handle client request\n");
            }
        }
    }

    close(new_fd);
    exit(0);
}

int handleClientRequests(request_t *request, client_t *client)
{

    if (request->clientID != client->clientID)
    {
        printf("ERR! No client found. Bad request\n");
        return 1;
    }

    if (request->commandID == SUB && request->channelID)
    { // Subscribe to Channel request

        subscribe(client, request->channelID);

        print_subscribers();

        return 0;
    }

    if (request->commandID == SEND && request->channelID)
    { // SEND message to channel
        // printf("$Command id %d and channel id %d\n", request->commandID, request->channelID);
        writeToChannelReq(request);
        print_channel_messages(request->channelID);
        print_subscribers();

        printf("Channel total messages: %d\n", hostedChannels[0].totalMsg);

        return 0;
    }

    if (request->commandID == NEXT_ID && request->channelID)
    { // READ message to channel
        printf("\nServer: next <id> request recevied.\n");

        if (client->unReadMsg > 0)
        {
            message_t unreadMessage = readNextMsgFromChannel(client->clientID, request->channelID);

            if (&unreadMessage != NULL)
            {
                // create server response to client
                response_t response = createServerResponse(&unreadMessage, client);

                if (response.error == 0)
                {
                    printf("\n =================== \n");
                    printf("SERVER RESPONSE ON SERVER \n");
                    printf("\nServer response client id: %d\n", response.clientID);
                    printf("\nServer response message : %s\n", response.message.content);
                    printf("\nServer response error status : %d\n", response.error);
                    printf("\n == ================= \n");
                }
                else
                {
                    printf("\n\nNo new messages found in channel!\n");
                }

                // send response to client
                if (sendResponse(response, new_fd) == 1)
                {
                    printf("\nSERVER: Error, failed to sent response to server [NEXT_ID]!\n");
                }
                else
                {
                    printf("\nClient: Successfully sent response to client [NEXT_ID]... \n");
                }
                // }

                print_subscribers();

                printf("\nChannel total messages: %d\n", hostedChannels[0].totalMsg);

                return 0;
            }
        }
        else
        {
            response_t response = createServerResponse(NULL, client);

            if (response.error == 1)
            {
                printf("\n\n =========**********========== \n");
                printf("\n\nNo new messages found in channel!\n");
                printf("\n =================== \n");
            }

            if (sendResponse(response, new_fd) == 1)
            {
                printf("\nSERVER: Error, failed to sent response to server [NEXT_ID]!\n");
            }
            else
            {
                printf("\nClient: Successfully sent response to client [NEXT_ID]... \n");
            }
        }
        if (request->commandID == NEXT)
        {
            printf("\nSERVER: Next without <id> request received.\n");

            // printf("\nServer response: \n ");
            // printf("Server: client id %d, unreadMessage %s\n", response.clientID, response.message.content);

            return 0;
        }
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

// Should i return a pointer response instead since using malloc? ask tutor
response_t createServerResponse(message_t *message, client_t *client)
{
    response_t *response = malloc(sizeof(response_t));

    if (message != NULL)
    {
        printf("\n\nhurray works!\n");

        response->clientID = client->clientID;
        response->message = *message;
        response->error = 0;
    }
    else
    {
        printf("\n\ncomes with error\n");
        response->clientID = client->clientID;
        response->error = 1;
    }

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
    channel_t *channel = &hostedChannels[0];

    client_t *subHead = channel->subscriberHead;

    printf("==== Subscribers =====\n");

    while (subHead != NULL)
    {
        printf("Subscriber id %d \n", subHead->clientID);
        printf("Subscriber messageQueueIndex %d \n", subHead->messageQueueIndex);
        printf("Subscriber entryIndexConstant %d \n", subHead->entryIndexConstant);
        printf("Subscriber readMsg %d \n", subHead->readMsg);
        printf("Subscriber unReadMsg %d \n", subHead->unReadMsg);
        printf("Subscriber totalMessageSent %d \n", subHead->totalMessageSent);
        printf("\n\n -------------------------- \n\n");

        subHead = subHead->next;
    }

    printf("TAIL: subscriber id %d\n\n", channel->subscriberTail->clientID);
    printf("HEAD: subscriber id %d\n\n", channel->subscriberHead->clientID);
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

client_t *findSubsriberInList(channel_t *channel, int clientID)
{
    client_t *client = NULL;
    client_t *subHead = channel->subscriberHead;

    if (subHead == NULL)
    {
        printf("no subscriber in the list\n");
    }

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

void subscribe(client_t *client, int channel_id)
{
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

void writeToChannelReq(request_t *request)
{ // critical section ?

    client_t *client;
    channel_t *channel;

    message_t *client_message = malloc(sizeof(message_t));
    *client_message = request->message;
    client_message->next = NULL;

    message_t *currentNode;

    int i = 0;

    for (i = 0; i < MAX_CHANNELS; i++)
    {
        if (hostedChannels[i].channelID == request->channelID)
        {
            channel = &hostedChannels[i];

            // channel exists
            client = findSubsriberInList(channel, request->clientID);

            if (client->clientID == request->clientID)
            { // client is subscribed to channel

                if (channel->messageHead == NULL)
                {
                    channel->messageHead = client_message;

                    channel->messageTail = channel->messageHead; /* point the tail to the head */

                    // update total message in channel
                    channel->totalMsg += 1;

                    // printf("(NULL)head msg %s \n", channel->messageHead->content);
                    // printf("(NULL)tail msg %s \n", channel->messageTail->content);
                }
                else
                {

                    currentNode = channel->messageHead;

                    while (currentNode->next != NULL)
                    {
                        currentNode = currentNode->next;
                    }
                    // Add new message to head of the list
                    currentNode->next = malloc(sizeof(message_t));
                    *currentNode->next = *client_message;

                    channel->messageTail = currentNode->next;

                    // update total msg count for channel.totalMessages
                    channel->totalMsg += 1;

                    // printf("(ESLE) head msg %s \n", channel->messageHead->content);
                    // printf("head msg next %s \n", channel->messageHead->next->content);
                    // printf("(ELSE) tail msg %s \n", channel->messageTail->content);
                }

                // works: update unreadMsg property for all subcribed clients except client with this clientID
                updateUnreadMsgCountForSubscribers(channel);

                // end of critical section ?
            }
            else
            {
                printf("Error: no client subscribed to the channel \n");
            }
        }
    }
}

void print_channel_messages(int channel_id)
{
    int i = 0;
    for (i = 0; i < MAX_CHANNELS; i++)
    {
        if (hostedChannels[i].channelID == channel_id)
        {
            channel_t *channel = &hostedChannels[0];
            message_t *msgHead = channel->messageHead;

            printf("\n==== Channel's Messages ====\n");
            if (msgHead == NULL)
            {
                printf("No Messages FOUND!");
            }
            while (msgHead != NULL)
            {
                printf("Message ID %d Message:Owner Id: %d. Content %s \n", msgHead->messageID, msgHead->ownerID, msgHead->content);
                msgHead = msgHead->next;
            }
        }
    }
    printf("\n\n==== Channel's Messages ====\n\n");
}

message_t *searchNextMsgInList(channel_t *channel, client_t *client)
{

    message_t *unreadMessage;
    message_t *currentNode = channel->messageHead;
    int clientsNextMessageToReadIndex = client->messageQueueIndex + 1;

    if (currentNode == NULL)
    {
        // no messages in channel
        // unreadMessage = NULL;
        printf("\nno messages in channel!\n");
    }
    else
    {

        while (currentNode != NULL)
        {
            if (currentNode->messageID == clientsNextMessageToReadIndex)
            {
                // if messageID is the next in messageQueueIndex for client
                unreadMessage = currentNode;
                client->messageQueueIndex += 1; // update messageQueueIndex

                client->readMsg += 1;   // update read messages count
                client->unReadMsg -= 1; // update unread messages count
            }
            currentNode = currentNode->next;
        }
        return unreadMessage;
    }
}

message_t readNextMsgFromChannel(int clientID, int channelID)
{
    message_t unreadMessage;
    int i = 0;

    for (i = 0; i < MAX_CHANNELS; i++)
    {
        if (hostedChannels[i].channelID == channelID)
        {
            // channel exists
            channel_t *channel = &hostedChannels[i];
            client_t *client = findSubsriberInList(channel, clientID);

            //  find the NEXT unread message in the channel messag linked list

            printf("greater than 0\n");
            message_t *nextMessage = searchNextMsgInList(channel, client);

            if (nextMessage == NULL)
            {
                printf("no message found\n");
            }
            else
            {
                unreadMessage = *nextMessage;
            }
        }
        else
        {
            printf("no messages found in channel!");
        }
    }

    return unreadMessage;
}

// message_t readNextMsgFromAllChannel(int clientID)
// {
//     int i = 0;
//     client_t *foundClient;
//     message_t unreadMessage;

//     for (i = 0; i < MAX_CHANNELS; i++)
//     {

//         foundClient = findSubsriberInList(hostedChannels[i].channelID, clientID);

//         if (foundClient != NULL && foundClient->clientID == clientID)
//         { // Found channel client is subscribed too!
//             unreadMessage = readNextMsgFromChannel(clientID, hostedChannels[i].channelID);
//             if (&unreadMessage != NULL)
//             {
//                 // code
//             }
//         }
//     }
// }
