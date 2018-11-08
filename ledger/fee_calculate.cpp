#include "fee_calculate.h"

namespace phantom{

	const int64_t OperationGasConfigure::create_account = 0;
	const int64_t OperationGasConfigure::issue_asset = 5000000;
	const int64_t OperationGasConfigure::pay_asset = 0;
	const int64_t OperationGasConfigure::set_metadata = 0;
	const int64_t OperationGasConfigure::set_sigure_weight = 0;
	const int64_t OperationGasConfigure::set_threshold = 0;
	const int64_t OperationGasConfigure::pay_coin = 0;
	const int64_t OperationGasConfigure::log = 0;
	const int64_t OperationGasConfigure::create_contract = 1000000;
	const int64_t OperationGasConfigure::set_privilege = 0;

    int64_t FeeCalculate::CaculateFee(const int64_t& price, const int64_t& gas){
		return price*gas;
	}

    int64_t FeeCalculate::GetOperationTypeGas(const protocol::Operation& op){
		const protocol::Operation_Type& op_type = op.type();
		switch (op_type) {
		case protocol::Operation_Type_UNKNOWN:
			return 0;
		case protocol::Operation_Type_CREATE_ACCOUNT:
		{
			const protocol::OperationCreateAccount& createaccount = op.create_account();
			std::string javascript = createaccount.contract().payload();
			if (!javascript.empty()) {
				return OperationGasConfigure::create_contract;
			}
			return OperationGasConfigure::create_account;
		}
        case protocol::Operation_Type_PAY_ASSET:
			return OperationGasConfigure::pay_asset;
		case protocol::Operation_Type_ISSUE_ASSET:
			return OperationGasConfigure::issue_asset;
		case protocol::Operation_Type_SET_METADATA:
			return OperationGasConfigure::set_metadata;
		case protocol::Operation_Type_SET_SIGNER_WEIGHT:
			return OperationGasConfigure::set_sigure_weight;
		case protocol::Operation_Type_SET_THRESHOLD:
			return OperationGasConfigure::set_threshold;
		case protocol::Operation_Type_PAY_COIN:
			return OperationGasConfigure::pay_coin;
		case protocol::Operation_Type_LOG:
			return OperationGasConfigure::log;
		case protocol::Operation_Type_SET_PRIVILEGE:
			return OperationGasConfigure::set_privilege;
		default:
			return 0;
		}
	}

}