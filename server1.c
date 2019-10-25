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
#include <pthread.h> /* pthread */

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "server_functions.h"

#define BACKLOG 10 /* how many pending connections queue will hold */

#define RETURNED_ERROR -1

#define NO_CHANNEL -1

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
#define LIVEFEED_FLAG_TRUE 0
#define LIVEFEED_FLAG_FALSE -1

#define CLIENT_ACTIVE -1
#define CLIENT_INACTIVE -2

#define SIZEOFMEMORY 27

// SOCKET fd
int sockfd;
int new_fd;

// MUTEX LOCK
pthread_mutex_t mutex_lock;

// shared memory variables
int shmid;
key_t key;

// thread
pthread_t thread1;

// shared memory pointer
sharedMemory_t *sharedChannels;

int main(int argc, char *argv[])
{
    /* Threads */
    // initialize mutex lock - new threads will be given lock
    pthread_mutex_init(&mutex_lock, NULL);

    int port = 12345;

    // signal handling
    struct sigaction sa;
    sa.sa_handler = shut_down_handler;
    sigaction(SIGINT, &sa, NULL);

    struct sockaddr_in my_addr;    /* my address information */
    struct sockaddr_in their_addr; /* connector's address information */
    socklen_t sin_size;
    int i = 0;

    /** Create Shared Memory    **/
    key = 1234;

    //  Create the segment using shmget
    int size = (sizeof(sharedMemory_t));
    if ((shmid = shmget(key, size, IPC_CREAT | 0666)) < 0) /* if return value is less 0, error*/
    {
        perror("shmget");
        exit(1);
    }

    /*
        * Now we attach the segment to our data space. For each process??????
        */
    if ((sharedChannels = shmat(shmid, NULL, 0)) == (sharedMemory_t *)-1)
    {
        perror("shmat");
        exit(1);
    }

    // GENEATE CHANNELS IN SHARED MEMORY
    for (i = 0; i < MAX_CHANNELS; i++)
    {

        sharedChannels->hostedChannels[i].channelID = i;
        sharedChannels->hostedChannels[i].messagesCount = 0;
        sharedChannels->hostedChannels[i].messagesCount = 0;
        sharedChannels->hostedChannels[i].subscriberCount = 0;
    }

    /* Get port number for server to listen on */
    if (argc == 2)
    {
        fprintf(stderr, "usage: client port_number\n");
        port = atoi(argv[1]);
    }

    /* generate the socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    // SO_REUSEADDR - addresses server binding.
    uint32_t yes = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

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

    printf("\n-------------------------------------------\n");

    /* repeat: accept, send, close the connection */
    while (1)
    {

        sin_size = sizeof(struct sockaddr_in);
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

        if (new_fd == -1)
        {
            perror("accept.");
            printf("\n...error: accept new_fd failed\n");
        }

        printf("server: got connection from %s\n",
               inet_ntoa(their_addr.sin_addr));

        if (!fork())
        { /* this is the child process */

            /*
            * Now we attach the segment to our data space.
            */
            if ((sharedChannels = shmat(shmid, NULL, 0)) == (sharedMemory_t *)-1)
            {
                perror("shmat");
                exit(1);
            }

            /* ***Server-Client Connected*** */
            int child_id = getpid();
            client_t client = generate_client(child_id);

            if (client.clientID < -1)
            {
                perror("SERVER: failed to create client object (Max. 100 clients allowed)");
                printf("SERVER: failed to create client object (Max. 100 clients allowed) \n");
                exit(1);
            }

            // Send: Welcome Message.
            if (send(new_fd, &client.clientID, sizeof(int), 0) == -1)
            {
                perror("send");
                printf("Error: Welcome message not sent to client \n");
            }

            // Handle incoming client requests
            while (recv(new_fd, &clientRequest, sizeof(request_t), 0))
            {

                if (handleClientRequests(&clientRequest, &client) == 0)
                {
                    printf("\n SERVER: Request Object: \n command id %d\n channel id %d \n client id %d\n client message %s\n msg ID %d\n",
                           clientRequest.commandID, clientRequest.channelID, clientRequest.clientID, clientRequest.message.content, clientRequest.message.messageID);

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

            close(new_fd);
            exit(1);
        }
    }
}

int handleClientRequests(request_t *request, client_t *client)
{

    printf("\n --> command: %d channel id %d \n", request->commandID, request->channelID);

    if (request->commandID == SUB)
    { // Subscribe to Channel request
        printf("\n SUB...\n");

        channel_t *channel;
        client_t *isClient;

        if (request->channelID == NO_CHANNEL)
        { // Client sent SUB with no id

            // create error message
            char *message = "No channel id given";

            message_t errorMessage = createStatusResponseMessage(message, request->clientID);

            if (&errorMessage.messageID != NULL)
            {
                response_t response = createServerErrorResponse(&errorMessage);
                sendResponse(response, new_fd);
            }
            return 0;
        }
        else
        {
            channel = getChannel(request->channelID);

            if (channel == NULL || channel->channelID != request->channelID)
            { // Invalid channel id

                char *msg = "Invalid channel: ";
                char *message_ptr = parseMessage(msg, request->channelID);

                message_t errorMessage = createStatusResponseMessage(message_ptr, request->clientID);

                if (&errorMessage.messageID != NULL)
                {
                    response_t response = createServerErrorResponse(&errorMessage);
                    sendResponse(response, new_fd);
                }
            }
            else
            { // Subscribe to channel

                isClient = findSubscriberInChannel(channel, request->clientID);

                if (isClient == NULL)
                {
                    subscribe(client, request->channelID);

                    print_channels_with_subscribers();

                    print_subscribers_all();

                    char *msg = "Subscribed to channel: ";
                    char *message_ptr = parseMessage(msg, request->channelID);

                    message_t successMessage = createStatusResponseMessage(message_ptr, request->clientID);

                    if (&successMessage.messageID != NULL)
                    {
                        response_t response = createServerResponse(&successMessage, client, request->channelID, -1, LIVEFEED_FLAG_FALSE);
                        sendResponse(response, new_fd);
                    }
                }
                else
                { // Client already subscribed

                    char *msg = "Already subscribed to channel: ";
                    char *message_ptr = parseMessage(msg, request->channelID);

                    message_t errorMessage = createStatusResponseMessage(message_ptr, request->clientID);

                    if (&errorMessage.messageID != NULL)
                    {
                        response_t response = createServerErrorResponse(&errorMessage);
                        sendResponse(response, new_fd);
                    }
                }

                return 0;
            }
        }

        return 0;
    }

    else if (request->commandID == UNSUB)
    { // UNSUB to Channel request
        printf("\n UNSUB...\n");

        channel_t *channel;
        client_t *isClient;

        if (request->channelID == NO_CHANNEL)
        { // Client sent SUB with no id

            // create error message
            char *message = "No channel id given";

            message_t errorMessage = createStatusResponseMessage(message, request->clientID);

            if (&errorMessage.messageID != NULL)
            {
                response_t response = createServerErrorResponse(&errorMessage);
                sendResponse(response, new_fd);
            }
        }
        else
        {
            printf("\n...here 1\n");

            channel = getChannel(request->channelID);

            if (channel == NULL || channel->channelID != request->channelID)
            { // Invalid channel id

                char *msg = "Invalid channel: ";
                char *message_ptr = parseMessage(msg, request->channelID);

                message_t errorMessage = createStatusResponseMessage(message_ptr, request->clientID);

                if (&errorMessage.messageID != NULL)
                {
                    response_t response = createServerErrorResponse(&errorMessage);
                    sendResponse(response, new_fd);
                }
            }
            else
            {
                printf("\n MADE IT..\n");

                isClient = findSubscriberInChannel(channel, request->clientID);

                if (isClient != NULL)
                { // unsubscribe client
                    printf("\n doing it\n");
                    unSubscribe(request->channelID, isClient->clientID);
                    printf("\n done!\n");

                    print_channels_with_subscribers();

                    char *msg = "Unsubscribed from channel: ";
                    char *message_ptr = parseMessage(msg, request->channelID);

                    message_t successMessage = createStatusResponseMessage(message_ptr, request->clientID);

                    if (&successMessage.messageID != NULL)
                    {
                        response_t response = createServerResponse(&successMessage, client, request->channelID, -1, LIVEFEED_FLAG_FALSE);
                        sendResponse(response, new_fd);
                    }
                }
                else
                { // Client already subscribed

                    char *msg = "Not subscribed to channel: ";
                    char *message_ptr = parseMessage(msg, request->channelID);

                    message_t errorMessage = createStatusResponseMessage(message_ptr, request->clientID);

                    if (&errorMessage.messageID != NULL)
                    {
                        response_t response = createServerErrorResponse(&errorMessage);
                        sendResponse(response, new_fd);
                    }
                }
            }

            printf("\n YUPPY..\n");
        }
        printf("\n END...\n");

        return 0;
    }

    if (request->commandID == SEND)
    { // SEND message to channel

        channel_t *channel;

        channel = getChannel(request->channelID);

        if (channel != NULL)
        { // NOTE: Client who has NOT* subscribe CAN send a message to a channel*

            writeToChannelReq(request, channel); // write to channel if channel found

            // // send client response
            char *message = "Message successfully sent";
            message_t successMessage = createStatusResponseMessage(message, request->clientID);

            if (&successMessage.messageID != NULL)
            {
                response_t response = createServerErrorResponse(&successMessage);
                sendResponse(response, new_fd);
            }
        }
        else
        {
            char *msg = "Invalid channel: ";
            char *message_ptr = parseMessage(msg, request->channelID);

            message_t errorMessage = createStatusResponseMessage(message_ptr, request->clientID);

            if (&errorMessage.messageID != NULL)
            {
                response_t response = createServerErrorResponse(&errorMessage);
                sendResponse(response, new_fd);
            }
        }

        print_channels_with_subscribers();
        print_channel_messages();

        return 0;
    }

    if (request->commandID == NEXT_ID && request->channelID != NO_CHANNEL)
    { // READ message to channel

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

                    if (&unreadMessage.messageID != NULL)
                    { // if unread message is found in channel

                        // create server response to client WITH MESSAGE
                        response_t response = createServerResponse(&unreadMessage, client, request->channelID, -1, LIVEFEED_FLAG_FALSE);

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
                { // if no new message send error response

                    char *message = "(NEXT with ID) No new messages found in channel!";
                    // printf("\n %s\n", message);

                    message_t noMsgErrorMessage = createStatusResponseMessage(message, request->clientID);

                    if (&noMsgErrorMessage.messageID != NULL)
                    {
                        response_t response = createServerErrorResponse(&noMsgErrorMessage);
                        sendResponse(response, new_fd);
                    }
                }
            }
            else
            { // if client not subscribed to channel send errer response

                char *msg = "Not subscribed to channel: ";
                char *message_ptr = parseMessage(msg, request->channelID);

                message_t errorMessage = createStatusResponseMessage(message_ptr, request->clientID);

                if (&errorMessage.messageID != NULL)
                {
                    response_t response = createServerErrorResponse(&errorMessage);
                    sendResponse(response, new_fd);
                }
            }
        }
        else
        { // if invalid channel id given

            char *msg = "Invalid channel: ";
            char *message_ptr = parseMessage(msg, request->channelID);

            message_t errorMessage = createStatusResponseMessage(message_ptr, request->clientID);

            if (&errorMessage.messageID != NULL)
            {
                response_t response = createServerErrorResponse(&errorMessage);
                sendResponse(response, new_fd);
            }
        }

        print_channels_with_subscribers();

        return 0;
    }

    if (request->commandID == NEXT && request->channelID == NO_CHANNEL)
    { // NEXT no id

        thdata data1; /* init thread data struct */

        data1.clientID = request->clientID;
        data1.channel_id = request->channelID;

        pthread_create(&thread1, NULL, (void *(*)(void *))handleNextSend, (void *)&data1); // run NEXT recieve command in thread

        pthread_join(thread1, NULL);

        return 0;
    }

    if (request->commandID == LIVEFEED_ID && request->liveFeedFlag == LIVEFEED_FLAG_TRUE || request->liveFeedFlag == LIVEFEED_FLAG_FALSE)
    { // LIVEFEED with id

        channel_t *channel;

        channel = getChannel(request->channelID); // find the channel given channel id
 
        if (channel != NULL)
        {
             client_t *client = findSubscriberInChannel(channel, request->clientID);

            if (client != NULL)
            {

                while (request->liveFeedFlag == LIVEFEED_FLAG_TRUE)
                { // loop while the livefeed flag is true in request
                    if (client->unReadMsg > 0)
                    { // send reponse if there are unread Messages
                        printf("\nlivefeed true\n");
                        message_t unreadMessage = readNextMsgFromChannel(channel, client);

                        if (&unreadMessage.messageID != NULL)
                        { // is unread message is true send to client

                            // create server response to client WITH MESSAGE
                            response_t response = createServerResponse(&unreadMessage, client, request->channelID, -1, LIVEFEED_FLAG_TRUE);

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
                }
            }
            else
            { // client not subscribed to a channel
                char *msg = "Not subscribed to channel: ";
                char *message_ptr = parseMessage(msg, request->channelID);

                message_t errorMessage = createStatusResponseMessage(message_ptr, request->clientID);

                if (&errorMessage.messageID != NULL)
                {
                    response_t response = createServerErrorResponse(&errorMessage);
                    sendResponse(response, new_fd);
                    return 0;
                }
            }
        }
        else
        { // invalid channel id

            char *msg = "Invalid channel: ";
            char *message_ptr = parseMessage(msg, request->channelID);

            message_t errorMessage = createStatusResponseMessage(message_ptr, request->clientID);

            if (&errorMessage.messageID != NULL)
            {
                response_t response = createServerErrorResponse(&errorMessage);
                sendResponse(response, new_fd);
            }
        }
    }

    if (request->commandID == LIVEFEED && request->channelID == NO_CHANNEL && request->liveFeedFlag == LIVEFEED_FLAG_TRUE || request->liveFeedFlag == LIVEFEED_FLAG_FALSE)
    { // LIVEFEED no id
        int count = 0;
        channel_t *channel;

        int notSubscribed = isClientSubscribedToAnyChannel(request->clientID);

        if (notSubscribed == 1)
        { // client not subscribed to any channel

            char *message = "Not subscribed to any channels.";

            // CREATE A response.errorMessage property to client with the message in printf? TODO: bug.
            message_t noMsgErrorMessage = createStatusResponseMessage(message, request->clientID);

            if (&noMsgErrorMessage.messageID != NULL)
            {
                response_t response = createServerErrorResponse(&noMsgErrorMessage);
                sendResponse(response, new_fd);
                printf("\nerror response send \n");
            }

            return 0;
        }
        else
        {
            while (request->liveFeedFlag == LIVEFEED_FLAG_TRUE)
            { // loop while the livefeed flag is true in request

                // Client subscribed to at least one channel at this point.
                while (count <= MAX_CHANNELS)
                { // LOOP through each channel

                    channel = getChannel(count); // find the channel given channel id

                    if (channel != NULL)
                    {
                        client_t *client = findSubscriberInChannel(channel, request->clientID);

                        if (client != NULL)
                        {

                            if (client->unReadMsg > 0)
                            { // send reponse if there are unread Messages
                                printf("\nlivefeed true\n");
                                message_t unreadMessage = readNextMsgFromChannel(channel, client);

                                if (&unreadMessage.messageID != NULL)
                                { // MOTE: NEXT NULL BECAUSE OUTER IF STATEMENT IS TRUE i.e. client->unReadMsg > 0

                                    // create server response to client WITH MESSAGE
                                    response_t response = createServerResponse(&unreadMessage, client, channel->channelID, -1, LIVEFEED_FLAG_TRUE);

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
                        }
                    }

                    count++; // increment to next channel id

                    if (count == MAX_CHANNELS)
                    { // reset channel count infinitely
                        count = 0;
                    }
                }
            }
        }
        return 0;
    }

    if (request->commandID == BYE)
    { // BYE command request sent by client
        printf("unsubscribe client from all channels..\n");

        int count = 0;
        while (count < MAX_CHANNELS)
        { // loop each channel
            channel_t *channel = getChannel(count);

            if (channel != NULL)
            {
                client_t *client = findSubscriberInChannel(channel, request->clientID);

                if (client != NULL)
                { // unscribe client if subscribed to channel.
                    printf("\nUnsubscribed from channel id %d\n", channel->channelID);
                    unSubscribe(channel->channelID, request->clientID);
                }
            }

            count++;
        }
    }

    return 0;
}

/***********************************************************************
 * func:            generates a client_t struct when a new client connects
 *                  to server
 *                 
 * param child_id:  the process id of the process is assiged as client's id
 *                  and returned
***********************************************************************/
client_t generate_client(int child_id)
{
    client_t *client = malloc(sizeof(client_t));

    client->clientID = child_id;
    client->entryIndexConstant = 0;
    client->messageQueueIndex = 0;
    client->readMsg = 0;
    client->totalMessageSent = 0;
    client->unReadMsg = 0;
    client->status = CLIENT_INACTIVE;

    return *client;
}

/********************************************************************************
 * func:            prints channel's properties with subscribers (for debugging)
 * 
*********************************************************************************/
void print_channels_with_subscribers()
{

    int i = 0;
    int j = 0;
    printf("\n-----CHANNELS WITH SUBSCRIBER(s) ---------\n");
    for (i = 0; i < MAX_CHANNELS; i++)
    {
        for (j = 0; j < MAX_CLIENTS; j++)
        {
            if (sharedChannels->hostedChannels[i].subscribers[j].status == CLIENT_ACTIVE)
            {
                printf("\n\nChannel id %d\n", sharedChannels->hostedChannels[i].channelID);
                printf("\nChannel SUB count %d\n", sharedChannels->hostedChannels[i].subscriberCount);
                printf("Client id: %d\n", sharedChannels->hostedChannels[i].subscribers[j].clientID);
                // printf("subscriber status: %d\n", sharedChannels->hostedChannels[i].subscribers[j].status);
                printf("subscriber count: %d\n", sharedChannels->hostedChannels[i].subscriberCount);

                printf("\n \t\t =>  entryIndexConstant %d\n", sharedChannels->hostedChannels[i].subscribers[j].entryIndexConstant);
                printf("\n \t\t =>  messageQueueIndex %d\n", sharedChannels->hostedChannels[i].subscribers[j].messageQueueIndex);
                printf("\n \t\t =>  readMsg %d\n", sharedChannels->hostedChannels[i].subscribers[j].readMsg);
                printf("\n \t\t =>  unReadMsg %d\n", sharedChannels->hostedChannels[i].subscribers[j].unReadMsg);
                printf("\n \t\t =>  totalMessageSent %d", sharedChannels->hostedChannels[i].subscribers[j].totalMessageSent);
            }
        }
    }
}

/*********************************************************************************
 * func:            returns count of total number of subcribers in a given channe
 *                 
 * param *channel:  pointer to the channel whose subscribers are to be counted
**********************************************************************************/
int subscriberCountInChannel(channel_t *channel)
{
    int subscriberCount = 0;

    subscriberCount = sizeof(channel->subscribers) / sizeof(client_t);

    return subscriberCount;
}

/*********************************************************************************
 * func:            Calculate the total number of next unread message client has 
 *                  in all the channels it is subscribed too. Returns an integer 
 *                  that is the total number of next Unread Message in all 
 *                  channels for given client
 *                 
 * param client_id:  takes client id for a given client
**********************************************************************************/
int getNextNthMessageCount(int client_id)
{
    int count = 0;
    int channelCount = 0;

    client_t *client;
    channel_t *channel;

    while (channelCount < MAX_CHANNELS)
    {
        channel = getChannel(channelCount);

        if (channel != NULL)
        {
            client = findSubscriberInChannel(channel, client_id);

            if (client != NULL)
            { // client found in one of the channels
                if (client->unReadMsg > 0)
                {
                    count += 1;
                }
            }
        }

        channelCount++;
    }
    printf("\n\t ||||=>> COUNT  %d\n", count); // print to server.

    return count;
}

/*********************************************************************************
 * func:            Checks if a client is subscribed ANY channel. Returns 0 false 
 *                  or 1 if true.
 *                 
 * param client_id: client id of the given client
**********************************************************************************/
int isClientSubscribedToAnyChannel(int client_id)
{
    int channelCount = 0;
    client_t *client;
    channel_t *channel;

    while (channelCount <= MAX_CHANNELS)
    {
        channel = getChannel(channelCount);

        if (channel != NULL)
        {
            client = findSubscriberInChannel(channel, client_id);

            if (client != NULL)
            { // client found in one of the channels
                return 0;
            }
        }

        channelCount++;
    }
    return 1; // client not sub'd to any channels
}
/**************************************************************************************
 * func:             Parses integer channel id and char message into one char message.
 *                   Returns a char pointer to the message concatenated int channel id 
 *                   and char message.
 *                 
 * param *msg:       char message to send the client
 * 
 * param channel_id: channel id to concatenate with the char message
***************************************************************************************/
char *parseMessage(char *msg, int channel_id)
{
    // convert channel id given from int to string
    char str_channel_id[MAX_MESSAGE_LENGTH];
    snprintf(str_channel_id, sizeof(int), "%d", channel_id);

    // create error message
    char *message = (char *)malloc(sizeof(char) * MAX_MESSAGE_LENGTH); // create message var with memory and return pointer
    strcpy(message, msg);

    strcat(message, str_channel_id); /* concate id and string message */

    return message;
}

/**************************************************************************************
 * func:             Removes a client struct from a given channel. Critical section: 
 *                   implements a mutex locking to ensure one process has access to the 
 *                   shared memory i.e. channels.
 *                 
 * param client_id:  client id of the subscriber to remove from channel
 * 
 * param channel_id: channel id of the channel to remove client from 
***************************************************************************************/
int unSubscribe(int channel_id, int client_id)
{ // removing client from array

    // critical section - removing the correct index in subscribers list in shared memory
    pthread_mutex_lock(&mutex_lock);

    channel_t *channel = getChannel(channel_id);
    printf("\nchannel found %d\n", channel->channelID);

    int i, j = 0;

    for (i = 0; i < MAX_CHANNELS; i++)
    {
        for (j = 0; j < MAX_CLIENTS; j++)
        {
            if (channel->subscribers[i].clientID == client_id)
            {
                printf("\n true ids found.\n");

                int pos = i; // pos = index to delete

                for (i = pos - 1; i < channel->subscriberCount; i++)
                {                                                          // i = previous element to the one being deleted
                    channel->subscribers[i] = channel->subscribers[i + 1]; // moved to the index of the element to be deleted.
                }

                channel->subscriberCount--; // reduce the length to show updated array

                pthread_mutex_unlock(&mutex_lock);

                return 0;
            }
        }
    }

    return 0;
}

/*********************************************************************************************
 * func:             Returns a pointer to the channel in shared memory i.e. arrays of channels
 *                 
 * param channelID:  Channel id of the channel to be returned 
***********************************************************************************************/
channel_t *getChannel(int channelID)
{
    int i;
    channel_t *channel = NULL;

    for (i = 0; i < MAX_CHANNELS; i++)
    {
        if (sharedChannels->hostedChannels[i].channelID == channelID)
        { // if channel id matches one in hosted channels array
            channel = &sharedChannels->hostedChannels[i];
        }
    }

    return channel;
}

/*******************************************************************************************
 * func:                       Sends a response to client server i.e. response_t struct 
 *                             using socket send() func. Returns 0 is successfully sent.
 *                 
 * param serverResponse:       The reponse_t struct to be sent over socket to client server
 * 
 * param sock_id:              socket descriptor
********************************************************************************************/
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
/*******************************************************************************************
 * func:                       Creates a reponse_t struct with the given "error" message to 
 *                             send client. Returns a pointer to reponse object
 *                 
 * param *message:             Takes message_t struct to be assigned the response object
********************************************************************************************/
response_t createServerErrorResponse(message_t *message)
{
    response_t *response = malloc(sizeof(response_t));

    response->message = *message;
    response->error = 1;

    return *response;
}

/**************************************************************************************************
 * func:                       Creates a reponse_t struct for a successful reponse to 
 *                             send client. Successful reponse e.g. sending a message from channel
 *                 
 * param *message:             Message pointer to message to be sent to client
 * 
 * param *client:              Takes pointer to client struct whose response is being created and sent
 * 
 * param channel_id:           Id of channel whose message is being sent to client
 * 
 * param unReadMessageCount:   Total count of unread messages 
 * 
 * param livefeedFlag:         int bool is the response being created is for a livefeed
***************************************************************************************************/
response_t createServerResponse(message_t *message, client_t *client, int channel_id, int unReadMessageCount, int livefeedFlag)
{
    response_t *response = malloc(sizeof(response_t));

    if (unReadMessageCount != 0)
    {
        response->unReadMessagesCount = unReadMessageCount;
    }

    if (message != NULL)
    { // if the response is being sent with a message
        response->clientID = client->clientID;
        response->message = *message;
        response->error = 0;
        response->channel_id = channel_id;
        response->liveFeedFlag = livefeedFlag;
    }
    else
    { // if no message is being sent error status is set to true i.e. no message in this server response
        response->clientID = client->clientID;
        response->error = 1;
    }

    return *response;
}
/**************************************************************************************************
 * func:                       Prints the client_t struct properties of all clients subscribed to 
 *                             any channel (for debugging)
***************************************************************************************************/
void print_subscribers_all()
{
    int i, j = 0;
    printf("\n\n==== All Subscribers =====\n\n");

    for (i = 0; i < MAX_CHANNELS; i++)
    {
        for (j = 0; j < MAX_CLIENTS; j++)
        {
            if (sharedChannels->hostedChannels[i].subscribers[j].status == CLIENT_ACTIVE)
            {
                printf("Client id: %d\n", sharedChannels->hostedChannels[i].subscribers[j].clientID);

                // printf("\t Subscriber messageQueueIndex %d \n", channel->subscribers[j].messageQueueIndex);
                // printf("\t Subscriber entryIndexConstant %d \n", channel->subscribers[j].entryIndexConstant);
                // printf("\t Subscriber readMsg %d \n", channel->subscribers[j].readMsg);
                // printf("\t Subscriber unReadMsg %d \n", channel->subscribers[j].unReadMsg);
                // printf("\t Subscriber totalMessageSent %d \n", channel->subscribers[j].totalMessageSent);
            }
        }
    }

    printf("====================== \n");
}
/**************************************************************************************************
 * func:                       Updates in the given channel each client's unReadMsg count when new
 *                             message has been added in the channel by one of the subscriber of channel.
 *                 
 * param *channel:             channel pointer to the channel subscribers being updated
***************************************************************************************************/
void updateUnreadMsgCountForSubscribers(channel_t *channel)
{
    int i, j = 0;

    for (i = 0; i < MAX_CHANNELS; i++)
    {
        if (sharedChannels->hostedChannels[i].channelID == channel->channelID)
        { // update unread count for all subscribers of the given channel
            for (j = 0; j < MAX_CLIENTS; j++)
            {
                sharedChannels->hostedChannels[i].subscribers[j].unReadMsg += 1;
            }
        }
    }
}
/**************************************************************************************************
 * func:                       Checks if a client (subscriber) exists in the given channel
 *                 
 * param *channel:             Pointer to the given channel
 * 
 * param clientID:             client's id to be searched in the given channel
***************************************************************************************************/
client_t *findSubscriberInChannel(channel_t *channel, int clientID)
{
    int i = 0;
    client_t *client = NULL;

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        if (clientID == channel->subscribers[i].clientID)
        {
            client = &channel->subscribers[i];
            return client;
        }
    }

    return client;
}

/**************************************************************************************************
 * func:                       Adds the given client passed in to the channel and "subscribed".
 *                             Implements critical section mutex lock to ensure only one process has
 *                             access to update shared memory of channel arrays at a time.
 *                 
 * param *client:              Pointer to the client struct being added to the channels subscriber array
 *                             list.
 * 
 * param channel_id:           Channel id of the channel to be updated
***************************************************************************************************/
void subscribe(client_t *client_, int channel_id)
{
    // CRITICAL SECTION - writing to shared memory hostedChannels subscriber array
    pthread_mutex_lock(&mutex_lock);

    int i = 0;

    client_t *client = malloc(sizeof(client_t));
    *client = *client_;

    for (i = 0; i < MAX_CHANNELS; i++)
    {
        if (sharedChannels->hostedChannels[i].channelID == channel_id)
        { // if channel id matches

            // current subscriber count in the channel
            int subscriberCount = sharedChannels->hostedChannels[i].subscriberCount;

            // update the client message queue to current message count in channel
            client->entryIndexConstant = sharedChannels->hostedChannels[i].messagesCount;
            client->messageQueueIndex = sharedChannels->hostedChannels[i].messagesCount;

            // update client status to active
            client->status = CLIENT_ACTIVE;

            // add client to the next free index
            sharedChannels->hostedChannels[i].subscribers[subscriberCount] = *client;

            printf("### Subscriber Count:: %d\n", sharedChannels->hostedChannels[i].subscriberCount);
            printf("### Subscriber ID: %d\n", sharedChannels->hostedChannels[i].subscribers[subscriberCount].clientID);

            printf("### Subscriber entryIndexConstant: %d\n", sharedChannels->hostedChannels[i].subscribers[subscriberCount].entryIndexConstant);
            printf("### CHANNEL messageCount: %d\n", sharedChannels->hostedChannels[i].messagesCount);
            printf("### Subscriber messageQueueIndex: %d\n", sharedChannels->hostedChannels[i].subscribers[subscriberCount].messageQueueIndex);

            // update channels subscriber count
            sharedChannels->hostedChannels[i].subscriberCount += 1;
        }
    }
    // unlock mutex
    pthread_mutex_unlock(&mutex_lock);
}
/*************************************************************************************************************
 * func:                      Adds a new message to the given channel in the shared memory of channels array.
 *                            Implements critical section mutex lock because func updates shared memory
 *                 
 * param *request:            Takes the request object pointer sent by the client server that contains the message
 * 
 * param *channel:            Takes pointer to the channel in shared memory to be updated with a new message
**************************************************************************************************************/
void writeToChannelReq(request_t *request, channel_t *channel)
{
    // critical section
    pthread_mutex_lock(&mutex_lock);

    int i, j = 0;

    message_t *client_message = malloc(sizeof(message_t));
    *client_message = request->message;

    for (i = 0; i < MAX_CHANNELS; i++)
    {
        for (j = 0; j < MAX_CLIENTS; j++)
        {
            if (sharedChannels->hostedChannels[i].channelID == request->channelID && sharedChannels->hostedChannels[i].messagesCount <= MAX_MESSAGE_COUNT)
            {

                // 1. Update Message ID: message id is equal to current total message count in channel (starting from 1)
                client_message->messageID = sharedChannels->hostedChannels[i].messagesCount + 1;

                // 2. Add message to channel
                sharedChannels->hostedChannels[i].messages[sharedChannels->hostedChannels[i].messagesCount] = *client_message;

                // 3. update total message in channel (before adding message to ensure correct message id given)
                sharedChannels->hostedChannels[i].messagesCount += 1;

                break;
            }
        }
    }

    // // works: update unreadMsg property for all subcribed clients except client with this clientID
    updateUnreadMsgCountForSubscribers(channel);

    // // end of critical section
    pthread_mutex_unlock(&mutex_lock);
}

/**************************************************************************************************
 * func:                       Prints all the messages from channels that have atleast one message
 *                             (for debuggin)
***************************************************************************************************/
void print_channel_messages()
{
    int i, j = 0;
    printf("\n==== All Channels Messages ===============================================\n");

    for (i = 0; i < MAX_CHANNELS; i++)
    {
        if (sharedChannels->hostedChannels[i].messagesCount > 0)
        {
            for (j = 0; j < MAX_MESSAGE_COUNT; j++)
            {
                if (sharedChannels->hostedChannels[i].messages[j].messageID >= 1)
                {
                    printf("ChannelID: %d, MessageID: %d | ClientID %d, Content: %s \n",
                           sharedChannels->hostedChannels[i].channelID,
                           sharedChannels->hostedChannels[i].messages[j].messageID,
                           sharedChannels->hostedChannels[i].messages[j].ownerID,
                           sharedChannels->hostedChannels[i].messages[j].content);
                }
            }
        }
    }
}
/**********************************************************************************************************
 * func:                       Returns the next unread message in the channel. Implements crtical section
 *                             as the client's unRead and read message count, and the next message in queue 
 *                             is updated (in shared memory).
 *                 
 * param *channel:             Takes pointer to channel to be searched for next unread message
 * 
 * param *client:              Takes pointer to client struct whose being sent their unread message.
***********************************************************************************************************/
message_t searchNextMsgInList(channel_t *channel, client_t *client)
{
    // critical section - updating the client's next queues
    pthread_mutex_lock(&mutex_lock);

    int i, j, k = 0;

    message_t unreadMessage;
    int clientsNextMessageToReadIndex = client->messageQueueIndex + 1;

    for (i = 0; i < MAX_CHANNELS; i++)
    {
        if (sharedChannels->hostedChannels[i].channelID == channel->channelID)
        { // if channel id match
            for (k = 0; k < MAX_CLIENTS; k++)
            {
                if (sharedChannels->hostedChannels[i].subscribers[k].clientID == client->clientID)
                { // if client id also matches
                    for (j = 0; j < MAX_MESSAGE_COUNT; j++)
                    {
                        if (sharedChannels->hostedChannels[i].messages[j].messageID == clientsNextMessageToReadIndex)
                        { // next message in the client's unread index

                            // if messageID is the next in messageQueueIndex for client
                            unreadMessage = sharedChannels->hostedChannels[i].messages[j];
                            sharedChannels->hostedChannels[i].subscribers[k].messageQueueIndex += 1; // update messageQueueIndex

                            sharedChannels->hostedChannels[i].subscribers[k].readMsg += 1;   // update read messages count
                            sharedChannels->hostedChannels[i].subscribers[k].unReadMsg -= 1; // update unread messages count
                        }
                    }
                }
            }
        }
    }
    // unlock mutex
    pthread_mutex_unlock(&mutex_lock);

    return unreadMessage;
}
/**********************************************************************************************************
 * func:                       Calls searchNextMsgInList (above func.) and returns the unreadMessage
 *                 
 * param *channel:             Takes pointer to channel to be searched for next unread message
 * 
 * param *client:              Takes pointer to client struct whose being sent their unread message.
***********************************************************************************************************/
message_t readNextMsgFromChannel(channel_t *channel, client_t *client)
{

    message_t unreadMessage = searchNextMsgInList(channel, client);

    return unreadMessage;
}
/**********************************************************************************************************
 * func:                       Creates a successful status response message to be sent to the server
 *                             i.e. used when the server successfully handles client's request.
 *                 
 * param *message:             Message to be sent in the server response
 * 
 * param clientID:             Client id to be added in the server response message.
***********************************************************************************************************/
message_t createStatusResponseMessage(char *message, int clientID)
{

    message_t statusMessage;

    statusMessage.messageID = 1;
    statusMessage.ownerID = clientID;

    strcpy(statusMessage.content, message);

    return statusMessage;
}

/**********************************************************************************************************
 * func:                      Thread function used when client requests with NEXT command. The thread sends
 *                            next unread messages to the client. By using a thread the server is able to 
 *                            and is ready to handle other incoming requests efficiently.
 *                 
 * param *ptr:                void pointer points to thread data
 ************************************************************************************************************/
void handleNextSend(void *ptr)
{

    thdata *data;
    data = (thdata *)ptr; /* type cast to a pointer to thdata */

    int count = 0;
    channel_t *channel;

    int totalUnreadMessageCount = 0;

    int notSubscribed = isClientSubscribedToAnyChannel(data->clientID);

    if (notSubscribed == 1)
    { // client not subscribed to any channel

        char *message = "Not subscribed to any channels.";

        // CREATE A response.errorMessage property to client with the message in printf? TODO: bug.
        message_t noMsgErrorMessage = createStatusResponseMessage(message, data->clientID);

        if (&noMsgErrorMessage.messageID != NULL)
        {
            response_t response = createServerErrorResponse(&noMsgErrorMessage);
            sendResponse(response, new_fd);
            printf("\nerror response send \n");
        }
    }
    else if ((totalUnreadMessageCount = getNextNthMessageCount(data->clientID) == 0))
    {
        // If not messages in any channel send "no message" to client
        char *message = "No new messages found in channel!";

        // CREATE A response.errorMessage property to client with the message in printf? TODO: bug.
        message_t noMsgErrorMessage = createStatusResponseMessage(message, data->clientID);

        if (&noMsgErrorMessage.messageID != NULL)
        {
            response_t response = createServerErrorResponse(&noMsgErrorMessage);
            sendResponse(response, new_fd);
        }
    }
    else
    {
        // returns an integer that is the total number of next Unread Message in all channels for given client
        totalUnreadMessageCount = getNextNthMessageCount(data->clientID);

        // Client subscribed to at least one channel at this point.
        while (count <= MAX_CHANNELS)
        {
            channel = getChannel(count); // find the channel given channel id

            if (channel != NULL)
            {
                client_t *client = findSubscriberInChannel(channel, data->clientID);

                if (client != NULL)
                {
                    if (client->unReadMsg >= 1 && totalUnreadMessageCount != 0)
                    {
                        message_t unreadMessage = readNextMsgFromChannel(channel, client);

                        if (&unreadMessage.messageID != NULL)
                        {
                            // create server response to client WITH MESSAGE
                            response_t response = createServerResponse(&unreadMessage, client, channel->channelID, totalUnreadMessageCount, LIVEFEED_FLAG_FALSE);

                            if (response.error == 0)
                            {
                                printf("\n________________________ \n");
                                printf("NEXT %d Message Sending...|\n", data->channel_id);
                                printf("__________________________ \n");

                                printf("\nClientID: %d\n", response.clientID);
                                printf("Next message : %s\n", response.message.content);
                                printf("UnReadMessageCOUNT : %d\n", response.unReadMessagesCount);
                                printf("Error status : %d\n", response.error);
                                printf("\n");
                            }
                            // send response to client
                            sendResponse(response, new_fd);
                            sleep(2);

                            totalUnreadMessageCount--; // decrement the total count of next Unread messages client server needs to wait for
                        }
                    }
                }
            }

            count++; // increment to next channel id
        }
    }
}

/**********************************************************************************************************
 * func:                      Gracefully shutsdowns server closes socks, descriptors, threads etc.
 ************************************************************************************************************/
void shut_down_handler()
{
    pthread_cancel(thread1);
    close(new_fd);
    close(sockfd);

    sleep(1);
    write(1, "\n\nShutting down gracefully, BYE!\n\n", 35);
    exit(1);
}
