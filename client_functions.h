#ifndef __CLIENT_H__
#define __CLIENT_H__

#define MAX_MESSAGE_LENGTH 1024

/*
    STRUCTS
*/
typedef struct request request_t;

typedef struct message message_t;

/* struct holds a message with message id, the client's id, and the message content
*/
struct message
{
    int messageID;
    int ownerID;
    char content[MAX_MESSAGE_LENGTH];
};

/* struct that creates a request to be sent to the server
*/
struct request
{
    int commandID; /* integer enum representing command i.e. SUB, NEXT*/
    int channelID;
    int clientID;
    message_t client_message;
    int liveFeedFlag;
};

typedef struct response response_t;

/* struct to the reponse sent by server
*/
struct response
{
    int clientID;
    message_t message;
    int error;
    int channel_id;
    int unReadMessagesCount;
    int liveFeedFlag
};

/* struct to hold data to be passed to a thread
*/
typedef struct str_thdata
{
    int thread_no;
    int clientID;
    int channel_id;
    int sockfd;
} thdata;

void display_options();

void display_menu();

int sendRequest(request_t clientRequest, int sock_id);

request_t createRequest(int command, int channel_id, int clientID, message_t *message, int liveFeedFlag);

int connection_success(int sock_id);

int parseUserMessage(message_t *client_message, char *user_msg_ptr, int clientID);

void bye(int userCommand, int id_inputted, int clientID, message_t client_message, int liveFeedFlag, int sockfd);

void handleNextSendClient(void *ptr);

void handleNextReceiveClient(void *ptr);
void handleMessageReceiveClient(void *ptr);

#endif //__CLIENT_H__