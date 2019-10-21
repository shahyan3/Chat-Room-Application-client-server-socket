#ifndef __CLIENT_H__
#define __CLIENT_H__

#define MAX_MESSAGE_LENGTH 1024

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

void display_options();

void display_menu();

int sendRequest(request_t clientRequest, int sock_id);

request_t createRequest(int command, int channel_id, int clientID, message_t *message, int liveFeedFlag);

int connection_success(int sock_id);

int parseUserMessage(message_t *client_message, char *user_msg_ptr, int clientID);

#endif //__CLIENT_H__