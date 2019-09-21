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

typedef struct message message_t;

struct message
{
    int messageID;
    int ownerID;
    char content[MAX_MESSAGE_LENGTH];
    message_t *next;
};

typedef struct channel channel_t;

struct channel
{
    int channelID;
    int totalMsg;
    message_t *messageHead;
    client_t *subscriberHead;
    client_t *subscriberTail;
};

channel_t hostedChannels[MAX_CHANNELS]; /* Global array so its in stack memory where child processes can access shared mem?*/

/*
    CHANNEL FUNCTIONS (INCLUDE IN SEPERATE FILE LATER)
*/
// when subscribing to channel, finds out the last message in the channel, and will be used to add the message id to the subscribing client
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

// subscribe client to channel
void subscribe(client_t *client, int channel_id)
{
    int i = 0;
    channel_t channel;
    client_t *currentNode;
    message_t *message;

    for (i = 0; i < 5; i++)
    {
        if (hostedChannels[i].channelID == channel_id)
        {
            printf("Channel selected found: %d\n", hostedChannels[i].channelID);
            channel = hostedChannels[i];
            currentNode = channel.subscriberHead;

            if (channel.subscriberHead == NULL)
            {
                printf("Selected channels subscriberHead is NULL\n");
                message = findLastMsgInLinkedList(channel);

                // message == null ? client.messageQueueIndex = 0 : client.messageQueueIndex = message.messageID;
                if (message == NULL)
                {
                    printf("message is NULL\n");
                    client->messageQueueIndex = 0;
                }
                else
                {
                    printf("Message is NOT NULL. ");
                    printf(" CHANNEL: messageHead id %d\n", message->messageID);
                    client->messageQueueIndex = message->messageID;
                }

                client->messageQueueIndex = channel.totalMsg;
                client->entryIndexConstant = channel.totalMsg; // assign entry index based on msg in list constant
                printf("....\n");
                channel.subscriberHead = client;
                channel.subscriberTail = channel.subscriberHead; // track the last node
                printf("CHANNEL SUBSCRIBER HEAD clinet id = %d\n", channel.subscriberHead->clientID);
            }
            else
            {

                printf("Selected channels subscriberHead is NOT NULL\n");

                while (currentNode->next != NULL)
                {
                    // if the next value null = last node
                    currentNode = currentNode->next;
                }
                message = findLastMsgInLinkedList(channel);
                // update the read message queue index for the above message read.
                // works too! message == null ? client.messageQueueIndex = 0 : client.messageQueueIndex = message.messageID;
                client->messageQueueIndex = channel.totalMsg;

                channel.totalMsg = channel.totalMsg;

                // assign the index value of the last msg as subbed entry (never changes!)
                client->entryIndexConstant = channel.totalMsg;

                currentNode->next = client;                 // on the last node add next client
                channel.subscriberTail = currentNode->next; // track the last node

                printf("..CHANNEL SUBSCRIBER HEAD clinet id = %d\n", channel.subscriberHead->clientID);
            }
        }
        else
        {
            printf("Channel id not found\n");
        }
    }
}

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
    // free allocated memory and open files
    sleep(1);
    write(1, "\n\nShutting down gracefully, BYE!\n\n", 35);
    exit(1);
}

void handleClientRequests(char *user_command, char *channel_id, int clientID);
int randomClientIdGenerator();

int parseRequest(int new_fd, char *clientRequest, char *user_command, char *channel_id);

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
    sin_size = sizeof(struct sockaddr_in);
    while (new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size))
    {
        printf("server: got connection from %s\n",
               inet_ntoa(their_addr.sin_addr));

        /* ***Server-Client Connected*** */

        printf("\n-------------------------------------------\n");

        int clientID; /* client id given to each connected client */
        clientID = randomClientIdGenerator();

        /* */
        char *msg = "This is message 1\0"; // USE MALLOC SO ITS ON HEAP, FOR EACH THREAD ACCESS, AND ENSRUE 1024 SIZE ??

        // Create a message
        message_t message1;
        message1.messageID = 1;
        message1.ownerID = 1;
        strcpy(message1.content, msg);
        message1.next = NULL;

        // Create a client 1
        client_t client1;

        client1.clientID = clientID;
        client1.readMsg = 0;
        client1.unReadMsg = 0;
        client1.totalMessageSent = 0;
        client1.entryIndexConstant = 0;
        client1.messageQueueIndex = 0;
        client1.next = NULL;

        client_t client2;

        client2.clientID = 2;
        client2.readMsg = 0;
        client2.unReadMsg = 0;
        client2.totalMessageSent = 0;
        client2.entryIndexConstant = 0;
        client2.messageQueueIndex = 0;
        client2.next = NULL;

        // Create a channel
        channel_t channel1;

        channel1.channelID = 1;
        channel1.totalMsg = 0;
        channel1.subscriberHead = NULL;
        channel1.subscriberTail = NULL;
        channel1.messageHead = NULL;

        hostedChannels[0] = channel1;

        // printf("Hosted Channel %d\n", hostedChannels[0].channelID);

        subscribe(&client1, 1);

        subscribe(&client2, 1);

        printf("Subcriber 1: id = %d\n", hostedChannels[0].subscriberHead->clientID);

        printf("\n-------------------------------------------\n");

        // Send: Welcome Message.
        if (send(new_fd, &clientID, sizeof(int), 0) == -1)
        {
            perror("send");
        }

        char clientRequest[1024]; /* client request buffer */

        // Receive: Command from User i.e. SUB, LIVEFEED etc.
        char user_command[100], channel_id[64];

        while (recv(new_fd, clientRequest, (sizeof(char) * 1024), 0))
        {

            if (parseRequest(new_fd, clientRequest, user_command, channel_id) == 0)
            {

                // // handle client request
                handleClientRequests(user_command, channel_id, clientID);
                printf("SERVER: successfully receive client command [userInput]!\n");
            }
            else
            {
                printf("\nSERVER: Error, failed to parse client command\n");
                perror("\nSERVER: Error, failed to parse client command\n");
            }
        }
    }

    close(new_fd);
    exit(0);
}

void handleClientRequests(char *user_command, char *channel_id, int clientID)
{
    // printf("\nResult:%s\n", user_command);
    // printf("Result:%d\n", atoi(channel_id));
    // printf("handleing request now...\n");

    int channelID = atoi(channel_id);
    char userCommand = *user_command;

    if (strncmp(user_command, "SUB", strlen("SUB")) == 0 && channel_id)
    {
        printf("\nSucribe() now to channel %d with %s command.\n", channelID, user_command);
        // subscribe(&client1, channelID);
    }
}

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

int randomClientIdGenerator()
{
    int num = rand() % MAX_CLIENTS;

    return num;
}

// int *Receive_Array_Int_Data(int socket_identifier, int size)
// {
//     int number_of_bytes, i = 0;
//     uint16_t statistics;

//     int *results = malloc(sizeof(int) * size);
//     for (i = 0; i < size; i++)
//     {
//         if ((number_of_bytes = recv(socket_identifier, &statistics, sizeof(uint16_t), 0)) == RETURNED_ERROR)
//         {
//             perror("recv");
//             exit(EXIT_FAILURE);
//         }
//         results[i] = ntohs(statistics);
//     }
//     return results;
// }