English 

## Introduction
The ledger module is mainly responsible for the execution of the ledger and block generation. It includes the following features:
- Generate the genesis account and genesis block based on configuration
- Execute transactions in the proposal after a consensus is reached
- Generate a new block based on the completed transaction package
- Synchronize blocks from the blockchain network regularly

## Module Structure
Class name | Statement file | Function
|:--- | --- | ---
|`Trie`                  | [trie.h](./trie.h)                                   | Dictionary tree base class. The dictionary tree is the underlying data query and access structure of `PHANTOM`. In addition to the dictionary features, `PHANTOM` also adds Merkel root's features to `Trie`. Trie defines the framework functionality of the dictionary tree and implements some of the interfaces.
|`KVTrie`                | [kv_trie.h](./kv_trie.h)                             | The derived class of `Trie` implements the function of the Merkel prefix tree.
|`Environment`           | [environment.h](./environment.h)                     | The execution container of the transaction, which provides transactional features for the transaction. The data that changes during the execution of the transaction will be written to the cache of `Environment`. After all the operations in the transaction have been executed, the update will be submitted uniformly.
|`FeeCalculate`          | [fee_calculate.h](./fee_calculate.h)                 | The cost calculation class defines the fee standard for various transaction operations and provides an external fee calculation interface.
|`AccountFrm`            | [account.h](./account.h)                             | Account class. The user's behavioral body on the `PHANTOM` chain records all user data including account attributes, account status, and content assets. All operations of the user are based on `AccountFrm`.
|`TransactionFrm`        | [transaction_frm.h](./transaction_frm.h)             | The transaction execution class is responsible for processing and executing transactions, and the specific operations within the transaction are executed by `OperationFrm`.
|`OperationFrm`          | [operation_frm.h](./operation_frm.h)                 | The operation execution class performs the operations in the transaction according to the operation type.
|`ContractManager`       | [contract_manager.h](./contract_manager.h)           |Smart contract management class. It provides code execution environment and management for smart contracts. This includes loading code interpreters, providing built-in variables and interfaces, contract code and parameter checking, code execution, and more. Primarily triggered by the operations of creating account and money transfering of `OperationFrm`.
|`LedgerManager`         | [ledger_manager.h](./ledger_manager.h)               | Ledger management class. It coordinates the execution management of the block, schedules each sub-module under `ledger` to generate a new block, write to database, and synchronize the latest block from the network regularly after executing the transaction in the consensus proposal.
|`LedgerContext`         | [ledgercontext_manager.h](./ledgercontext_manager.h) | The execution context of the ledger, which carries the content data and attribute status data of the ledger.
|`LedgerContextManager`  | [ledgercontext_manager.h](./ledgercontext_manager.h) | The management class of `LedgerContext` is convenient for multi-thread execution scheduling.
|`LedgerFrm`             | [ledger_frm.h](./ledger_frm.h)                       | The ledger execution class is responsible for the specific processing of the ledger. The main task is to transfer the transactions in the ledger one by one to `TransactionFrm` to execute.
## Workflow
- When the program starts, `LedgerManager` is initialized and the genesis Account and genesis Zone are created according to the configuration file.
- After the blockchain network starts running, `LedgerManager` receives the consensus proposal passed through the `glue` module and checks the validity of the proposal.
- After passing the legal check, hand over the consensus proposal to `LedgerContextManager`.
- `LedgerContextManager` generates an execution context `LedgerContext` object for processing the consensus proposal, and `LedgerContext` passes the proposal to `LedgerFrm` for specific processing.
- `LedgerFrm` creates an `Environment` object, provides a transaction container for executing the transactions within the proposal, and then extracts the transactions of the proposal one by one, and transfer them to `TransactionFrm`to process.
- `TransactionFrm` takes the operations in the transaction one by one and sends them to `OperationFrm` to execute.
- `OperationFrm` performs different operations within the transaction according to the type, and writes the data of the operation change to the cache of `Environment`’, where the operation of creating an account by `OperationFrm` is to create a contract account, or to perform a transfer operation (including transferring assets and transferring BU coins). It will trigger `ContractManager` to load and execute the contract code, and the data changed in the process of contract execution will also be written to the `Environment`.
- During the execution of the transaction, `FeeCalculate` is called to calculate the actual cost.
- After all operations in each transaction are completed, the change cache in `Environment` will be submitted for update.
- After all the transactions in the proposal have been executed, `LedgerManager` packages the proposal to generate a new block and writes the new block and updated data to the database.
- In addition, `LedgerManager` synchronizes the latest block from the blockchain network through timer regularly.

