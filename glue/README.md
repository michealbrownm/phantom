English 

## Introduction
`glue` is the glue module of the `PHANTOM` blockchain, which is responsible for bonding the important modules together and acting as an intermediary to provide interactive information transfer services for each module. It mainly includes the following functions:
- Connect the `console` module so that users can submit transactions to the `glue` transaction pool via the command line
- Connect the `api` module so that users can submit transactions to the `glue` transaction pool via the light client or the `http` tool
- Connect the `overlay` module so that `glue` can be connected to other modules to call the 'overlay` `p2p` network via `glue` for node communication
- Connect the `consensus` module so that `consensus` can send and receive consensus messages via `glue` which calls `overlay`, or submit the consensus proposal to `ledger` via `glue`
- Connect the `ledger` module so that the consensus proposal for `consensus` can be submitted to `ledger` via `glue` to generate a block; the set of authentication nodes updated by `ledger` or the upgrade information can be submitted to `consensus` to update and take effect; it also enables `ledger` to synchronize blocks via `overlay`

## Module Structure
Class name | Statement file | Function
|:--- | --- | ---
|`GlueManager`      | [glue_manager.h](./glue_manager.h)            | Glue management class, the interface provided by `GlueManager` is mainly the packaging of the external interfaces of each module, and each module communicates with each other by calling the wrapper interface provided by `GlueManager`.
|`LedgerUpgradeFrm` | [glue_manager.h](./glue_manager.h)            | Responsible for the `PHANTOM` account upgrade. The `PHANTOM` blockchain provides backward compatibility. After each verification node is upgraded, it will broadcast its own upgrade information. After the upgraded verification nodes reach a certain ratio, all verification nodes follow the new version to generate a block, otherwise the block is generated according to the old version. `LedgerUpgradeFrm` is responsible for handling various processes of the `PHANTOM` upgrade.
|`TransactionQueue` | [transaction_queue.h](./transaction_queue.h)  | Transaction pool. Put the user-submitted transaction into the transaction cache queue and double-sorting the transaction according to the account `nonce` value and `gas_price` for the `GlueManager` package consensus proposal.
