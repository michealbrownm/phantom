
#ifndef FEE_COMPULATE_H

#include<cstdint>
#include<proto/cpp/chain.pb.h>

namespace phantom{

	struct OperationGasConfigure {
		const static int64_t create_account;
		const static int64_t issue_asset;
		const static int64_t payment;
		const static int64_t set_metadata;
		const static int64_t set_sigure_weight;
		const static int64_t set_threshold;
		const static int64_t pay_coin;
		const static int64_t log;
		const static int64_t create_contract;
	};

	class FeeCompulate {
	public:
		static int64_t ByteFee(const int64_t& price,const int64_t& tx_size);
		static int64_t OperationFee(const int64_t& price, const protocol::Operation_Type& op_type, const protocol::Operation* op = nullptr);
	};
}

#endif // !FEE_COMPULATE_H

