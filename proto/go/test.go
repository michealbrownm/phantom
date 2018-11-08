package main

import (
	"fmt"
	"github.com/goout/3rd/proto"
	"github.com/goout/protocol"
)

func main() {
	sourceAddress := "buQmWJrdYJP5CPKTbkQUqscwvTGaU44dord8"
	var nonce int64 = 1
	var gasPrice int64 = 1000
	var feeLimit int64 = 1000 * gasPrice
	receiverAddress := "buQVkUUBKpDKRmHYWw1MU8U7ngoQehno165i"
	var amount int64 = 1000
	Transaction := &protocol.Transaction{
		SourceAddress: sourceAddress,
		Nonce:         nonce,
		FeeLimit:      feeLimit,
		GasPrice:      gasPrice,
		Operations: []*protocol.Operation{
			{
				Type: protocol.Operation_PAY_COIN,
				PayCoin: &protocol.OperationPayCoin{
					DestAddress: receiverAddress,
					Amount:      amount,
				},
			},
		},
	}

	// Serialization operation
	data, err := proto.Marshal(Transaction)
	if err != nil {
		fmt.Print("error");
	}
	fmt.Println(data)
}
