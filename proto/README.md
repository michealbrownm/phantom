English 

## Introduction
The definition module for serialized message of `PHANTOM`. Serialization and deserialization of data are used in the internal communication, node communication, and permanent storage of the blockchain program. `PHANTOM` uses `protobuf` as the serialization tool. When using this module, first define the `message` structure according to the syntax standard defined by `protobuf`, then call the `protobuf` compilation tool to compile the file defining the `message` structure into the `C++` source file. Then, other source files of `PHANTOM` can directly call the `message` source file compiled by `protobuf`.

## Module Structure
File | Function
|:--- | ---
[chain.proto](./chain.proto)           | The serializable data structure used by the `ledger` module and the module interacting with `ledger`, including account, transaction, block header and other related structures.
[common.proto](./common.proto)         | Basic serializable data structures which are commonly used, including key-value pairs, signatures, `ping`, error codes, and so on.
[consensus.proto](./consensus.proto)   | Serializable data structures related to consensus and cost, including various types of consensus messages, validation nodes, and cost structures for `pbft`.
[merkeltrie.proto](./merkeltrie.proto) | The serializable data structure used by the Merkel prefix tree, including nodes, subnodes, and types.
[monitor.proto](./monitor.proto)       | Various serializable data structures used by the monitoring module, including registration information, blockchain status, and computer hardware resources.
[overlay.proto](./overlay.proto)       | The `overlay` module is a self-contained or serializable data structure used by the module of the `overlay` communication, including communication status, `peer` information, and account information.

## Other
There are other folders in the `proto` directory, such as `php`, `go`, `python`, etc., which are serialized source files of different languages ​​compiled by `*.proto` files to support `SDK` calls in different languages ​​for PHANTOM.
