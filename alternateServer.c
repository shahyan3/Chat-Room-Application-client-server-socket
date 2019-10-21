// // VERSION 1 - LINKED LIST DESIGN

// #define _XOPEN_SOURCE // sigaction obj needs this to work...man 7 feature_test_macros
// #define STDOUT_FILENO 1;

// #include <signal.h>

// #include <arpa/inet.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <errno.h>
// #include <string.h>
// #include <sys/types.h>
// #include <netinet/in.h>
// #include <sys/socket.h>
// #include <sys/wait.h>
// #include <unistd.h>
// #include <errno.h>

// #define ARRAY_SIZE 30 /* Size of array to receive */

// #define BACKLOG 10 /* how many pending connections queue will hold */

// #define RETURNED_ERROR -1

// #define MAX_CLIENTS 20

// #define MAX_CHANNELS 4

// #define MAX_MESSAGE_LENGTH 1024

// #define NO_CHANNEL -1

// /* COMMANDS */
// #define SUB 0
// #define UNSUB 1
// #define NEXT_ID 2
// #define LIVEFEED_ID 3
// #define SEND 4

// #define NEXT 5
// #define LIVEFEED 6

// /* BOOLEAN */
// #define LIVEFEED_TRUE 0
// #define LIVEFEED_FALSE 1

// int sockfd;
// int new_fd; /* listen on sock_fd, new connection on new_fd */

// typedef struct message message_t;

// struct message
// {
//     int messageID;
//     int ownerID;
//     char content[MAX_MESSAGE_LENGTH];
//     message_t *next;
// };

// message_t client_message;

// typedef struct request request_t;

// struct request
// {
//     int commandID; /* integer enum representing command i.e. SUB, NEXT*/
//     int channelID;
//     int clientID;
//     message_t message;
//     int liveFeed;
// };

// request_t clientRequest; /* move into main ??*/

// typedef struct client client_t;

// struct client
// {
//     int clientID;
//     int readMsg;
//     int unReadMsg;
//     int totalMessageSent;
//     int messageQueueIndex;  /* index = message that it read currently, NOT NEXT ONE */
//     int entryIndexConstant; /* index of msg queue at which the client subed */
//     client_t *next;
// };

// typedef struct channel channel_t;

// struct channel
// {
//     int channelID;
//     int totalMsg;
//     message_t *messageHead;
//     message_t *messageTail;
//     client_t *subscriberHead;
//     client_t *subscriberTail;
// };

// channel_t hostedChannels[MAX_CHANNELS]; /* Global array so its in stack memory where child processes can access shared mem?*/

// typedef struct response response_t;

// struct response
// {
//     int clientID;
//     message_t message;
//     int error;
//     int channel_id;
//     int unReadMessagesCount;
// };

// /*
//     CHANNEL FUNCTIONS (INCLUDE IN SEPERATE FILE LATER)
// */
// // when subscribing to channel, finds out the last message in the channel, and will be used to add the message id to the subscribing client
// message_t createStatusResponseMessage(char *message, int clientID);
// response_t createServerErrorResponse(message_t *message);

// // SUBSCRIBE AND UNSUBSCRIBE FUNCS
// message_t *findLastMsgInLinkedList(channel_t channel);

// void subscribe(client_t *client_, int channel_id);

// int unSubscribe(int channel_id, int client_id);

// char *parseMessage(char *msg, int channel_id);

// client_t *findSubscriberInChannel(channel_t *channel, int clientID);

// void print_subscribers();

// void updateUnreadMsgCountForSubscribers(channel_t *channel);

// void writeToChannelReq(request_t *request, channel_t *channel);

// void print_channel_messages(int channel_id);

// void print_channels();

// message_t searchNextMsgInList(channel_t *channel, client_t *client);

// message_t readNextMsgFromChannel(channel_t *channel, client_t *client);

// int isClientSubscribedToAnyChannel(int client_id);

// int getNextNthMessageCount(int client_id);

// response_t createServerResponse(message_t *message, client_t *client, int channel_id, int unReadMessageCount);
// int sendResponse(response_t serverResponse, int sock_id);

// message_t readNextMsgFromAllChannel(int clientID); // next no id

// channel_t *getChannel(int channelID);

// int nextMessageCountInChannels(int clientID);

// /*
//     END OF
// */

// void shut_down_handler()
// {
//     // close threads
//     // close processes
//     // close sockets
//     // shared memory regions
//     // close(new_fd);
//     close(sockfd);
//     close(new_fd);

//     // free allocated memory and open files
//     // sleep(1);
//     write(1, "\n\nShutting down gracefully, BYE!\n\n", 35);
//     exit(1);
// }

// int create_client();

// int handleClientRequests(request_t *request, client_t *client);

// int randomClientIdGenerator();

// int parseRequest(int new_fd, char *clientRequest, char *user_command, char *channel_id);

// int main(int argc, char *argv[])
// {
//     int port = 12345;

//     // signal handling
//     struct sigaction sa;
//     sa.sa_handler = shut_down_handler;
//     sigaction(SIGINT, &sa, NULL);

//     struct sockaddr_in my_addr;    /* my address information */
//     struct sockaddr_in their_addr; /* connector's address information */
//     socklen_t sin_size;
//     int i = 0;

//     /* Get port number for server to listen on */
//     if (argc == 2)
//     {
//         // fprintf(stderr, "usage: client port_number\n");
//         // exit(1);
//         port = atoi(argv[1]);
//     }

//     /* generate the socket */
//     if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
//     {
//         perror("socket");
//         exit(1);
//     }

//     // SO_REUSEADDR - addresses server binding.
//     uint32_t yes = 1;
//     setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

//     /* generate the end point */
//     my_addr.sin_family = AF_INET;             /* host byte order */
//     my_addr.sin_port = htons(port);           /* short, network byte order */
//     my_addr.sin_addr.s_addr = INADDR_ANY;     /* auto-fill with my IP */
//     /* bzero(&(my_addr.sin_zero), 8);   ZJL*/ /* zero the rest of the struct */

//     /* bind the socket to the end point */
//     if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
//     {
//         perror("bind");
//         exit(1);
//     }

//     /* start listnening */
//     if (listen(sockfd, BACKLOG) == -1)
//     {
//         perror("listen");
//         exit(1);
//     }

//     printf("server starts listening on %d...\n", port);

//     /* repeat: accept, send, close the connection */
//     sin_size = sizeof(struct sockaddr_in);
//     while (new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size))
//     {
//         printf("server: got connection from %s\n",
//                inet_ntoa(their_addr.sin_addr));

//         /* ***Server-Client Connected*** */

//         printf("\n-------------------------------------------\n");
//         // Create a new client object
//         client_t client;
//         if (create_client(&client) == 1)
//         {
//             perror("SERVER: failed to create client object\n");
//             printf("SERVER: failed to create client object\n");
//         }
//         printf("This is server\n");

//         // Create a channel
//         channel_t channel0;
//         channel_t channel1;
//         channel_t channel2;
//         channel_t channel3;

//         channel0.channelID = 0;
//         channel0.totalMsg = 0;
//         channel0.subscriberHead = NULL;
//         channel0.subscriberTail = NULL;
//         channel0.messageHead = NULL;
//         channel0.messageTail = NULL;

//         channel1.channelID = 1;
//         channel1.totalMsg = 0;
//         channel1.subscriberHead = NULL;
//         channel1.subscriberTail = NULL;
//         channel1.messageHead = NULL;
//         channel1.messageTail = NULL;

//         channel2.totalMsg = 0;
//         channel2.channelID = 2;
//         channel2.subscriberHead = NULL;
//         channel2.subscriberTail = NULL;
//         channel2.messageHead = NULL;
//         channel2.messageTail = NULL;

//         channel3.totalMsg = 0;
//         channel3.channelID = 3;
//         channel3.subscriberHead = NULL;
//         channel3.subscriberTail = NULL;
//         channel3.messageHead = NULL;
//         channel3.messageTail = NULL;

//         hostedChannels[0] = channel0;
//         hostedChannels[1] = channel1;
//         hostedChannels[2] = channel2;
//         hostedChannels[3] = channel3;
//         // printf("Hosted Channel %d\n", hostedChannels[0].channelID);

//         printf("\n-------------------------------------------\n");

//         // Send: Welcome Message.
//         if (send(new_fd, &client.clientID, sizeof(int), 0) == -1)
//         {
//             perror("send");
//             printf("Error: Welcome message not sent to client \n");
//         }

//         while (recv(new_fd, &clientRequest, sizeof(request_t), 0))
//         {

//             if (handleClientRequests(&clientRequest, &client) == 0)
//             {
//                 // printf("\n SERVER: Request Object: \n command id %d\n channel id %d \n client id %d\n client message %s\n msg ID %d\n",
//                 //        clientRequest.commandID, clientRequest.channelID, clientRequest.clientID, clientRequest.message.content, clientRequest.message.messageID);

//                 printf("\n********************************************************\n");
//                 printf("SERVER: successfully receive client request [userInput] |\n");
//                 printf("********************************************************\n");
//             }
//             else
//             {
//                 printf("\n***********************************************\n");
//                 printf("SERVER: Bad request. Failed to handle client request |\n");
//                 printf("\n***********************************************\n");
//             }
//         }
//     }

//     // close(new_fd);
//     // exit(0);
// }

// int handleClientRequests(request_t *request, client_t *client)
// {
//     printf("\n --> command: %d \n", request->commandID);

//     if (request->commandID == SUB)
//     { // Subscribe to Channel request

//         channel_t *channel;
//         client_t *isClient;

//         if (request->channelID == NO_CHANNEL)
//         { // Client sent SUB with no id

//             // create error message
//             char *message = "No channel id given";

//             message_t errorMessage = createStatusResponseMessage(message, request->clientID);

//             if (&errorMessage != NULL)
//             {
//                 response_t response = createServerErrorResponse(&errorMessage);
//                 sendResponse(response, new_fd);
//             }
//             return 0;
//         }
//         else
//         {
//             channel = getChannel(request->channelID);

//             if (channel == NULL || channel->channelID != request->channelID)
//             { // Invalid channel id

//                 char *msg = "Invalid channel: ";
//                 char *message_ptr = parseMessage(msg, request->channelID);
//                 // printf("\n %s\n", message_ptr);

//                 message_t errorMessage = createStatusResponseMessage(message_ptr, request->clientID);

//                 if (&errorMessage != NULL)
//                 {
//                     response_t response = createServerErrorResponse(&errorMessage);
//                     sendResponse(response, new_fd);
//                 }
//             }
//             else
//             {
//                 isClient = findSubscriberInChannel(channel, request->clientID);

//                 if (isClient == NULL)
//                 {
//                     subscribe(client, request->channelID);

//                     print_subscribers();

//                     print_channels();

//                     char *msg = "Subscribed to channel: ";
//                     char *message_ptr = parseMessage(msg, request->channelID);

//                     message_t successMessage = createStatusResponseMessage(message_ptr, request->clientID);

//                     if (&successMessage != NULL)
//                     {
//                         response_t response = createServerResponse(&successMessage, client, request->channelID, -1);
//                         sendResponse(response, new_fd);
//                     }
//                 }
//                 else
//                 { // Client already subscribed

//                     char *msg = "Already subscribed to channel: ";
//                     char *message_ptr = parseMessage(msg, request->channelID);

//                     message_t errorMessage = createStatusResponseMessage(message_ptr, request->clientID);

//                     if (&errorMessage != NULL)
//                     {
//                         response_t response = createServerErrorResponse(&errorMessage);
//                         sendResponse(response, new_fd);
//                     }
//                 }
//             }
//         }

//         return 0;
//     }

//     else if (request->commandID == UNSUB)
//     { // UNSUB to Channel request

//         channel_t *channel;
//         client_t *isClient;

//         if (request->channelID == NO_CHANNEL)
//         { // Client sent SUB with no id

//             // create error message
//             char *message = "No channel id given";

//             message_t errorMessage = createStatusResponseMessage(message, request->clientID);

//             if (&errorMessage != NULL)
//             {
//                 response_t response = createServerErrorResponse(&errorMessage);
//                 sendResponse(response, new_fd);
//             }
//             return 0;
//         }
//         else
//         {
//             channel = getChannel(request->channelID);

//             if (channel == NULL || channel->channelID != request->channelID)
//             { // Invalid channel id

//                 char *msg = "Invalid channel: ";
//                 char *message_ptr = parseMessage(msg, request->channelID);

//                 message_t errorMessage = createStatusResponseMessage(message_ptr, request->clientID);

//                 if (&errorMessage != NULL)
//                 {
//                     response_t response = createServerErrorResponse(&errorMessage);
//                     sendResponse(response, new_fd);
//                 }
//             }
//             else
//             {
//                 isClient = findSubscriberInChannel(channel, request->clientID);

//                 if (isClient != NULL)
//                 { // unsubscribe client
//                     unSubscribe(request->channelID, isClient->clientID);

//                     print_subscribers();

//                     print_channels();

//                     char *msg = "Unsubscribed from channel: ";
//                     char *message_ptr = parseMessage(msg, request->channelID);

//                     message_t successMessage = createStatusResponseMessage(message_ptr, request->clientID);

//                     if (&successMessage != NULL)
//                     {
//                         response_t response = createServerResponse(&successMessage, client, request->channelID, -1);
//                         sendResponse(response, new_fd);
//                     }
//                 }
//                 else
//                 { // Client already subscribed

//                     char *msg = "Not subscribed to channel: ";
//                     char *message_ptr = parseMessage(msg, request->channelID);

//                     message_t errorMessage = createStatusResponseMessage(message_ptr, request->clientID);

//                     if (&errorMessage != NULL)
//                     {
//                         response_t response = createServerErrorResponse(&errorMessage);
//                         sendResponse(response, new_fd);
//                     }
//                 }
//             }
//         }

//         return 0;
//     }

//     if (request->commandID == SEND)
//     { // SEND message to channel
//         printf("$Command id %d and channel id %d\n", request->commandID, request->channelID);
//         int i;
//         channel_t *channel;

//         channel = getChannel(request->channelID);

//         if (channel != NULL)
//         { // NOTE: Client who has NOT* subscribe CAN send a message to a channel*

//             // client = findSubscriberInChannel(channel, request->clientID);

//             // if (client != NULL && client->clientID == request->clientID)
//             // { // client is subscribed to channel

//             writeToChannelReq(request, channel);

//             // send client response
//             char *message = "Message successfully sent";
//             message_t successMessage = createStatusResponseMessage(message, request->clientID);

//             if (&successMessage != NULL)
//             {
//                 response_t response = createServerErrorResponse(&successMessage);
//                 sendResponse(response, new_fd);
//             }
//             // }
//             // else
//             // {
//             //     char *message = "Error: no client subscribed to the channel.";
//             //     printf("\n %s\n", message);

//             //     // CREATE A response.errorMessage property to client with the message in printf? TODO: bug.
//             //     message_t errorMessage = createStatusResponseMessage(message, request->clientID);

//             //     if (&errorMessage != NULL)
//             //     {
//             //         response_t response = createServerErrorResponse(&errorMessage);
//             //         sendResponse(response, new_fd);
//             //     }
//             // }
//         }
//         else
//         {
//             printf("SEND: channel id null!! %d\n", request->channelID);

//             char *msg = "Invalid channel: ";
//             char *message_ptr = parseMessage(msg, request->channelID);

//             message_t errorMessage = createStatusResponseMessage(message_ptr, request->clientID);

//             if (&errorMessage != NULL)
//             {
//                 response_t response = createServerErrorResponse(&errorMessage);
//                 sendResponse(response, new_fd);
//             }
//         }

//         print_channels();
//         print_channel_messages(request->channelID);
//         // print_subscribers();

//         return 0;
//     }

//     if (request->commandID == NEXT_ID && request->channelID != NO_CHANNEL)
//     { // READ message to channel
//         printf("\nServer: NEXT WITH id %d request recevied.\n", request->channelID);

//         channel_t *channel;

//         channel = getChannel(request->channelID); // find the channel given channel id

//         if (channel != NULL)
//         {
//             client_t *client = findSubscriberInChannel(channel, request->clientID);

//             if (client != NULL)
//             {
//                 if (client->unReadMsg > 0)
//                 {
//                     message_t unreadMessage = readNextMsgFromChannel(channel, client);

//                     if (&unreadMessage != NULL)
//                     { // MOTE: NEXT NULL BECAUSE OUTER IF STATEMENT IS TRUE i.e. client->unReadMsg > 0

//                         // create server response to client WITH MESSAGE
//                         response_t response = createServerResponse(&unreadMessage, client, request->channelID, -1);

//                         if (response.error == 0)
//                         {
//                             printf("\n________________________ \n");
//                             printf("NEXT %d Message Sending...|\n", request->channelID);
//                             printf("__________________________ \n");

//                             printf("\nClientID: %d\n", response.clientID);
//                             printf("Next message : %s\n", response.message.content);
//                             printf("Error status : %d\n", response.error);
//                             printf("\n");
//                         }

//                         // send response to client
//                         sendResponse(response, new_fd);
//                     }
//                 }
//                 else
//                 {
//                     // note: ERROR MESSAGE LIKE THIS COMING FROM SOMEHWERE WITH NEXT command!!!

//                     char *message = "(NEXT with ID) No new messages found in channel!";
//                     // printf("\n %s\n", message);

//                     // // CREATE A response.errorMessage property to client with the message in printf? TODO: bug.
//                     message_t noMsgErrorMessage = createStatusResponseMessage(message, request->clientID);

//                     if (&noMsgErrorMessage != NULL)
//                     {
//                         response_t response = createServerErrorResponse(&noMsgErrorMessage);
//                         sendResponse(response, new_fd);
//                     }
//                 }
//             }
//             else
//             {

//                 char *msg = "Not subscribed to channel: ";
//                 char *message_ptr = parseMessage(msg, request->channelID);

//                 message_t errorMessage = createStatusResponseMessage(message_ptr, request->clientID);

//                 if (&errorMessage != NULL)
//                 {
//                     response_t response = createServerErrorResponse(&errorMessage);
//                     sendResponse(response, new_fd);
//                 }
//             }
//         }
//         else
//         {

//             char *msg = "Invalid channel: ";
//             char *message_ptr = parseMessage(msg, request->channelID);

//             message_t errorMessage = createStatusResponseMessage(message_ptr, request->clientID);

//             if (&errorMessage != NULL)
//             {
//                 response_t response = createServerErrorResponse(&errorMessage);
//                 sendResponse(response, new_fd);
//             }
//         }

//         print_subscribers();

//         return 0;
//     }

//     if (request->commandID == NEXT && request->channelID == NO_CHANNEL)
//     { // NEXT no id
//         printf("\nSERVER: Next WITHOUT <%d> request received.\n", request->channelID);

//         int count = 0;
//         channel_t *channel;

//         int totalUnreadMessageCount = 0;

//         int notSubscribed = isClientSubscribedToAnyChannel(request->clientID); // TODO TMRW

//         if (notSubscribed == 1)
//         { // client not subscribed to any channel

//             char *message = "Not subscribed to any channels.";

//             // CREATE A response.errorMessage property to client with the message in printf? TODO: bug.
//             message_t noMsgErrorMessage = createStatusResponseMessage(message, request->clientID);

//             if (&noMsgErrorMessage != NULL)
//             {
//                 response_t response = createServerErrorResponse(&noMsgErrorMessage);
//                 sendResponse(response, new_fd);
//                 printf("\nerror response send \n");
//             }

//             return 0;
//         }
//         else if ((totalUnreadMessageCount = getNextNthMessageCount(request->clientID) == 0))
//         {
//             // If not messages in any channel send "no message" to client
//             // returns an integer that is the total number of next Unread Message in all channels for given client

//             char *message = "(NEXT no id)No new messages found in channel!";

//             // CREATE A response.errorMessage property to client with the message in printf? TODO: bug.
//             message_t noMsgErrorMessage = createStatusResponseMessage(message, request->clientID);

//             if (&noMsgErrorMessage != NULL)
//             {
//                 response_t response = createServerErrorResponse(&noMsgErrorMessage);
//                 sendResponse(response, new_fd);
//             }
//         }
//         else
//         {
//             // returns an integer that is the total number of next Unread Message in all channels for given client
//             totalUnreadMessageCount = getNextNthMessageCount(request->clientID);
//             printf("\n  1) total unread: %d \n", totalUnreadMessageCount);

//             // Client subscribed to at least one channel at this point.
//             while (count <= MAX_CHANNELS)
//             {
//                 channel = getChannel(count); // find the channel given channel id

//                 if (channel != NULL)
//                 {
//                     client_t *client = findSubscriberInChannel(channel, request->clientID);

//                     if (client != NULL)
//                     {
//                         if (client->unReadMsg >= 1 && totalUnreadMessageCount != 0)
//                         {
//                             // printf("\n ------------> YES client->unReadMsg %d\n", client->unReadMsg);
//                             message_t unreadMessage = readNextMsgFromChannel(channel, client);

//                             if (&unreadMessage != NULL)
//                             { // NOTE: NEXT NULL BECAUSE OUTER IF STATEMENT IS TRUE i.e. client->unReadMsg > 0

//                                 // create server response to client WITH MESSAGE
//                                 response_t response = createServerResponse(&unreadMessage, client, channel->channelID, totalUnreadMessageCount);

//                                 if (response.error == 0)
//                                 {
//                                     printf("\n________________________ \n");
//                                     printf("NEXT %d Message Sending...|\n", request->channelID);
//                                     printf("__________________________ \n");

//                                     printf("\nClientID: %d\n", response.clientID);
//                                     printf("Next message : %s\n", response.message.content);
//                                     printf("UnReadMessageCOUNT : %d\n", response.unReadMessagesCount);
//                                     printf("Error status : %d\n", response.error);
//                                     printf("\n");
//                                 }

//                                 // send response to client
//                                 sendResponse(response, new_fd);
//                                 sleep(1);

//                                 totalUnreadMessageCount--; // decrement the total count of next Unread messages client server needs to wait for
//                                 // printf("\n DECREMENT number UNREAD COUNT %d <-- \n", totalUnreadMessageCount);
//                             }
//                         }
//                     }
//                 }

//                 count++; // increment to next channel id
//             }
//         }

//         return 0;
//     }

//     if (request->commandID == LIVEFEED_ID && request->liveFeed == LIVEFEED_TRUE)
//     { // LIVEFEED with id
//         printf("Server: livefeed got channel id %d", request->channelID);

//         int count = 0;
//         channel_t *channel;

//         int totalUnreadMessageCount = 0;

//         int notSubscribed = isClientSubscribedToAnyChannel(request->clientID); // TODO TMRW

//         if (notSubscribed == 1)
//         { // client not subscribed to any channel

//             char *message = "Not subscribed to any channels.";

//             // CREATE A response.errorMessage property to client with the message in printf? TODO: bug.
//             message_t noMsgErrorMessage = createStatusResponseMessage(message, request->clientID);

//             if (&noMsgErrorMessage != NULL)
//             {
//                 response_t response = createServerErrorResponse(&noMsgErrorMessage);
//                 sendResponse(response, new_fd);
//                 printf("\nerror response send \n");
//             }

//             return 0;
//         }
//         else
//         {
//             // // returns an integer that is the total number of next Unread Message in all channels for given client
//             // totalUnreadMessageCount = getNextNthMessageCount(request->clientID);
//             // printf("\n  1) total unread: %d \n", totalUnreadMessageCount);

//             // while() {

//             // }
//             // Client subscribed to at least one channel at this point.
//             while (count <= MAX_CHANNELS)
//             {
//                 channel = getChannel(count); // find the channel given channel id

//                 if (channel != NULL)
//                 {
//                     client_t *client = findSubscriberInChannel(channel, request->clientID);

//                     if (client != NULL)
//                     {
//                         if (client->unReadMsg >= 1 && totalUnreadMessageCount != 0)
//                         {
//                             // printf("\n ------------> YES client->unReadMsg %d\n", client->unReadMsg);
//                             message_t unreadMessage = readNextMsgFromChannel(channel, client);

//                             if (&unreadMessage != NULL)
//                             { // NOTE: NEXT NULL BECAUSE OUTER IF STATEMENT IS TRUE i.e. client->unReadMsg > 0

//                                 // create server response to client WITH MESSAGE
//                                 response_t response = createServerResponse(&unreadMessage, client, channel->channelID, totalUnreadMessageCount);

//                                 if (response.error == 0)
//                                 {
//                                     printf("\n________________________ \n");
//                                     printf("NEXT %d Message Sending...|\n", request->channelID);
//                                     printf("__________________________ \n");

//                                     printf("\nClientID: %d\n", response.clientID);
//                                     printf("Next message : %s\n", response.message.content);
//                                     printf("UnReadMessageCOUNT : %d\n", response.unReadMessagesCount);
//                                     printf("Error status : %d\n", response.error);
//                                     printf("\n");
//                                 }

//                                 // send response to client
//                                 sendResponse(response, new_fd);
//                                 sleep(1);

//                                 totalUnreadMessageCount--; // decrement the total count of next Unread messages client server needs to wait for
//                                 // printf("\n DECREMENT number UNREAD COUNT %d <-- \n", totalUnreadMessageCount);
//                             }
//                         }
//                     }
//                 }

//                 count++; // increment to next channel id
//             }
//         }

//         return 0;
//     }
// }

// // don't need it?
// int parseRequest(int new_fd, char *clientRequest, char *user_command, char *channel_id)
// {
//     char *string_ptr;
//     char delim[] = " ";

//     string_ptr = strtok(clientRequest, delim);

//     strcpy(user_command, string_ptr);
//     // printf("%s", user_command);
//     string_ptr = strtok(NULL, delim);

//     strcpy(channel_id, string_ptr);
//     // printf("%s", channel_id);
//     string_ptr = strtok(NULL, delim);

//     return 0;
// }

// // Calculate the total number of next unread message client has in all the channels it is subscribed too
// // returns an integer that is the total number of next Unread Message in all channels for given client

// int getNextNthMessageCount(int client_id)
// {
//     int count = 0;
//     int channelCount = 0;

//     client_t *client;
//     channel_t *channel;

//     while (channelCount <= MAX_CHANNELS)
//     {
//         channel = getChannel(channelCount);

//         if (channel != NULL)
//         {
//             client = findSubscriberInChannel(channel, client_id);

//             if (client != NULL)
//             { // client found in one of the channels
//                 if (client->unReadMsg > 0)
//                 {
//                     count++;
//                 }
//             }
//         }

//         channelCount++;
//     }

//     return count;
// }

// int isClientSubscribedToAnyChannel(int client_id)
// {
//     int channelCount = 0;
//     client_t *client;
//     channel_t *channel;

//     while (channelCount <= MAX_CHANNELS)
//     {
//         channel = getChannel(channelCount);

//         if (channel != NULL)
//         {
//             client = findSubscriberInChannel(channel, client_id);

//             if (client != NULL)
//             { // client found in one of the channels
//                 return 0;
//             }
//         }

//         channelCount++;
//     }
//     return 1; // client not sub'd to any channels
// }

// char *parseMessage(char *msg, int channel_id)
// {
//     // convert channel id given from int to string
//     char str_channel_id[MAX_MESSAGE_LENGTH];
//     snprintf(str_channel_id, sizeof(int), "%d", channel_id);
//     // printf("###### %s", str_channel_id);

//     // create error message
//     char *message = (char *)malloc(sizeof(char) * MAX_MESSAGE_LENGTH); // create message var with memory and return pointer
//     strcpy(message, msg);

//     strcat(message, str_channel_id); /* concate id and string message */
//                                      // printf("\n %s\n", message);

//     return message;
// }

// int unSubscribe(int channel_id, int client_id)
// { // Delete clinet from linked list

//     channel_t *channel = getChannel(channel_id);
//     client_t *subHead = channel->subscriberHead, *prevNode;

//     // first node is the client to delete
//     if (subHead != NULL && subHead->clientID == client_id)
//     {                                            // if head node is the client
//         channel->subscriberHead = subHead->next; // subscriber head is linked to the next node
//         free(subHead);

//         return 0;
//     }

//     // or Search for the node to be deleted
//     while (subHead != NULL && subHead->clientID != client_id)
//     { // save the previous node
//         prevNode = subHead;
//         subHead = subHead->next;
//     }

//     // *Unlink the node from the linked list (found)
//     prevNode->next = subHead->next; // and link the node previous to the one being deleted to the next node after one being deleted

//     free(subHead);

//     return 0;
// }

// channel_t *getChannel(int channelID)
// {
//     int i;
//     channel_t *channel = NULL;

//     for (i = 0; i < MAX_CHANNELS; i++)
//     {
//         if (hostedChannels[i].channelID == channelID)
//         {
//             channel = &hostedChannels[i];
//         }
//     }

//     return channel;
// }

// int sendResponse(response_t serverResponse, int sock_id)
// {

//     // send request to server
//     if (send(sock_id, &serverResponse, sizeof(response_t), 0) == -1)
//     {
//         perror("SERVER: response to server failed...\n");
//         return 1;
//     }

//     return 0; // requests successful.
// }

// response_t createServerErrorResponse(message_t *message)
// {
//     response_t *response = malloc(sizeof(response_t));

//     response->message = *message;
//     response->error = 1;

//     return *response;
// }

// // Should i return a pointer response instead since using malloc? ask tutor
// response_t createServerResponse(message_t *message, client_t *client, int channel_id, int unReadMessageCount)
// {
//     response_t *response = malloc(sizeof(response_t));

//     if (unReadMessageCount != 0)
//     {
//         response->unReadMessagesCount = unReadMessageCount;
//     }

//     if (message != NULL)
//     {
//         // printf("\n\n CreateResponseServer: Message given not NULL\n");

//         response->clientID = client->clientID;
//         response->message = *message;
//         response->error = 0;
//         response->channel_id = channel_id;
//     }
//     else
//     {
//         // printf("\n\n CreateResponseServer: Message given NULL\n");
//         response->clientID = client->clientID;
//         response->error = 1;
//     }

//     // printf("\n$MAS: %s\n", response->message.content);

//     return *response;
// }

// int randomClientIdGenerator()
// {
//     int num = rand() % MAX_CLIENTS;

//     return num;
// }

// int create_client(client_t *client)
// {

//     int clientID; /* client id given to each connected client */
//     clientID = randomClientIdGenerator();

//     client->clientID = clientID;
//     client->readMsg = 0;
//     client->unReadMsg = 0;
//     client->totalMessageSent = 0;
//     client->messageQueueIndex = 0;
//     client->entryIndexConstant = 0;
//     client->next = NULL;

//     if (&client->clientID != NULL)
//     {
//         return 0;
//     }
//     else
//     {
//         return 1;
//     }
// }
// void print_subscribers()
// {
//     int i = 0;
//     channel_t *channel;

//     printf("==== Subscribers =====\n");

//     for (i = 0; i < MAX_CHANNELS; i++)
//     {
//         channel_t *channel = &hostedChannels[i];

//         if (channel->subscriberHead != NULL)
//         {
//             client_t *subHead = channel->subscriberHead;
//             printf("\n CHANNEL: %d \n", channel->channelID);
//             while (subHead != NULL)
//             {
//                 printf("\t Subscriber id %d \n", subHead->clientID);
//                 printf("\t Subscriber messageQueueIndex %d \n", subHead->messageQueueIndex);
//                 printf("\t Subscriber entryIndexConstant %d \n", subHead->entryIndexConstant);
//                 printf("\t Subscriber readMsg %d \n", subHead->readMsg);
//                 printf("\t Subscriber unReadMsg %d \n", subHead->unReadMsg);
//                 printf("\t Subscriber totalMessageSent %d \n", subHead->totalMessageSent);
//                 printf("\n -------------------------- \n");

//                 subHead = subHead->next;
//             }
//         }
//     }

//     // printf("TAIL: subscriber id %d\n\n", channel->subscriberTail->clientID);
//     // printf("HEAD: subscriber id %d\n\n", channel->subscriberHead->clientID);
//     printf("====================== \n");
// }

// void updateUnreadMsgCountForSubscribers(channel_t *channel)
// {
//     client_t *currentNode = channel->subscriberHead;
//     if (currentNode == NULL)
//     {
//         printf("\nSERVER:Error, no subscribers found in channel\n");
//     }
//     while (currentNode != NULL)
//     {
//         currentNode->unReadMsg += 1;
//         currentNode = currentNode->next;
//     }
// }

// client_t *findSubscriberInChannel(channel_t *channel, int clientID)
// {
//     // printf("\n\n *** clientid %d, CHANNEL ID %d\n", clientID, channel->channelID);

//     client_t *client = NULL;

//     client_t *subHead = channel->subscriberHead;

//     if (subHead == NULL)
//     {
//         // printf("\n **no subscriber in the list\n");
//         return subHead;
//     }
//     else
//     {
//         while (subHead != NULL)
//         {
//             if (subHead->clientID == clientID)
//             {
//                 client = subHead;
//                 break;
//             }
//             subHead = subHead->next;
//         }
//         return subHead;
//     }
// }

// message_t *findLastMsgInLinkedList(channel_t channel)
// {

//     message_t *lastMsgInList = NULL;
//     message_t *msgHead = channel.messageHead;

//     if (channel.messageHead == NULL)
//     {
//         // no messages in linkedlist
//         lastMsgInList = NULL;
//     }

//     while (msgHead != NULL)
//     {
//         lastMsgInList = msgHead;
//         msgHead = msgHead->next;
//     }

//     return lastMsgInList;
// }

// void subscribe(client_t *client_, int channel_id)
// {
//     client_t *client = malloc(sizeof(client_t));
//     *client = *client_;

//     int i = 0;
//     channel_t *channel;
//     client_t *currentNode;
//     message_t *message;

//     for (i = 0; i < MAX_CHANNELS; i++)
//     {
//         if (hostedChannels[i].channelID == channel_id)
//         {
//             // printf("\n\nChannel selected found: %d\n", hostedChannels[i].channelID);
//             channel = &hostedChannels[i];
//             currentNode = channel->subscriberHead;

//             if (channel->subscriberHead == NULL)
//             {
//                 // printf("Selected channels subscriberHead is NULL\n");

//                 // update the client message queue to current message count in channel
//                 client->entryIndexConstant = channel->totalMsg;
//                 client->messageQueueIndex = channel->totalMsg;

//                 // push to linked list
//                 channel->subscriberHead = client;
//                 channel->subscriberTail = channel->subscriberHead; // track the last node
//                 // printf("1. CHANNEL SUBSCRIBER HEAD clinet id = %d\n", channel->subscriberHead->clientID);
//             }
//             else
//             {
//                 // printf("Selected channels subscriberHead is NOT NULL\n");

//                 // update the client message queue to current message count in channel
//                 client->entryIndexConstant = channel->totalMsg;
//                 client->messageQueueIndex = channel->totalMsg;

//                 while (currentNode->next != NULL)
//                 {
//                     // if the next value null = last node
//                     currentNode = currentNode->next;
//                 }

//                 // push to linked list
//                 currentNode->next = client;                  // on the last node add next client
//                 channel->subscriberTail = currentNode->next; // track the last node
//             }
//         }
//         else
//         {
//             // printf("Channel id not found\n");
//         }
//     }
// }

// void writeToChannelReq(request_t *request, channel_t *channel)
// { // critical section **&^%$%^&&^%

//     client_t *client;

//     message_t *client_message = malloc(sizeof(message_t));
//     *client_message = request->message;

//     client_message->next = NULL;

//     message_t *currentNode;

//     if (channel->messageHead == NULL)
//     {
//         // 1. update total message in channel (before adding message to ensure correct message id given)
//         channel->totalMsg += 1;

//         // 2. Update Message ID: message id is equal to current total message count in channel
//         client_message->messageID = channel->totalMsg;

//         // 3. Add message to channel
//         channel->messageHead = client_message;

//         channel->messageTail = channel->messageHead; /* point the tail to the head */
//     }
//     else
//     {

//         currentNode = channel->messageHead;

//         while (currentNode->next != NULL)
//         {
//             currentNode = currentNode->next;
//         }
//         // 1. create memory for last node.next pointer
//         currentNode->next = malloc(sizeof(message_t));

//         // 2. update total msg count for channel.totalMessages
//         channel->totalMsg += 1;

//         // 3. Update Message ID: message id is equal to current total message count in channel
//         client_message->messageID = channel->totalMsg;

//         // 4. Add message to the end of the messageHead list
//         *currentNode->next = *client_message;

//         channel->messageTail = currentNode->next;
//     }

//     // works: update unreadMsg property for all subcribed clients except client with this clientID
//     updateUnreadMsgCountForSubscribers(channel);

//     // end of critical section ?
// }

// void print_channel_messages(int channel_id)
// {
//     int i = 0;
//     printf("\n==== All Channels Messages ===============================================\n");

//     for (i = 0; i < MAX_CHANNELS; i++)
//     {

//         channel_t *channel = &hostedChannels[i];
//         message_t *msgHead = channel->messageHead;

//         if (msgHead == NULL)
//         {
//             // printf("No Messages FOUND!");
//         }
//         while (msgHead != NULL)
//         {
//             printf("ChannelID: %d, MessageID: %d | ClientID %d, Content: %s \n", channel->channelID, msgHead->messageID, msgHead->ownerID, msgHead->content);
//             msgHead = msgHead->next;
//         }
//     }
// }

// void print_channels()
// {
//     int i = 0;
//     printf("\n-----CHANNELS WITH SUBSCRIBER(s) ---------\n");
//     for (i = 0; i < MAX_CHANNELS; i++)
//     {
//         if (hostedChannels[i].subscriberHead != NULL)
//         {
//             printf("Channel id %d\n", hostedChannels[i].channelID);
//             printf("Channel total msgs: %d\n", hostedChannels[i].totalMsg);
//             printf("\n");
//         }
//     }
// }

// message_t searchNextMsgInList(channel_t *channel, client_t *client)
// {

//     message_t unreadMessage;
//     message_t *currentNode = channel->messageHead;
//     int clientsNextMessageToReadIndex = client->messageQueueIndex + 1;

//     printf("\nSELECTED CHANNEL unReads ### %d\n", client->unReadMsg);

//     if (currentNode == NULL)
//     {
//         // no messages in channel
//         // unreadMessage = NULL;
//         printf("\n-->no messages in channel!\n");
//         return unreadMessage;
//     }
//     else
//     {
//         while (currentNode != NULL)
//         {

//             if (currentNode->messageID == clientsNextMessageToReadIndex)
//             {
//                 // if messageID is the next in messageQueueIndex for client
//                 unreadMessage = *currentNode;
//                 client->messageQueueIndex += 1; // update messageQueueIndex

//                 client->readMsg += 1;   // update read messages count
//                 client->unReadMsg -= 1; // update unread messages count
//             }
//             currentNode = currentNode->next;
//         }

//         return unreadMessage;
//     }
// }

// message_t readNextMsgFromChannel(channel_t *channel, client_t *client)
// { // &&&&&&&&&&&&&&& FIX LIKE BLOODY SEND ONE - REMOVE FOR LOOP WITH FUCNTION

//     printf("SELECTED CHANNEL unReads $$ %d", client->unReadMsg);
//     message_t unreadMessage = searchNextMsgInList(channel, client);

//     // printf("\n 2.Next msg: %s\n", unreadMessage.content);

//     return unreadMessage;
// }

// // NEXT: returns unread message from all channels one by one
// message_t readNextMsgFromAllChannel(int clientID)
// {
//     // int i = 0;
//     // client_t *foundClient;
//     // message_t unreadMessage;
//     // channel_t *channel;

//     // for (i = 0; i < MAX_CHANNELS; i++)
//     // {
//     //     channel = &hostedChannels[1];
//     //     foundClient = findSubscriberInChannel(channel, clientID);

//     //     if (foundClient != NULL && foundClient->clientID == clientID)
//     //     { // Found channel client is subscribed too!
//     //         unreadMessage = readNextMsgFromChannel(channel, foundClient);

//     //         return unreadMessage;
//     //     }
//     // }
// }

// // counts the nextUnread message from all channels, returns the count
// int nextMessageCountInChannels(int clientID)
// {
//     // int i = 0;
//     // int totalUnreadMsg = 0;

//     // client_t *foundClient;
//     // message_t unreadMessage;
//     // channel_t *channel;

//     // for (i = 0; i < MAX_CHANNELS; i++)
//     // {
//     //     channel = &hostedChannels[i];
//     //     if (channel != NULL)
//     //     {
//     //         foundClient = findSubscriberInChannel(channel, clientID);

//     //         // test below
//     //         if (foundClient != NULL)
//     //         {
//     //             printf("\n\nclientid %d, CHANNEL ID %d given. \n", clientID, channel->channelID);
//     //             printf("\n Result:FOUND client ID: %d \n", foundClient->clientID);
//     //         }
//     //         // test above

//     //         if (foundClient != NULL && foundClient->clientID == clientID)
//     //         { // Found channel client is subscribed too!

//     //             printf("\n #Subscriber found in channel \n");

//     //             unreadMessage = readNextMsgFromChannel(channel, foundClient);

//     //             if (&unreadMessage != NULL)
//     //             {
//     //                 totalUnreadMsg += 1;
//     //                 printf("\nmessage FOUND ID %d %s\n", unreadMessage.ownerID, unreadMessage.content);
//     //                 printf("total msg num: %d\n", totalUnreadMsg);
//     //             }
//     //         }
//     //     }
//     //     // printf("for loop i = %d", i);
//     // }

//     // printf("()()() count final: %d\n\n", totalUnreadMsg);

//     // return totalUnreadMsg;
// }

// message_t createStatusResponseMessage(char *message, int clientID)
// {

//     message_t statusMessage;

//     statusMessage.messageID = 1;
//     statusMessage.ownerID = clientID;

//     strcpy(statusMessage.content, message);

//     return statusMessage;
// }



/************************************************************************* second one/
 * 
 * /*
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

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "server_functions.h"

#define SHMSZ 27

#define ARRAY_SIZE 30 /* Size of array to receive */

#define BACKLOG 10 /* how many pending connections queue will hold */

#define RETURNED_ERROR -1

#define MAX_CLIENTS 100

#define MAX_MESSAGE_COUNT 1000

#define MAX_CHANNELS 256

// #define MAX_MESSAGE_LENGTH 1024

#define NO_CHANNEL -1

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

#define CLIENT_ACTIVE -1
#define CLIENT_INACTIVE -2

#define SIZEOFMEMORY 27

int sockfd;
int new_fd; /* listen on sock_fd, new connection on new_fd */

// typedef struct message message_t;

// struct message
// {
//     int messageID;
//     int ownerID;
//     char content[MAX_MESSAGE_LENGTH];
//     // message_t *next;
// };

// message_t client_message;

// typedef struct request request_t;

// struct request
// {
//     int commandID; /* integer enum representing command i.e. SUB, NEXT*/
//     int channelID;
//     int clientID;
//     message_t message;
//     int liveFeed;
// };

// request_t clientRequest; /* move into main ??*/

// typedef struct client client_t;

// struct client
// {
//     int clientID;
//     int readMsg;
//     int unReadMsg;
//     int totalMessageSent;
//     int messageQueueIndex;  /* index = message that it read currently, NOT NEXT ONE */
//     int entryIndexConstant; /* index of msg queue at which the client subed */
//     int status;
//     // client_t *next;
// };

// typedef struct channel channel_t;

// struct channel
// {
//     int channelID;
//     int totalMsg;
//     message_t *messages;
//     client_t *subscribers;
//     int subscriberCount;
//     int messagesCount;
// };

// typedef struct response response_t;

// struct response
// {
//     int clientID;
//     message_t message;
//     int error;
//     int channel_id;
//     int unReadMessagesCount;
// };

/*
*   SHARED MEMORY 
*/

// channel_t hostedChannels[MAX_CHANNELS];

struct shared
{
    channel_t hostedChannels[MAX_CHANNELS];
} typedef sharedMemory_t;

sharedMemory_t *sharedChannels;

// channel_t *hostedChannels[MAX_CHANNELS];

int shmid;
key_t key;

/*
    CHANNEL FUNCTIONS (INCLUDE IN SEPERATE FILE LATER)
*/

// // Creates 256 channels for global hostedChannels array
// channel_t *generateHostChannels();

// client_t generate_client(int child_id); // cretes inactive client objects to the channels
// int randomClientIdGenerator();

// void print_channels_with_subscribers();

// // when subscribing to channel, finds out the last message in the channel, and will be used to add the message id to the subscribing client
// message_t createStatusResponseMessage(char *message, int clientID);
// response_t createServerErrorResponse(message_t *message);

// // // SUBSCRIBE AND UNSUBSCRIBE FUNCS
// // message_t *findLastMsgInLinkedList(channel_t channel);

// void subscribe(client_t *client_, int channel_id);

// int unSubscribe(int channel_id, int client_id);

// char *parseMessage(char *msg, int channel_id);

// client_t *findSubscriberInChannel(channel_t *channel, int clientID);

// int subscriberCountInChannel(channel_t *channel);

// void print_subscribers_all();

// void updateUnreadMsgCountForSubscribers(channel_t *channel);

// void writeToChannelReq(request_t *request, channel_t *channel);

// void print_channel_messages();

// message_t searchNextMsgInList(channel_t *channel, client_t *client);

// message_t readNextMsgFromChannel(channel_t *channel, client_t *client);

// int isClientSubscribedToAnyChannel(int client_id);

// int getNextNthMessageCount(int client_id);

// response_t createServerResponse(message_t *message, client_t *client, int channel_id, int unReadMessageCount);
// int sendResponse(response_t serverResponse, int sock_id);

// // message_t readNextMsgFromAllChannel(int clientID); // next no id

// channel_t *getChannel(int channelID);

// // int nextMessageCountInChannels(int clientID);

/*
    END OF
*/

void shut_down_handler()
{
    // close threads
    // close processes
    // close sockets
    // shared memory regions
    // close(new_fd);
    close(sockfd);
    close(new_fd);

    // free allocated memory and open files
    // sleep(1);
    write(1, "\n\nShutting down gracefully, BYE!\n\n", 35);
    exit(1);
}

// int handleClientRequests(request_t *request, client_t *client);

// // int parseRequest(int new_fd, char *clientRequest, char *user_command, char *channel_id);

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

    /* Get port number for server to listen on */
    if (argc == 2)
    {
        fprintf(stderr, "usage: client port_number\n");
        // exit(1);
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
            // continue;
        }

        printf("server: got connection from %s\n",
               inet_ntoa(their_addr.sin_addr));

        for (i = 0; i < MAX_CHANNELS; i++)
        {

            sharedChannels->hostedChannels[i].channelID = i;
            sharedChannels->hostedChannels[i].totalMsg = 0;
            sharedChannels->hostedChannels[i].messagesCount = 0;
            sharedChannels->hostedChannels[i].subscriberCount = 0;

            sharedChannels->hostedChannels[i].subscribers = (client_t *)malloc(sizeof(client_t) * MAX_CLIENTS);
            sharedChannels->hostedChannels[i].messages = (message_t *)malloc(sizeof(message_t) * MAX_MESSAGE_COUNT);
        }

        for (i = 0; i < 5; i++)
        {
            printf("\nParent process: channels id %d\n", sharedChannels->hostedChannels[i].channelID);
        }

        if (!fork())
        { /* this is the child process */
            printf("\n-----------------------CHILD START ----------\n");
            printf("\n child process id is %d. parent id is: %d\n", getpid(), getppid());

            /*
            * Locate the segment.
            */
            key_t key = 1234;

            if ((shmid = shmget(key, SHMSZ, 0666)) < 0)
            {
                perror("shmget");
                exit(1);
            }

            /*
            * Now we attach the segment to our data space.
            */
            if ((sharedChannels = shmat(shmid, NULL, 0)) == (sharedMemory_t *)-1)
            {
                perror("shmat");
                exit(1);
            }

            for (i = 0; i < 5; i++)
            {
                printf("\nchild process: channels id %d\n", sharedChannels->hostedChannels[i].channelID);
            }

            /* ***Server-Client Connected*** */
            int child_id = getpid();
            client_t client = generate_client(child_id);

            printf("\n =>client id %d STATUS: %d\n", client.clientID, client.status);

            // ADD a new client objec
            if (client.clientID < -1)
            {
                perror("SERVER: failed to create client object (Max. 100 clients allowed)");
                printf("SERVER: failed to create client object (Max. 100 clients allowed) \n");
                exit(1);
                // send response to client Cant accept connection
            }

            printf("\n-------------------------------------------\n");

            printf("\n >> new_fd %d\n", new_fd);

            // Send: Welcome Message.
            if (send(new_fd, &client.clientID, sizeof(int), 0) == -1)
            {
                perror("send");
                printf("Error: Welcome message not sent to client \n");
            }

            while (recv(new_fd, &clientRequest, sizeof(request_t), 0))
            {

                printf("\n HANDLE request\n");

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

            printf("\n-----------------------CHILD END ----------\n");
            close(new_fd);
            exit(1);
        }
    }
}

// int handleClientRequests(request_t *request, client_t *client)
// {
//     printf("\n --> command: %d channel id %d \n", request->commandID, request->channelID);

//     if (request->commandID == SUB)
//     { // Subscribe to Channel request

//         channel_t *channel;
//         client_t *isClient;
//         // client_t client;

//         if (request->channelID == NO_CHANNEL)
//         { // Client sent SUB with no id

//             // create error message
//             char *message = "No channel id given";

//             message_t errorMessage = createStatusResponseMessage(message, request->clientID);

//             if (&errorMessage != NULL)
//             {
//                 response_t response = createServerErrorResponse(&errorMessage);
//                 sendResponse(response, new_fd);
//             }
//             return 0;
//         }
//         else
//         {
//             channel = getChannel(request->channelID);

//             if (channel == NULL || channel->channelID != request->channelID)
//             { // Invalid channel id

//                 char *msg = "Invalid channel: ";
//                 char *message_ptr = parseMessage(msg, request->channelID);
//                 // printf("\n %s\n", message_ptr);

//                 message_t errorMessage = createStatusResponseMessage(message_ptr, request->clientID);

//                 if (&errorMessage != NULL)
//                 {
//                     response_t response = createServerErrorResponse(&errorMessage);
//                     sendResponse(response, new_fd);
//                     printf("--->-->message %s\n", errorMessage.content);
//                 }
//             }
//             else
//             { // Subscribe to channel

//                 isClient = findSubscriberInChannel(channel, request->clientID);

//                 if (isClient == NULL)
//                 {
//                     printf("is Client = NULL\n");
//                     printf("\nSubscribe now...\n");

//                     // DISABLE SUB IF MORE THAN 100 IN BUFFER? TODO LATER. IF/ELSE
//                     // int currentSubscriberCount = subscriberCountInChannel(channel);

//                     subscribe(client, request->channelID);

//                     print_channels_with_subscribers();

//                     print_subscribers_all();

//                     char *msg = "Subscribed to channel: ";
//                     char *message_ptr = parseMessage(msg, request->channelID);

//                     message_t successMessage = createStatusResponseMessage(message_ptr, request->clientID);

//                     if (&successMessage != NULL)
//                     {
//                         response_t response = createServerResponse(&successMessage, client, request->channelID, -1);
//                         sendResponse(response, new_fd);
//                     }
//                 }
//                 else
//                 { // Client already subscribed
//                     printf("is Client = NOT NULL (already subscribed)\n");

//                     char *msg = "Already subscribed to channel: ";
//                     char *message_ptr = parseMessage(msg, request->channelID);

//                     message_t errorMessage = createStatusResponseMessage(message_ptr, request->clientID);

//                     if (&errorMessage != NULL)
//                     {
//                         response_t response = createServerErrorResponse(&errorMessage);
//                         sendResponse(response, new_fd);
//                     }
//                 }

//                 return 0;
//             }
//         }
//     }

//     else if (request->commandID == UNSUB)
//     { // UNSUB to Channel request

//         channel_t *channel;
//         client_t *isClient;

//         if (request->channelID == NO_CHANNEL)
//         { // Client sent SUB with no id

//             // create error message
//             char *message = "No channel id given";

//             message_t errorMessage = createStatusResponseMessage(message, request->clientID);

//             if (&errorMessage != NULL)
//             {
//                 response_t response = createServerErrorResponse(&errorMessage);
//                 sendResponse(response, new_fd);
//             }
//             return 0;
//         }
//         else
//         {
//             channel = getChannel(request->channelID);

//             if (channel == NULL || channel->channelID != request->channelID)
//             { // Invalid channel id

//                 char *msg = "Invalid channel: ";
//                 char *message_ptr = parseMessage(msg, request->channelID);

//                 message_t errorMessage = createStatusResponseMessage(message_ptr, request->clientID);

//                 if (&errorMessage != NULL)
//                 {
//                     response_t response = createServerErrorResponse(&errorMessage);
//                     sendResponse(response, new_fd);
//                 }
//             }
//             else
//             {
//                 isClient = findSubscriberInChannel(channel, request->clientID);

//                 if (isClient != NULL)
//                 { // unsubscribe client
//                     // printf("\n isClient != NULL **\n");
//                     unSubscribe(request->channelID, isClient->clientID);

//                     print_channels_with_subscribers();
//                     // print_subscribers_all();

//                     char *msg = "Unsubscribed from channel: ";
//                     char *message_ptr = parseMessage(msg, request->channelID);

//                     message_t successMessage = createStatusResponseMessage(message_ptr, request->clientID);

//                     if (&successMessage != NULL)
//                     {
//                         response_t response = createServerResponse(&successMessage, client, request->channelID, -1);
//                         sendResponse(response, new_fd);
//                     }
//                 }
//                 else
//                 { // Client already subscribed

//                     char *msg = "Not subscribed to channel: ";
//                     char *message_ptr = parseMessage(msg, request->channelID);

//                     message_t errorMessage = createStatusResponseMessage(message_ptr, request->clientID);

//                     if (&errorMessage != NULL)
//                     {
//                         response_t response = createServerErrorResponse(&errorMessage);
//                         sendResponse(response, new_fd);
//                     }
//                 }
//             }
//         }

//         return 0;
//     }

//     if (request->commandID == SEND)
//     { // SEND message to channel
//         printf("$Command id %d and channel id %d\n", request->commandID, request->channelID);
//         int i;
//         channel_t *channel;

//         channel = getChannel(request->channelID);

//         if (channel != NULL)
//         { // NOTE: Client who has NOT* subscribe CAN send a message to a channel*

//             printf("\n channel is not null!!\n");

//             writeToChannelReq(request, channel);

//             // // send client response
//             char *message = "Message successfully sent";
//             message_t successMessage = createStatusResponseMessage(message, request->clientID);

//             if (&successMessage != NULL)
//             {
//                 response_t response = createServerErrorResponse(&successMessage);
//                 sendResponse(response, new_fd);
//             }
//         }
//         else
//         {
//             printf("SEND: channel id null!! %d\n", request->channelID);

//             char *msg = "Invalid channel: ";
//             char *message_ptr = parseMessage(msg, request->channelID);

//             message_t errorMessage = createStatusResponseMessage(message_ptr, request->clientID);

//             if (&errorMessage != NULL)
//             {
//                 response_t response = createServerErrorResponse(&errorMessage);
//                 sendResponse(response, new_fd);
//             }
//         }

//         print_channels_with_subscribers();
//         print_channel_messages();
//         // print_subscribers_all();

//         return 0;
//     }

//     if (request->commandID == NEXT_ID && request->channelID != NO_CHANNEL)
//     { // READ message to channel
//         printf("\nServer: NEXT WITH id %d request recevied.\n", request->channelID);

//         channel_t *channel;

//         channel = getChannel(request->channelID); // find the channel given channel id

//         if (channel != NULL)
//         {
//             client_t *client = findSubscriberInChannel(channel, request->clientID);

//             if (client != NULL)
//             {
//                 if (client->unReadMsg > 0)
//                 {
//                     message_t unreadMessage = readNextMsgFromChannel(channel, client);

//                     if (&unreadMessage != NULL)
//                     { // MOTE: NEXT NULL BECAUSE OUTER IF STATEMENT IS TRUE i.e. client->unReadMsg > 0

//                         // create server response to client WITH MESSAGE
//                         response_t response = createServerResponse(&unreadMessage, client, request->channelID, -1);

//                         if (response.error == 0)
//                         {
//                             printf("\n________________________ \n");
//                             printf("NEXT %d Message Sending...|\n", request->channelID);
//                             printf("__________________________ \n");

//                             printf("\nClientID: %d\n", response.clientID);
//                             printf("Next message : %s\n", response.message.content);
//                             printf("Error status : %d\n", response.error);
//                             printf("\n");
//                         }

//                         // send response to client
//                         sendResponse(response, new_fd);
//                     }
//                 }
//                 else
//                 {
//                     // note: ERROR MESSAGE LIKE THIS COMING FROM SOMEHWERE WITH NEXT command!!!

//                     char *message = "(NEXT with ID) No new messages found in channel!";
//                     // printf("\n %s\n", message);

//                     // // CREATE A response.errorMessage property to client with the message in printf? TODO: bug.
//                     message_t noMsgErrorMessage = createStatusResponseMessage(message, request->clientID);

//                     if (&noMsgErrorMessage != NULL)
//                     {
//                         response_t response = createServerErrorResponse(&noMsgErrorMessage);
//                         sendResponse(response, new_fd);
//                     }
//                 }
//             }
//             else
//             {

//                 char *msg = "Not subscribed to channel: ";
//                 char *message_ptr = parseMessage(msg, request->channelID);

//                 message_t errorMessage = createStatusResponseMessage(message_ptr, request->clientID);

//                 if (&errorMessage != NULL)
//                 {
//                     response_t response = createServerErrorResponse(&errorMessage);
//                     sendResponse(response, new_fd);
//                 }
//             }
//         }
//         else
//         {

//             char *msg = "Invalid channel: ";
//             char *message_ptr = parseMessage(msg, request->channelID);

//             message_t errorMessage = createStatusResponseMessage(message_ptr, request->clientID);

//             if (&errorMessage != NULL)
//             {
//                 response_t response = createServerErrorResponse(&errorMessage);
//                 sendResponse(response, new_fd);
//             }
//         }

//         print_channels_with_subscribers();

//         return 0;
//     }

//     if (request->commandID == NEXT && request->channelID == NO_CHANNEL)
//     { // NEXT no id
//         printf("\nSERVER: Next WITHOUT <%d> request received.\n", request->channelID);

//         int count = 0;
//         channel_t *channel;

//         int totalUnreadMessageCount = 0;

//         int notSubscribed = isClientSubscribedToAnyChannel(request->clientID);

//         if (notSubscribed == 1)
//         { // client not subscribed to any channel

//             char *message = "Not subscribed to any channels.";

//             // CREATE A response.errorMessage property to client with the message in printf? TODO: bug.
//             message_t noMsgErrorMessage = createStatusResponseMessage(message, request->clientID);

//             if (&noMsgErrorMessage != NULL)
//             {
//                 response_t response = createServerErrorResponse(&noMsgErrorMessage);
//                 sendResponse(response, new_fd);
//                 printf("\nerror response send \n");
//             }

//             return 0;
//         }
//         else if ((totalUnreadMessageCount = getNextNthMessageCount(request->clientID) == 0))
//         {
//             // If not messages in any channel send "no message" to client
//             // returns an integer that is the total number of next Unread Message in all channels for given client

//             char *message = "No new messages found in channel!";

//             // CREATE A response.errorMessage property to client with the message in printf? TODO: bug.
//             message_t noMsgErrorMessage = createStatusResponseMessage(message, request->clientID);

//             if (&noMsgErrorMessage != NULL)
//             {
//                 response_t response = createServerErrorResponse(&noMsgErrorMessage);
//                 sendResponse(response, new_fd);
//             }
//         }
//         else
//         {
//             // returns an integer that is the total number of next Unread Message in all channels for given client
//             totalUnreadMessageCount = getNextNthMessageCount(request->clientID);

//             // Client subscribed to at least one channel at this point.
//             while (count <= MAX_CHANNELS)
//             {
//                 channel = getChannel(count); // find the channel given channel id

//                 if (channel != NULL)
//                 {
//                     client_t *client = findSubscriberInChannel(channel, request->clientID);

//                     if (client != NULL)
//                     {
//                         if (client->unReadMsg >= 1 && totalUnreadMessageCount != 0)
//                         {
//                             // printf("\n ------------> YES client->unReadMsg %d\n", client->unReadMsg);
//                             message_t unreadMessage = readNextMsgFromChannel(channel, client);

//                             if (&unreadMessage != NULL)
//                             { // NOTE: NEXT NULL BECAUSE OUTER IF STATEMENT IS TRUE i.e. client->unReadMsg > 0

//                                 // create server response to client WITH MESSAGE
//                                 response_t response = createServerResponse(&unreadMessage, client, channel->channelID, totalUnreadMessageCount);

//                                 if (response.error == 0)
//                                 {
//                                     printf("\n________________________ \n");
//                                     printf("NEXT %d Message Sending...|\n", request->channelID);
//                                     printf("__________________________ \n");

//                                     printf("\nClientID: %d\n", response.clientID);
//                                     printf("Next message : %s\n", response.message.content);
//                                     printf("UnReadMessageCOUNT : %d\n", response.unReadMessagesCount);
//                                     printf("Error status : %d\n", response.error);
//                                     printf("\n");
//                                 }

//                                 // send response to client
//                                 sendResponse(response, new_fd);
//                                 sleep(1);

//                                 totalUnreadMessageCount--; // decrement the total count of next Unread messages client server needs to wait for
//                                 // printf("\n DECREMENT number UNREAD COUNT %d <-- \n", totalUnreadMessageCount);
//                             }
//                         }
//                     }
//                 }

//                 count++; // increment to next channel id
//             }
//         }

//         return 0;
//     }

//     if (request->commandID == LIVEFEED_ID && request->liveFeed == LIVEFEED_TRUE)
//     { // LIVEFEED with id
//         printf("Server: livefeed got channel id %d", request->channelID);

//         // int count = 0;
//         // channel_t *channel;

//         // int totalUnreadMessageCount = 0;

//         // int notSubscribed = isClientSubscribedToAnyChannel(request->clientID); // TODO TMRW

//         // if (notSubscribed == 1)
//         // { // client not subscribed to any channel

//         //     char *message = "Not subscribed to any channels.";

//         //     // CREATE A response.errorMessage property to client with the message in printf? TODO: bug.
//         //     message_t noMsgErrorMessage = createStatusResponseMessage(message, request->clientID);

//         //     if (&noMsgErrorMessage != NULL)
//         //     {
//         //         response_t response = createServerErrorResponse(&noMsgErrorMessage);
//         //         sendResponse(response, new_fd);
//         //         printf("\nerror response send \n");
//         //     }

//         //     return 0;
//         // }
//         // else
//         // {
//         //     // // returns an integer that is the total number of next Unread Message in all channels for given client
//         //     // totalUnreadMessageCount = getNextNthMessageCount(request->clientID);
//         //     // printf("\n  1) total unread: %d \n", totalUnreadMessageCount);

//         //     // while() {

//         //     // }
//         //     // Client subscribed to at least one channel at this point.
//         //     while (count <= MAX_CHANNELS)
//         //     {
//         //         channel = getChannel(count); // find the channel given channel id

//         //         if (channel != NULL)
//         //         {
//         //             client_t *client = findSubscriberInChannel(channel, request->clientID);

//         //             if (client != NULL)
//         //             {
//         //                 if (client->unReadMsg >= 1 && totalUnreadMessageCount != 0)
//         //                 {
//         //                     // printf("\n ------------> YES client->unReadMsg %d\n", client->unReadMsg);
//         //                     message_t unreadMessage = readNextMsgFromChannel(channel, client);

//         //                     if (&unreadMessage != NULL)
//         //                     { // NOTE: NEXT NULL BECAUSE OUTER IF STATEMENT IS TRUE i.e. client->unReadMsg > 0

//         //                         // create server response to client WITH MESSAGE
//         //                         response_t response = createServerResponse(&unreadMessage, client, channel->channelID, totalUnreadMessageCount);

//         //                         if (response.error == 0)
//         //                         {
//         //                             printf("\n________________________ \n");
//         //                             printf("NEXT %d Message Sending...|\n", request->channelID);
//         //                             printf("__________________________ \n");

//         //                             printf("\nClientID: %d\n", response.clientID);
//         //                             printf("Next message : %s\n", response.message.content);
//         //                             printf("UnReadMessageCOUNT : %d\n", response.unReadMessagesCount);
//         //                             printf("Error status : %d\n", response.error);
//         //                             printf("\n");
//         //                         }

//         //                         // send response to client
//         //                         sendResponse(response, new_fd);
//         //                         sleep(1);

//         //                         totalUnreadMessageCount--; // decrement the total count of next Unread messages client server needs to wait for
//         //                         // printf("\n DECREMENT number UNREAD COUNT %d <-- \n", totalUnreadMessageCount);
//         //                     }
//         //                 }
//         //             }
//         //         }

//         //         count++; // increment to next channel id
//         //     }
//         // }
//         return 0;
//     }

//     return 0;
// }

// // don't need it?
// int parseRequest(int new_fd, char *clientRequest, char *user_command, char *channel_id)
// {
//     char *string_ptr;
//     char delim[] = " ";

//     string_ptr = strtok(clientRequest, delim);

//     strcpy(user_command, string_ptr);
//     // printf("%s", user_command);
//     string_ptr = strtok(NULL, delim);

//     strcpy(channel_id, string_ptr);
//     // printf("%s", channel_id);
//     string_ptr = strtok(NULL, delim);

//     return 0;
// }

// // channel_t *generateHostChannels()
// // {
// //     int i, j = 0;

// //     channel_t hostedChannels[MAX_CHANNELS];

// //     for (i = 0; i < MAX_CHANNELS; i++)
// //     {

// //         hostedChannels[i].channelID = i;
// //         hostedChannels[i].totalMsg = 0;
// //         hostedChannels[i].messagesCount = 0;
// //         hostedChannels[i].subscriberCount = 0;

// //         hostedChannels[i].subscribers = (client_t *)malloc(sizeof(client_t) * MAX_CLIENTS);
// //         hostedChannels[i].messages = (message_t *)malloc(sizeof(message_t) * MAX_MESSAGE_COUNT);
// //     }

// //     return hostedChannels;
// // }
// client_t generate_client(int child_id)
// {
//     client_t *client = malloc(sizeof(client_t));

//     // client->clientID = randomClientIdGenerator();
//     client->clientID = child_id;
//     client->entryIndexConstant = 0;
//     client->messageQueueIndex = 0;
//     client->readMsg = 0;
//     client->totalMessageSent = 0;
//     client->unReadMsg = 0;
//     client->status = CLIENT_INACTIVE;

//     return *client;
// }

// void print_channels_with_subscribers()
// {

//     int i = 0;
//     int j = 0;
//     printf("\n-----CHANNELS WITH SUBSCRIBER(s) ---------\n");
//     for (i = 0; i < MAX_CHANNELS; i++)
//     {
//         for (j = 0; j < MAX_CLIENTS; j++)
//         {
//             if (hostedChannels[i].subscribers[j].status == CLIENT_ACTIVE)
//             {
//                 printf("\n\nChannel id %d\n", hostedChannels[i].channelID);
//                 printf("Client id: %d\n", hostedChannels[i].subscribers[j].clientID);
//                 printf("subscriber status: %d\n", hostedChannels[i].subscribers[j].status);
//                 printf("subscriber count: %d\n", hostedChannels[i].subscriberCount);

//                 printf("\n \t\t =>  entryIndexConstant %d\n", hostedChannels[i].subscribers[j].entryIndexConstant);
//                 printf("\n \t\t =>  messageQueueIndex %d\n", hostedChannels[i].subscribers[j].messageQueueIndex);
//                 printf("\n \t\t =>  readMsg %d\n", hostedChannels[i].subscribers[j].readMsg);
//                 printf("\n \t\t =>  unReadMsg %d\n", hostedChannels[i].subscribers[j].unReadMsg);
//                 printf("\n \t\t =>  totalMessageSent %d", hostedChannels[i].subscribers[j].totalMessageSent);
//             }
//         }
//     }
// }

// int randomClientIdGenerator()
// {
//     int num = rand() % MAX_CLIENTS;

//     return num;
// }

// int subscriberCountInChannel(channel_t *channel)
// {
//     int subscriberCount = 0;
//     int i, j = 0;

//     subscriberCount = sizeof(channel->subscribers) / sizeof(client_t);

//     return subscriberCount;
// }

// // Calculate the total number of next unread message client has in all the channels it is subscribed too
// // returns an integer that is the total number of next Unread Message in all channels for given client

// int getNextNthMessageCount(int client_id)
// {
//     int count = 0;
//     int channelCount = 0;

//     client_t *client;
//     channel_t *channel;

//     while (channelCount < MAX_CHANNELS)
//     {
//         channel = getChannel(channelCount);

//         if (channel != NULL)
//         {
//             client = findSubscriberInChannel(channel, client_id);

//             if (client != NULL)
//             { // client found in one of the channels
//                 if (client->unReadMsg > 0)
//                 {
//                     count += 1;
//                 }
//             }
//         }

//         channelCount++;
//     }
//     printf("\n \t ||||=>> COUNT  %d\n", count);

//     return count;
// }

// int isClientSubscribedToAnyChannel(int client_id)
// {
//     int channelCount = 0;
//     client_t *client;
//     channel_t *channel;

//     while (channelCount <= MAX_CHANNELS)
//     {
//         channel = getChannel(channelCount);

//         if (channel != NULL)
//         {
//             client = findSubscriberInChannel(channel, client_id);

//             if (client != NULL)
//             { // client found in one of the channels
//                 return 0;
//             }
//         }

//         channelCount++;
//     }
//     return 1; // client not sub'd to any channels
// }

// char *parseMessage(char *msg, int channel_id)
// {
//     // convert channel id given from int to string
//     char str_channel_id[MAX_MESSAGE_LENGTH];
//     snprintf(str_channel_id, sizeof(int), "%d", channel_id);
//     // printf("###### %s", str_channel_id);

//     // create error message
//     char *message = (char *)malloc(sizeof(char) * MAX_MESSAGE_LENGTH); // create message var with memory and return pointer
//     strcpy(message, msg);

//     strcat(message, str_channel_id); /* concate id and string message */
//                                      // printf("\n %s\n", message);

//     return message;
// }

// int unSubscribe(int channel_id, int client_id)
// { // Delete clinet from linked list

//     channel_t *channel = getChannel(channel_id);

//     int i, j = 0;

//     for (i = 0; i < MAX_CHANNELS; i++)
//     {
//         for (j = 0; j < MAX_CLIENTS; j++)
//         {
//             if (channel->subscribers[i].clientID == client_id)
//             {
//                 // printf("\nunsubinggggg  client id %d\n\n", channel->subscribers[i].clientID);
//                 int pos = i; // pos = index to delete

//                 for (i = pos - 1; i < channel->subscriberCount; i++)
//                 {                                                          // i = previous element to the one being deleted
//                     channel->subscribers[i] = channel->subscribers[i + 1]; // moved to the index of the element to be deleted.
//                 }

//                 channel->subscriberCount--; // reduce the length to show updated array

//                 return 0;
//             }
//         }
//     }

//     return 0;
// }

// channel_t *getChannel(int channelID)
// {
//     int i;
//     channel_t *channel = NULL;

//     for (i = 0; i < MAX_CHANNELS; i++)
//     {
//         if (hostedChannels[i].channelID == channelID)
//         {
//             channel = &hostedChannels[i];
//         }
//     }

//     return channel;
// }

// int sendResponse(response_t serverResponse, int sock_id)
// {

//     // send request to server
//     if (send(sock_id, &serverResponse, sizeof(response_t), 0) == -1)
//     {
//         perror("SERVER: response to server failed...\n");
//         return 1;
//     }

//     return 0; // requests successful.
// }

// response_t createServerErrorResponse(message_t *message)
// {
//     response_t *response = malloc(sizeof(response_t));

//     response->message = *message;
//     response->error = 1;

//     return *response;
// }
// // client_t client;

// // // Should i return a pointer response instead since using malloc? ask tutor
// response_t createServerResponse(message_t *message, client_t *client, int channel_id, int unReadMessageCount)
// {
//     response_t *response = malloc(sizeof(response_t));

//     if (unReadMessageCount != 0)
//     {
//         response->unReadMessagesCount = unReadMessageCount;
//     }

//     if (message != NULL)
//     {
//         // printf("\n\n CreateResponseServer: Message given not NULL\n");

//         response->clientID = client->clientID;
//         response->message = *message;
//         response->error = 0;
//         response->channel_id = channel_id;
//     }
//     else
//     {
//         // printf("\n\n CreateResponseServer: Message given NULL\n");
//         response->clientID = client->clientID;
//         response->error = 1;
//     }

//     // printf("\n$MAS: %s\n", response->message.content);

//     return *response;
// }

// void print_subscribers_all()
// {
//     int i, j = 0;
//     channel_t *channel;

//     printf("==== All Subscribers =====\n");

//     for (i = 0; i < MAX_CHANNELS; i++)
//     {
//         channel_t *channel = &hostedChannels[i];

//         for (j = 0; j < MAX_CLIENTS; j++)
//         {
//             if (channel->subscribers[j].status == CLIENT_ACTIVE)
//             {
//                 printf("\n\t Subscriber id %d \n", channel->subscribers[j].clientID);
//                 // printf("\t Subscriber messageQueueIndex %d \n", channel->subscribers[j].messageQueueIndex);
//                 // printf("\t Subscriber entryIndexConstant %d \n", channel->subscribers[j].entryIndexConstant);
//                 // printf("\t Subscriber readMsg %d \n", channel->subscribers[j].readMsg);
//                 // printf("\t Subscriber unReadMsg %d \n", channel->subscribers[j].unReadMsg);
//                 // printf("\t Subscriber totalMessageSent %d \n", channel->subscribers[j].totalMessageSent);
//                 printf("\n -------------------------- \n");
//             }
//         }
//     }

//     printf("====================== \n");
// }

// void updateUnreadMsgCountForSubscribers(channel_t *channel)
// {
//     int i, j = 0;

//     for (i = 0; i < MAX_CHANNELS; i++)
//     {
//         if (hostedChannels[i].channelID == channel->channelID)
//         { // update unread count for all subscribers of the given channel
//             for (j = 0; j < MAX_CLIENTS; j++)
//             {
//                 hostedChannels[i].subscribers[j].unReadMsg += 1;
//             }
//         }
//     }
// }

// client_t *findSubscriberInChannel(channel_t *channel, int clientID)
// {
//     // printf("\n\n *** clientid %d, CHANNEL ID %d\n", clientID, channel->channelID);

//     int i = 0;
//     client_t *client = NULL;

//     for (i = 0; i < MAX_CLIENTS; i++)
//     {
//         if (clientID == channel->subscribers[i].clientID)
//         {
//             client = &channel->subscribers[i];
//             return client;
//         }
//     }

//     return client;

//     // client_t *client = NULL;

//     // client_t *subHead = channel->subscriberHead;

//     // if (subHead == NULL)
//     // {
//     //     // printf("\n **no subscriber in the list\n");
//     //     return subHead;
//     // }
//     // else
//     // {
//     //     while (subHead != NULL)
//     //     {
//     //         if (subHead->clientID == clientID)
//     //         {
//     //             client = subHead;
//     //             break;
//     //         }
//     //         subHead = subHead->next;
//     //     }
//     //     return subHead;
//     // }
// }

// // message_t *findLastMsgInLinkedList(channel_t channel)
// // {

// //     message_t *lastMsgInList = NULL;
// //     message_t *msgHead = channel.messageHead;

// //     if (channel.messageHead == NULL)
// //     {
// //         // no messages in linkedlist
// //         lastMsgInList = NULL;
// //     }

// //     while (msgHead != NULL)
// //     {
// //         lastMsgInList = msgHead;
// //         msgHead = msgHead->next;
// //     }

// //     return lastMsgInList;
// // }

// void subscribe(client_t *client_, int channel_id)
// {
//     int i = 0;

//     //CRITICAL SECTION - writing to shared memory hostedChannels subscriber array ?

//     client_t *client = malloc(sizeof(client_t));
//     *client = *client_;

//     for (i = 0; i < MAX_CHANNELS; i++)
//     {
//         if (hostedChannels[i].channelID == channel_id)
//         {

//             printf("\n\nChannel selected found: %d\n", hostedChannels[i].channelID);
//             // channel = &hostedChannels[i];

//             // current subscriber count in the channel
//             int subscriberCount = hostedChannels[i].subscriberCount;

//             // update the client message queue to current message count in channel
//             client->entryIndexConstant = hostedChannels[i].totalMsg;
//             client->messageQueueIndex = hostedChannels[i].totalMsg;

//             // update client status to active
//             client->status = CLIENT_ACTIVE;

//             // add client to the next free index
//             hostedChannels[i].subscribers[subscriberCount] = *client;

//             // update channels subscriber count
//             hostedChannels[i].subscriberCount += 1;
//         }
//     }
// }

// void writeToChannelReq(request_t *request, channel_t *channel)
// { // critical section **&^%$%^&&^%

//     int i, j = 0;
//     int count = 0;

//     message_t *client_message = malloc(sizeof(message_t));
//     *client_message = request->message;

//     for (i = 0; i < MAX_CHANNELS; i++)
//     {
//         for (j = 0; j < MAX_CLIENTS; j++)
//         {
//             if (hostedChannels[i].channelID == request->channelID && hostedChannels[i].messagesCount <= MAX_MESSAGE_COUNT)
//             {

//                 // 1. Update Message ID: message id is equal to current total message count in channel (starting from 1)
//                 client_message->messageID = hostedChannels[i].messagesCount + 1;

//                 // 2. Add message to channel
//                 hostedChannels[i].messages[hostedChannels[i].messagesCount] = *client_message;

//                 // 3. update total message in channel (before adding message to ensure correct message id given)
//                 hostedChannels[i].messagesCount += 1;

//                 break;
//             }
//         }
//     }

//     // // works: update unreadMsg property for all subcribed clients except client with this clientID
//     updateUnreadMsgCountForSubscribers(channel);

//     // // end of critical section ?
// }

// void print_channel_messages()
// {
//     int i, j = 0;
//     printf("\n==== All Channels Messages ===============================================\n");

//     for (i = 0; i < MAX_CHANNELS; i++)
//     {
//         if (hostedChannels[i].messagesCount > 0)
//         {
//             for (j = 0; j < MAX_MESSAGE_COUNT; j++)
//             {
//                 if (hostedChannels[i].messages[j].messageID >= 1)
//                 {
//                     printf("ChannelID: %d, MessageID: %d | ClientID %d, Content: %s \n",
//                            hostedChannels[i].channelID,
//                            hostedChannels[i].messages[j].messageID,
//                            hostedChannels[i].messages[j].ownerID,
//                            hostedChannels[i].messages[j].content);
//                 }
//             }
//         }
//     }
// }

// message_t searchNextMsgInList(channel_t *channel, client_t *client)
// {
//     int i, j, k = 0;

//     // printf("\n =====>: clientt unReads %d\n", client->unReadMsg);
//     // printf("\n =====>: channel id %d\n", channel->channelID);

//     message_t unreadMessage;
//     int clientsNextMessageToReadIndex = client->messageQueueIndex + 1;

//     for (i = 0; i < MAX_CHANNELS; i++)
//     {
//         if (hostedChannels[i].channelID == channel->channelID)
//         { // if channel id match
//             for (k = 0; k < MAX_CLIENTS; k++)
//             {
//                 if (hostedChannels[i].subscribers[k].clientID == client->clientID)
//                 { // if client id also matches
//                     for (j = 0; j < MAX_MESSAGE_COUNT; j++)
//                     {
//                         if (hostedChannels[i].messages[j].messageID == clientsNextMessageToReadIndex)
//                         { // next message in the client's unread index

//                             // if messageID is the next in messageQueueIndex for client
//                             unreadMessage = hostedChannels[i].messages[j];
//                             hostedChannels[i].subscribers[k].messageQueueIndex += 1; // update messageQueueIndex

//                             hostedChannels[i].subscribers[k].readMsg += 1;   // update read messages count
//                             hostedChannels[i].subscribers[k].unReadMsg -= 1; // update unread messages count
//                         }
//                     }
//                 }
//             }
//         }
//     }

//     return unreadMessage;
// }

// message_t readNextMsgFromChannel(channel_t *channel, client_t *client)
// {

//     message_t unreadMessage = searchNextMsgInList(channel, client);

//     // printf("\n 2.Next msg: %s\n", unreadMessage.content);

//     return unreadMessage;
// }

// // // NEXT: returns unread message from all channels one by one
// // message_t readNextMsgFromAllChannel(int clientID)
// // {
// //     // int i = 0;
// //     // client_t *foundClient;
// //     // message_t unreadMessage;
// //     // channel_t *channel;

// //     // for (i = 0; i < MAX_CHANNELS; i++)
// //     // {
// //     //     channel = &hostedChannels[1];
// //     //     foundClient = findSubscriberInChannel(channel, clientID);

// //     //     if (foundClient != NULL && foundClient->clientID == clientID)
// //     //     { // Found channel client is subscribed too!
// //     //         unreadMessage = readNextMsgFromChannel(channel, foundClient);

// //     //         return unreadMessage;
// //     //     }
// //     // }
// // }

// // // counts the nextUnread message from all channels, returns the count
// // int nextMessageCountInChannels(int clientID)
// // {
// //     // int i = 0;
// //     // int totalUnreadMsg = 0;

// //     // client_t *foundClient;
// //     // message_t unreadMessage;
// //     // channel_t *channel;

// //     // for (i = 0; i < MAX_CHANNELS; i++)
// //     // {
// //     //     channel = &hostedChannels[i];
// //     //     if (channel != NULL)
// //     //     {
// //     //         foundClient = findSubscriberInChannel(channel, clientID);

// //     //         // test below
// //     //         if (foundClient != NULL)
// //     //         {
// //     //             printf("\n\nclientid %d, CHANNEL ID %d given. \n", clientID, channel->channelID);
// //     //             printf("\n Result:FOUND client ID: %d \n", foundClient->clientID);
// //     //         }
// //     //         // test above

// //     //         if (foundClient != NULL && foundClient->clientID == clientID)
// //     //         { // Found channel client is subscribed too!

// //     //             printf("\n #Subscriber found in channel \n");

// //     //             unreadMessage = readNextMsgFromChannel(channel, foundClient);

// //     //             if (&unreadMessage != NULL)
// //     //             {
// //     //                 totalUnreadMsg += 1;
// //     //                 printf("\nmessage FOUND ID %d %s\n", unreadMessage.ownerID, unreadMessage.content);
// //     //                 printf("total msg num: %d\n", totalUnreadMsg);
// //     //             }
// //     //         }
// //     //     }
// //     //     // printf("for loop i = %d", i);
// //     // }

// //     // printf("()()() count final: %d\n\n", totalUnreadMsg);

// //     // return totalUnreadMsg;
// // }

// message_t createStatusResponseMessage(char *message, int clientID)
// {

//     message_t statusMessage;

//     statusMessage.messageID = 1;
//     statusMessage.ownerID = clientID;

//     strcpy(statusMessage.content, message);

//     return statusMessage;
// }