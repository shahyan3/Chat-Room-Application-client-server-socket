const channel1 = {
  channelID: 1,
  totalMsg: 0,
  messageHead: null,
  subscribers: []
}

const message1 = {
  messageID: 1,
  ownerID: 1,
  content: "This is a message! 1",
}
const message2 = {
  messageID: 2,
  ownerID: 1,
  content: "This is a message! 2",
}
const message3 = {
  messageID: 3,
  ownerID: 1,
  content: "This is a message! 3",
}

const client1 = {
  clientID: 1,
  readMsg: 0,
  unReadMsg: null,
  totalMessageSent: null,
  messageQueueIndex: null, /* index = message that it read currently, NOT NEXT ONE */
  next: null
}

//request comes in by client one to subscribe
function subscribe(client, channelID) {
  // find the channel requested
  hostedChannels.forEach((channel) => {
    if(channel.channelID == channelID) {
        // sorting through "linkedlist"
        const message = findLastMsgInLinkedList(channel);
        // add the lastMessageSubscribePoint client only accesses msgs after subscription
        message == null ? client.messageQueueIndex = 0 : client.messageQueueIndex = message.messageID;
              
        channel.subscribers.push(client); // add client 
    } else {
      return Error("no chanel with id found");
    }
  });
}
// when subscribing to channel, finds out the last message in the channel, and will be used to add the message id to the subscribing client
function findLastMsgInLinkedList(channel) {
  var lastMsgInList = null;
   
   if(channel.messageHead == null ) { // no messages in linkedlist
      lastMsgInList = null;
    }

    while(channel.messageHead != null ) {
       lastMsgInList = channel.messageHead;
       channel.messageHead = channel.messageHead.next;
     }

  return lastMsgInList;
}

function writeToChannelReq(clientID, channelID, newMessage) {
  var tailMsg = null;
  hostedChannels.forEach((channel) => {
    if(channel.channelID == channelID) { // channel exists
      channel.subscribers.forEach((client) => {
        if(client.clientID == clientID) { // client is subscribed to channel
          // critical section ?
         tailMsg = channel.messageHead;
         channel.messageHead = newMessage;
         channel.messageHead.next = tailMsg;
          // end of critical section ?

          // TODO: update unreadMsg property for all subcribed clients except client with this clientID
          // updateUnreadMsgCountForSubscribers(channel);
        } else {
          Error("no client subscribed to the ")
        }
      });  
    }
  });
}

function readNextMsgFromChannel(clientID, channelID) {
  var unreadMessage = null;
  hostedChannels.forEach((channel) => {
    if(channel.channelID == channelID) { // channel exists
      channel.subscribers.forEach((client) => {
        if(client.clientID == clientID) { // client subscribed true
          // find the NEXT unread message in the channel messag linked list
          var nextMessage = searchNextMsgInList(channel, client);
          nextMessage == null ? unreadMessage = "no message found" : unreadMessage = nextMessage;
         }
      });
    }
  });
  return unreadMessage;
}

function searchNextMsgInList(channel, client) {
    var unreadMessage = null;

    var head = channel.messageHead;
    var next;
     var clientsNextMessageToReadIndex = client.messageQueueIndex + 1;

    if(head== null ) { // no messages in channel 
      unreadMessage = null; 
    }

    while(head != null) {
       if(head.messageID == clientsNextMessageToReadIndex) { // if messageID is the next in messageQueueIndex for client 
         unreadMessage = head;
         client.messageQueueIndex +=1 ; // update messageQueueIndex

        client.readMsg +=1; // update read messages count
        client.unReadMsg -=1; // update unread messages count
       }
       head = head.next;
    }

    return unreadMessage;
}


 
// // output 
hostedChannels = [channel1]; // add channel
 
subscribe(client1, 1); // subscribe client to channel
// console.log("subed client to channel: ",hostedChannels[0].subscribers[0]); // works! added!



writeToChannelReq(1, 1, message1); // write to channel;
writeToChannelReq(1, 1, message2); // write to channel;
writeToChannelReq(1, 1, message3); // write to channel;
console.log("write to channel client1: ", hostedChannels[0].messageHead);
// console.log("client's stats: ", hostedChannels[0].subscribers[0])

var m1 = readNextMsgFromChannel(1, 1);
console.log("unread message: ", m1)
 

 var m2 = readNextMsgFromChannel(1, 1);
console.log("unread message: ", m2);

var ms2 = readNextMsgFromChannel(1, 1);
console.log("unread message: ", ms2);
 

// var ms3 = readNextMsgFromChannel(1, 1);
// console.log("unread message: ", ms3)


// var ms4 = readNextMsgFromChannel(1, 1);
// console.log("unread message: ", ms4)

// WORKING V1


console.log("client's stats: ", hostedChannels[0].subscribers[0])

