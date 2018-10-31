English 


# Common

## Introduction
The common operations in C++ projects are separated from the business layer and belong to the common module. The functions are as follows:
- Parse parameters
- Parse configuration files
- Assist daemon process
- Encapsulate network layer
- Other functions, such as timer, state machine, proto to json, private key generation, database, etc

## Module Structure
Class name | Statement file | Function
|:--- | --- | ---
| `Argument` | [argument.h](./argument.h) | The argument used to parse the `main` function. It allows signatures, creating accounts, managing KeyStore, encrypting and decrypting, and converting bytes.
| `ConfigureBase` | [configure_base.h](./configure_base.h) | It parses the base class of the configuration file, providing basic operations for loading and getting values. The header file implements three sub-configuration load classes at the same time: `LoggerConfigure` log configuration, `DbConfigure` database configuration, `SSLConfigure` SSL configuration.
| `Daemon` | [daemon.h](./daemon.h) | A daemon aid that writes the latest timestamp to shared memory for monitoring by the daemon.
| `General` | [general.h](./general.h) | It defines global static variables that are general to the project, and provides small tool classes such as `Result` , `TimerNotify`, `StatusModule`, `SlowTimer`, `Global`, `HashWrapper`.
| `KeyStore` | [key_store.h](./key_store.h) | It implements the ability to create and parse KeyStore.
| `Network` | [network.h](./network.h) | It allows node network communication. Use `asio::io_service` as an asynchronous IO while managing all network connections, such as new, close, and keep heartbeat, etc., and responsible for distributing and parsing received messages. The `Connection` class is a wrapper for a single network connection, using `websocketpp::server` and `websocketpp::client` as management objects to implement functions such as sending data and obtaining TCP status.
| `Json2Proto`、`Proto2Json`| [pb2json.h](./pb2json.h) | It is used for data conversion between Google Proto buffer and JSON.
| `PublicKey`、`PrivateKey` | [private_key.h](./private_key.h) | `PublicKey` is a utility class for public key data conversion and verification signature data. `PrivateKey` is a utility class for private key data conversion and signature data.
| `Storage` | [storage.h](./storage.h) | `Storage` is the management class for the key vaule database. The interface class `KeyValueDb` of the database operation is also defined in the header file, and two subclasses `LevelDbDriver` and `RocksDbDriver` are derived, which are used to operate LevelDb and RocksDB respectively.
