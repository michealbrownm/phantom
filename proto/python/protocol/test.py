import chain_pb2
import sys

transaction = chain_pb2.Transaction()
transaction.nonce = 1
transaction.source_address = "buQiu6i3aVP4SXBNmPsvJZxwYEcEBHUZd4Wj"
transaction.gas_price = 1000
transaction.fee_limit = 1000000
transaction.metadata = "test"

operation = transaction.operations.add()
operation.type = chain_pb2.Operation.CREATE_ACCOUNT
operation.source_address = "buQiu6i3aVP4SXBNmPsvJZxwYEcEBHUZd4Wj"
operation.metadata = "test"

createAccount = operation.create_account
createAccount.dest_address = "buQpCTN3x6K4pAyboF4C1CoUYbr2ooqRyCjZ"
createAccount.init_balance = 1000000000

serial_str = transaction.SerializeToString()

print len(serial_str)