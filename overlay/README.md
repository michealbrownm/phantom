# P2P Network

## Introduction
This module enables P2P network communication and parses service message. 
- Node connection management. It includes configuring known nodes, discovering new nodes, actively connecting to other nodes, and listening to other node connections.
- Node communication. It provides message interaction with other nodes, such as initiating transactions, block synchronization, block consensus messages, and so on.
- Message parsing. After receiving the message, it is parsed and passed to other internal modules.

## Module Structure

Class name | Statement file | Function
|:--- | --- | ---
| `PeerManager` | [peer_manager.h](./peer_manager.h) | The manager of the node module. It provides a timer and thread execution environment for the module, as well as interfaces for sending unicast and broadcast messages to other modules.
|`PeerNetwork`|  [peer_network.h](./peer_network.h) | It enables node message to be processed . It extends from the `Network` class. This class has two functions: one is to manage node connections: such as connecting other nodes, emptying the failed connection; the second is to process the received messages, such as acquiring nodes, initiating transactions, synchronizing blocks, block consensus, and ledger upgrades.
|`Network`|  [network.h](../common/network.h)  | It enables node network communication. Use the `asio::io_service` asynchronous IO module to listen to network events and manage all network connections, such as creating new connections, closing connections, keeping heartbeats, and distributing and analyzing received messages.
|`Peer`|  [peer.h](./peer.h) | It is used to encapsulate TCP connections. It extends from the `Connection` class. Refer to [network.h](../common/network.h). It provides an interface for sending data, providing the current state of TCP, using `websocketpp::server` and `websocketpp::client` as management objects.
|`Broadcast`| [broadcast.h](./broadcast.h)  | The manager of the broadcast message. It supports sending broadcast messages, recording broadcast messages, and clearing broadcast messages. It is called by `PeerNetwork`.


## Protocol Definition
P2P message is defined by Google protocol buffer. Refer to [overlay.proto](../proto/overlay.proto) and the types are listed below:
```
OVERLAY_MSGTYPE_PEERS  #Obtain node info
OVERLAY_MSGTYPE_TRANSACTION  #Initiate transactions
OVERLAY_MSGTYPE_LEDGERS   #Obtain blocks
OVERLAY_MSGTYPE_PBFT    #Block consensus
OVERLAY_MSGTYPE_LEDGER_UPGRADE_NOTIFY   #Ledger upgrade
```

These messages can be classified as unicast and broadcast, as below:
- Unicast.`OVERLAY_MSGTYPE_PEER` and `OVERLAY_MSGTYPE_LEDGERS`
- Broadcast.`OVERLAY_MSGTYPE_TRANSACTION`,`OVERLAY_MSGTYPE_PBF` and `OVERLAY_MSGTYPE_LEDGER_UPGRADE_NOTIFY`
