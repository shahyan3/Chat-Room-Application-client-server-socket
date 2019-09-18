const channel1 = {
  channelID: 1,
  totalMsg: 0,
  messageHead: null,
  subscriberHead: null,
  subscriberTail: null
};

const message1 = {
  messageID: 1,
  ownerID: 1,
  content: "This is a message! 1",
  next: null
};
const message2 = {
  messageID: 2,
  ownerID: 1,
  content: "This is a message! 2",
  next: null
};
const message3 = {
  messageID: 3,
  ownerID: 1,
  content: "This is a message! 3",
  next: null
};
const message4 = {
  messageID: 4,
  ownerID: 1,
  content: "This is a message! 4",
  next: null
};

const client1 = {
  clientID: 1,
  readMsg: 0,
  unReadMsg: 0,
  totalMessageSent: 0,
  messageQueueIndex: 0 /* index = message that it read currently, NOT NEXT ONE */,
  entryIndexConstant: null /* index of msg queue at which the client subed */,
  next: null
};
const client2 = {
  clientID: 2,
  readMsg: 0,
  unReadMsg: 0,
  totalMessageSent: 0,
  messageQueueIndex: 0 /* index = message that it read currently, NOT NEXT ONE */,
  entryIndexConstant: null /* index of msg queue at which the client subed */,

  next: null
};
const client3 = {
  clientID: 3,
  readMsg: 0,
  unReadMsg: 0,
  totalMessageSent: 0,
  messageQueueIndex: 0 /* index = message that it read currently, NOT NEXT ONE */,
  entryIndexConstant: null /* index of msg queue at which the client subed */,

  next: null
};
const client4 = {
  clientID: 4,
  readMsg: 0,
  unReadMsg: 0,
  totalMessageSent: 0,
  messageQueueIndex: 0 /* index = message that it read currently, NOT NEXT ONE */,
  entryIndexConstant: null /* index of msg queue at which the client subed */,
  next: null
};

//request comes in by client one to subscribe
function subscribe(client, channelID) {
  /*  critical section start */

  // find the channel requested
  hostedChannels.forEach(channel => {
    if (channel.channelID == channelID) {
      // sorting through "linkedlist"
      var currentNode = channel.subscriberHead;

      if (channel.subscriberHead == null) {
        const message = findLastMsgInLinkedList(channel);
        // works too! message == null ? client.messageQueueIndex = 0 : client.messageQueueIndex = message.messageID;
        client.messageQueueIndex = channel.totalMsg;
        client.entryIndexConstant = channel.totalMsg; // assign entry index based on msg in list constant

        channel.subscriberHead = client;
        channel.subscriberTail = channel.subscriberHead; // track the last node
      } else {
        while (currentNode.next != null) {
          // if the next value null = last node
          currentNode = currentNode.next;
        }
        const message = findLastMsgInLinkedList(channel);
        // update the read message queue index for the above message read.
        // works too! message == null ? client.messageQueueIndex = 0 : client.messageQueueIndex = message.messageID;
        client.messageQueueIndex = channel.totalMsg;

        channel.totalMsg = channel.totalMsg;

        // assign the index value of the last msg as subbed entry (never changes!)
        client.entryIndexConstant = channel.totalMsg;

        currentNode.next = client; // on the last node add next client
        channel.subscriberTail = currentNode.next; // track the last node
      }

      // subHead.next = client;  //  add client at end of list
    } else {
      return Error("no chanel with id found");
    }
    /*  critical section end ?*/
  });
}
// when subscribing to channel, finds out the last message in the channel, and will be used to add the message id to the subscribing client
function findLastMsgInLinkedList(channel) {
  var lastMsgInList = null;
  var msgHead = channel.messageHead;

  if (channel.messageHead == null) {
    // no messages in linkedlist
    lastMsgInList = null;
  }

  while (msgHead != null) {
    lastMsgInList = msgHead;
    msgHead = msgHead.next;
  }

  return lastMsgInList;
}

function writeToChannelReq(clientID, channelID, newMessage) {
  var tailMsg = null;
  hostedChannels.forEach(channel => {
    if (channel.channelID == channelID) {
      // channel exists
      var client = findSubsriberInList(channel, clientID);
      if (client.clientID == clientID) {
        // client is subscribed to channel
        // critical section ?
        tailMsg = channel.messageHead;
        channel.messageHead = newMessage;
        channel.messageHead.next = tailMsg;
        // works: update unreadMsg property for all subcribed clients except client with this clientID
        updateUnreadMsgCountForSubscribers(channel);

        // update total msg count for channel.totalMessages
        channel.totalMsg += 1;
        // end of critical section ?
      } else {
        console.log("error: no client subscribed to the ");
      }
    }
  });
}

function findSubsriberInList(channel, clientID) {
  var client = null;
  var subHead = channel.subscriberHead;

  if (subHead == null) {
    Error("no subscriber in the list");
  }

  while (subHead != null) {
    if (subHead.clientID == clientID) {
      client = subHead;
      break;
    }
    subHead = subHead.next;
  }
  return subHead;
}
function updateUnreadMsgCountForSubscribers(channel) {
  var currentNode = channel.subscriberHead;
  if (currentNode == null) {
    Error("no subscribers found in channel");
  }
  while (currentNode != null) {
    currentNode.unReadMsg += 1;
    currentNode = currentNode.next;
  }
}

function readNextMsgFromChannel(clientID, channelID) {
  var unreadMessage = null;
  hostedChannels.forEach(channel => {
    if (channel.channelID == channelID) {
      // channel exists
      var client = findSubsriberInList(channel, clientID);
      //  find the NEXT unread message in the channel messag linked list
      var nextMessage = searchNextMsgInList(channel, client);
      nextMessage == null
        ? (unreadMessage = "no message found")
        : (unreadMessage = nextMessage);
    }
  });
  return unreadMessage;
}
function searchNextMsgInList(channel, client) {
  var unreadMessage = null;
  var currentNode = channel.messageHead;
  var clientsNextMessageToReadIndex = client.messageQueueIndex + 1;

  if (currentNode == null) {
    // no messages in channel
    unreadMessage = null;
    console.log("no messages in channel!");
  }

  while (currentNode != null) {
    if (currentNode.messageID == clientsNextMessageToReadIndex) {
      // if messageID is the next in messageQueueIndex for client
      unreadMessage = currentNode;
      client.messageQueueIndex += 1; // update messageQueueIndex

      client.readMsg += 1; // update read messages count
      client.unReadMsg -= 1; // update unread messages count
    }
    currentNode = currentNode.next;
  }
  return unreadMessage;
}

// // output
hostedChannels = [channel1]; // add channel

subscribe(client1, 1); // subscribe client to channel
writeToChannelReq(1, 1, message1); // write to channel;
writeToChannelReq(1, 1, message2); // write to channel;

subscribe(client2, 1); // subscribe client to channel
writeToChannelReq(1, 1, message3); // write to channel;

subscribe(client3, 1); // subscribe client to channel
writeToChannelReq(1, 1, message4); // write to channel;
// writeToChannelReq(1, 1, message3); // write to channel;

subscribe(client4, 1); // subscribe client to channel
// writeToChannelReq(2, 1, message3); // write to channel;

// console.log("subed client to channel: ",hostedChannels[0].subscriberHead); // works! added!

// console.log("write to channel client1: ", hostedChannels[0].messageHead);
// console.log("client's stats: ", hostedChannels[0].subscribers[0])

var m1 = readNextMsgFromChannel(1, 1);
// console.log("message says ", m1)
var m1 = readNextMsgFromChannel(1, 1);

var m1 = readNextMsgFromChannel(2, 1);

// WORKING V3

console.log("first subscriber on channel: ", hostedChannels[0].subscriberHead);
console.log("Last subscriber on channel: ", hostedChannels[0].subscriberTail);

console.log("channel 1 %%%: ", hostedChannels[0]);
