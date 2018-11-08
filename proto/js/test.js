const chain = require('./protocol/chain_pb.js')
const Long = require('./protocol/long.js')

let tx = new chain.Transaction();
tx.setSourceAddress('buQiu6i3aVP4SXBNmPsvJZxwYEcEBHUZd4Wj');
tx.setGasPrice(1000);
tx.setFeeLimit(Long.fromString('999999999897999986'));
tx.setNonce(2);

let opPayCoin = new chain.OperationPayCoin();
opPayCoin.setDestAddress('buQpCTN3x6K4pAyboF4C1CoUYbr2ooqRyCjZ');
opPayCoin.setAmount(Long.fromString('999999999897999998'));
let op = new chain.Operation();
op.setType(chain.Operation.Type.PAY_COIN);
op.setPayCoin(opPayCoin);
tx.addOperations(op);

console.log(Long.fromValue(tx.getFeeLimit()).toString())