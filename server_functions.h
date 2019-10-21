#ifndef __SERVER_H__
#define __SERVER_H__

#define MAX_MESSAGE_LENGTH 1024

#define MAX_CLIENTS 100

#define MAX_MESSAGE_COUNT 1000

typedef struct message message_t;

struct message
{
    int messageID;
    int ownerID;
    char content[MAX_MESSAGE_LENGTH];
    // message_t *next;
};

message_t client_message;

typedef struct request request_t;

struct request
{
    int commandID; /* integer enum representing command i.e. SUB, NEXT*/
    int channelID;
    int clientID;
    message_t message;
    int liveFeed;
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
    int status;
    // client_t *next;
};

typedef struct channel channel_t;

struct channel
{
    int channelID;
    int totalMsg;
    message_t messages[MAX_MESSAGE_COUNT];
    client_t subscribers[MAX_CLIENTS];
    int subscriberCount;
    int messagesCount;
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
    CHANNEL FUNCTIONS (INCLUDE IN SEPERATE FILE LATER)
*/

// Creates 256 channels for global hostedChannels array
channel_t *generateHostChannels();

client_t generate_client(int child_id); // cretes inactive client objects to the channels
int randomClientIdGenerator();

void print_channels_with_subscribers();

// when subscribing to channel, finds out the last message in the channel, and will be used to add the message id to the subscribing client
message_t createStatusResponseMessage(char *message, int clientID);
response_t createServerErrorResponse(message_t *message);

// // SUBSCRIBE AND UNSUBSCRIBE FUNCS
// message_t *findLastMsgInLinkedList(channel_t channel);

void subscribe(client_t *client_, int channel_id);

int unSubscribe(int channel_id, int client_id);

char *parseMessage(char *msg, int channel_id);

client_t *findSubscriberInChannel(channel_t *channel, int clientID);

int subscriberCountInChannel(channel_t *channel);

void print_subscribers_all();

void updateUnreadMsgCountForSubscribers(channel_t *channel);

void writeToChannelReq(request_t *request, channel_t *channel);

void print_channel_messages();

message_t searchNextMsgInList(channel_t *channel, client_t *client);

message_t readNextMsgFromChannel(channel_t *channel, client_t *client);

int isClientSubscribedToAnyChannel(int client_id);

int getNextNthMessageCount(int client_id);

response_t createServerResponse(message_t *message, client_t *client, int channel_id, int unReadMessageCount);
int sendResponse(response_t serverResponse, int sock_id);

channel_t *getChannel(int channelID);

int handleClientRequests(request_t *request, client_t *client);

// int parseRequest(int new_fd, char *clientRequest, char *user_command, char *channel_id);

// message_t readNextMsgFromAllChannel(int clientID); // next no id
// int nextMessageCountInChannels(int clientID);

/*
    END OF
*/

#endif //__SERVER_H__