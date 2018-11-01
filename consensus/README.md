English 

## Introduction
The `consensus` module is one of the core modules of the blockchain. PHANTOM packages the transaction collection and creates a consensus proposal for further processing by the `consensus` module. Each node in the blockchain will review and vote on the proposal through the `consensus` module. After each node agrees on the proposal, then the proposal will be submitted to other modules to execute the transactions within it, and finally generate a block.

## Module Structure
Class name | Statement file | Function
|:--- | --- | ---
|`ConsensusManager` | [consensus_manager.h](./consensus_manager.h) | It is responsible for the overall management of the consensus module and the interaction of other modules.
|`Consensus`        | [consensus.h](./consensus.h)                 | It defines the main functional framework of the consensus and implements some interfaces. Which consensus algorithm is used and the detailed implementation is handled by the derived class.
|`Pbft`             | [bft.h](./bft.h)                             | According to the `pbft` algorithm, the concrete consensus processing class is derived from the `Consensus` class, which is responsible for processing the specific consensus process.
|`PbftInstance`     | [bft_instance.h](./bft_instance.h)           | The consensus instance class for the proposal. Each `PbftInstance` object corresponds to a consensus instance of a proposal, records the content of the proposal, the stage of the consensus process, and the consensus messages collected at each consensus stage to support `Pbft` to do specific consensus processing based on specific examples.
|`ConsensusMsg`     | [consensus_msg.h](./consensus_msg.h)         | The wrapper class of the consensus message. In addition to the content of the consensus message, it includes a message sequence number, a message type, a node that sends the message, a message hash, etc., for the message receiver to check and classify the process.

## Workflow
- After the main node's `ConsensusManager` receives the consensus proposal submitted by the `glue` module, it is handled by `Pbft` through Consensus.
- `Pbft` generates a `PbftInstance` instance for the consensus proposal, writes the contents of the proposal to the `PbftInstance` instance, and then broadcasts the proposal to other consensus node consensus.
- The consensus nodes use the message wrapped by `ConsensusMsg` to communicate, and the communication content and processing data of each stage of the consensus are written to the `PbftInstance` instance.
- Finally, after reaching an agreement, the consensus proposal is passed to the `ledger` module via the `glue` module.

