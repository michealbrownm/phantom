<?php
require "vendor/autoload.php";
require "GPBMetadata/Common.php";
require "GPBMetadata/Chain.php";

use Google\Protobuf\Internal\RepeatedField;
use Google\Protobuf\Internal\GPBType;

include "Protocol/Transaction.php";
include "Protocol/Operation.php";
include "Protocol/OperationCreateAccount.php";

$tran = new \Protocol\Transaction();
$tran->setNonce(1);
$tran->setSourceAddress("buQiu6i3aVP4SXBNmPsvJZxwYEcEBHUZd4Wj");
$tran->setMetadata("test");
$tran->setGasPrice(1000);
$tran->setFeeLimit(1000000);

$opers = new RepeatedField(GPBType::MESSAGE, Protocol\Operation::class);
$oper = new Protocol\Operation();
$oper->setSourceAddress("buQiu6i3aVP4SXBNmPsvJZxwYEcEBHUZd4Wj");
$oper->setMetadata("test");
$oper->setType(1);

$createAccount = new \Protocol\OperationCreateAccount();
$createAccount->setDestAddress("buQpCTN3x6K4pAyboF4C1CoUYbr2ooqRyCjZ");
$createAccount->setInitBalance(999999999897999998);
$oper->setCreateAccount($createAccount);

$opers[] = $oper;
$tran->setOperations($opers);

$serialTran = $tran->serializeToString();

$tranParse = new \Protocol\Transaction();
$tranParse->mergeFromString($serialTran);

echo $tranParse->getOperations()[0]->getSourceAddress();