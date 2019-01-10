/*
	phantom is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	phantom is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with phantom.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <utils/strings.h>
#include <ledger/ledger_manager.h>
#include "transaction_frm.h"
#include "operation_frm.h"
#include "contract_manager.h"
#include "fee_compulate.h"


namespace phantom {
	OperationFrm::OperationFrm(const protocol::Operation &operation, TransactionFrm* tran, int32_t index) :
		operation_(operation), transaction_(tran), index_(index), ope_fee_(0){
		if (tran) {
			ope_fee_ = FeeCompulate::OperationFee(tran->GetGasPrice(), operation.type(), &operation);
		}
	}

	OperationFrm::~OperationFrm() {}

	Result OperationFrm::GetResult() const {
		return result_;
	}

	int64_t OperationFrm::GetOpeFee() const {
		return ope_fee_;
	}

	Result OperationFrm::CheckValid(const protocol::Operation& operation, const std::string &source_address) {
		Result result;
		result.set_code(protocol::ERRCODE_SUCCESS);
		auto type = operation.type();
		const protocol::OperationCreateAccount& create_account = operation.create_account();
		const protocol::OperationPayment& payment = operation.payment();
		const protocol::OperationIssueAsset& issue_asset = operation.issue_asset();

		if (!phantom::PublicKey::IsAddressValid(source_address)) {
			result.set_code(protocol::ERRCODE_INVALID_ADDRESS);
			result.set_desc(utils::String::Format("Source address should be a valid account address"));
			return result;
		}
		//const auto &issue_property = issue_asset.
		switch (type) {
		case protocol::Operation_Type_CREATE_ACCOUNT:
		{
			if (!phantom::PublicKey::IsAddressValid(create_account.dest_address())) {
				result.set_code(protocol::ERRCODE_INVALID_ADDRESS);
				result.set_desc(utils::String::Format("Dest account address(%s) invalid", create_account.dest_address().c_str()));
				break;
			}

			if (!create_account.has_priv()) {
				result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
				result.set_desc(utils::String::Format("Dest account address(%s) has no priv object", create_account.dest_address().c_str()));
				break;
			} 

			const protocol::AccountPrivilege &priv = create_account.priv();
			if (priv.master_weight() < 0 || priv.master_weight() > UINT32_MAX) {
				result.set_code(protocol::ERRCODE_WEIGHT_NOT_VALID);
				result.set_desc(utils::String::Format("Master weight(" FMT_I64 ") is larger than %u or less 0", priv.master_weight(), UINT32_MAX));
				break;
			}

			//for signers
			std::set<std::string> duplicate_set;
			bool shouldBreak = false;
			for (int32_t i = 0; i < priv.signers_size(); i++) {
				const protocol::Signer &signer = priv.signers(i);
				if (signer.weight() < 0 || signer.weight() > UINT32_MAX) {
					result.set_code(protocol::ERRCODE_WEIGHT_NOT_VALID);
					result.set_desc(utils::String::Format("Signer weight(" FMT_I64 ") is larger than %u or less 0", signer.weight(), UINT32_MAX));
					shouldBreak = true;
					break;
				}

				if (signer.address() == create_account.dest_address()) {
					result.set_code(protocol::ERRCODE_INVALID_ADDRESS);
					result.set_desc(utils::String::Format("Signer address(%s) can't be equal to the source address", signer.address().c_str()));
					shouldBreak = true;
					break;
				}

				if (!PublicKey::IsAddressValid(signer.address())) {
					result.set_code(protocol::ERRCODE_INVALID_ADDRESS);
					result.set_desc(utils::String::Format("Signer address(%s) is not valid", signer.address().c_str()));
					shouldBreak = true;
					break;
				}

				if (duplicate_set.find(signer.address()) != duplicate_set.end()) {
					result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
					result.set_desc(utils::String::Format("Signer address(%s) duplicated", signer.address().c_str()));
					shouldBreak = true;
					break;
				} 

				duplicate_set.insert(signer.address());
			}
			if (shouldBreak) break;

			//for threshold
			if (!priv.has_thresholds()) {
				result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
				result.set_desc(utils::String::Format("dest account address(%s) has no threshold object", create_account.dest_address().c_str()));
				break;
			}

			const protocol::AccountThreshold &threshold = priv.thresholds();
			if (threshold.tx_threshold() < 0) {
				result.set_code(protocol::ERRCODE_THRESHOLD_NOT_VALID);
				result.set_desc(utils::String::Format("Low threshold(" FMT_I64 ") is less than 0", threshold.tx_threshold()));
				break;
			}

			std::set<int32_t> duplicate_type;
			for (int32_t i = 0; i < threshold.type_thresholds_size(); i++) {
				const protocol::OperationTypeThreshold  &type_thresholds = threshold.type_thresholds(i);
				if (type_thresholds.type() > 100 || type_thresholds.type() <= 0) {
					result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
					result.set_desc(utils::String::Format("Operation type(%u) not support", type_thresholds.type()));
					break;
				}

				if (type_thresholds.threshold() < 0) {
					result.set_code(protocol::ERRCODE_THRESHOLD_NOT_VALID);
					result.set_desc(utils::String::Format("Operation type(%d) threshold(" FMT_I64 ") is less than 0", (int32_t)type_thresholds.type(), type_thresholds.threshold()));
					break;
				}

				if (duplicate_type.find(type_thresholds.type()) != duplicate_type.end()) {
					result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
					result.set_desc(utils::String::Format("Operation type(%u) duplicated", type_thresholds.type()));
					break;
				} 
				
				duplicate_type.insert(type_thresholds.type());
			}

			//if it's contract then {master_weight:0 , thresholds:{tx_threshold:1} }
			if (create_account.contract().payload() != ""){
 				if (create_account.contract().payload().size() > General::CONTRACT_CODE_LIMIT) {
					result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
					result.set_desc(utils::String::Format("Contract payload size(" FMT_SIZE ") > limit(%d)",
						create_account.contract().payload().size(), General::CONTRACT_CODE_LIMIT));
					break;
				}

				if (!(priv.master_weight() == 0 &&
					priv.signers_size() == 0 &&
					threshold.tx_threshold() == 1 &&
					threshold.type_thresholds_size() == 0 
					)) {
					result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
					result.set_desc(utils::String::Format("Contract account 'priv' config must be({master_weight:0, thresholds:{tx_threshold:1}})"));
					break;
				}
				
				std::string src = create_account.contract().payload();
				result = ContractManager::Instance().SourceCodeCheck(Contract::TYPE_V8, src);
			}

			for (int32_t i = 0; i < create_account.metadatas_size(); i++){
				const auto kp = create_account.metadatas(i);
				if (kp.key().size() == 0 || kp.key().size() > General::METADATA_KEY_MAXSIZE) {
					result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
					result.set_desc(utils::String::Format("Length of the key should be between [1, %d]. key=%s,key.length=%d",
						General::METADATA_KEY_MAXSIZE, kp.key().c_str(), kp.key().length()));
				}

				if (kp.value().size() > General::METADATA_MAX_VALUE_SIZE){
					result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
					result.set_desc(utils::String::Format("Length of the value should be between [1, %d].key=%s,value.length=%d",
						General::METADATA_MAX_VALUE_SIZE, kp.key().c_str(), kp.value().length()));
				}
			}
			break;
		}
		case protocol::Operation_Type_PAYMENT:
		{
			if (payment.has_asset()){
				if (payment.asset().key().type() != 0){
					result.set_code(protocol::ERRCODE_ASSET_INVALID);
					result.set_desc(utils::String::Format("payment asset type must be 0"));
					break;
				}

				if (payment.asset().amount() <= 0) {
					result.set_code(protocol::ERRCODE_ASSET_INVALID);
					result.set_desc(utils::String::Format("amount should be bigger than 0"));
					break;
				}

				std::string trim_code = payment.asset().key().code();
				//utils::String::Trim(trim_code);
				if (trim_code.size() == 0 || trim_code.size() > General::ASSET_CODE_MAX_SIZE) {
					result.set_code(protocol::ERRCODE_ASSET_INVALID);
					result.set_desc(utils::String::Format("asset code length should between (0,64]"));
					break;
				}

				if (!phantom::PublicKey::IsAddressValid(payment.asset().key().issuer())) {
					result.set_code(protocol::ERRCODE_ASSET_INVALID);
					result.set_desc(utils::String::Format("asset issuer should be a valid account address"));
					break;
				}
			}

			if (source_address == payment.dest_address()) {
				result.set_code(protocol::ERRCODE_ACCOUNT_SOURCEDEST_EQUAL);
				result.set_desc(utils::String::Format("Source address(%s) equal to dest address", source_address.c_str()));
				break;
			} 

			if (!phantom::PublicKey::IsAddressValid(payment.dest_address())) {
				result.set_code(protocol::ERRCODE_INVALID_ADDRESS);
				result.set_desc(utils::String::Format("Dest address should be a valid account address"));
				break;
			}
			break;
		}

		case protocol::Operation_Type_ISSUE_ASSET:
		{
			if (issue_asset.amount() <= 0) {
				result.set_code(protocol::ERRCODE_ASSET_INVALID);
				result.set_desc(utils::String::Format("amount should be bigger than 0"));
				break;
			}

			std::string trim_code = issue_asset.code();
			trim_code = utils::String::Trim(trim_code);
			if (trim_code.size() == 0 || trim_code.size() > General::ASSET_CODE_MAX_SIZE ||
				trim_code.size() != issue_asset.code().size()) {
				result.set_code(protocol::ERRCODE_ASSET_INVALID);
				result.set_desc(utils::String::Format("Asset code length should between (0,64]"));
				break;
			}

			break;
		}
		case protocol::Operation_Type_SET_METADATA:
		{
			const protocol::OperationSetMetadata &set_metadata = operation.set_metadata();

			std::string trim = set_metadata.key();
			if (trim.size() == 0 || trim.size() > General::METADATA_KEY_MAXSIZE) {
				result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
				result.set_desc(utils::String::Format("Length of the key should be between [1, %d]. key=%s,key.length=%d",
					General::METADATA_KEY_MAXSIZE, trim.c_str(), trim.length()));
				break;
			}

			if (set_metadata.value().size() > General::METADATA_MAX_VALUE_SIZE) {
				result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
				result.set_desc(utils::String::Format("Length of the value should be between [0, %d]. key=%s,value.length=%d",
					General::METADATA_MAX_VALUE_SIZE, trim.c_str(), set_metadata.value().length()));
				break;
			}


			break;
		}
		case protocol::Operation_Type_SET_SIGNER_WEIGHT:
		{
			const protocol::OperationSetSignerWeight &operation_setoptions = operation.set_signer_weight();
			if (operation_setoptions.master_weight() < -1 || operation_setoptions.master_weight() > UINT32_MAX) {
				result.set_code(protocol::ERRCODE_WEIGHT_NOT_VALID);
				result.set_desc(utils::String::Format("Master weight(" FMT_I64 ") is larger than %u or less -1", operation_setoptions.master_weight(), UINT32_MAX));
				break;
			}

			for (int32_t i = 0; i < operation_setoptions.signers_size(); i++) {
				const protocol::Signer &signer = operation_setoptions.signers(i);
				if (signer.weight() < 0 || signer.weight() > UINT32_MAX) {
					result.set_code(protocol::ERRCODE_WEIGHT_NOT_VALID);
					result.set_desc(utils::String::Format("Signer weight(" FMT_I64 ") is larger than %u or less 0", signer.weight(), UINT32_MAX));
					break;
				}

				if (signer.address() == source_address) {
					result.set_code(protocol::ERRCODE_INVALID_ADDRESS);
					result.set_desc(utils::String::Format("Signer address(%s) can't be equal to the source address", signer.address().c_str()));
					break;
				}

				if (!PublicKey::IsAddressValid(signer.address())) {
					result.set_code(protocol::ERRCODE_INVALID_ADDRESS);
					result.set_desc(utils::String::Format("Signer address(%s) is not valid", signer.address().c_str()));
					break;
				}
			}

			break;
		}
		case protocol::Operation_Type_SET_THRESHOLD:
		{
			const protocol::OperationSetThreshold operation_setoptions = operation.set_threshold();

			if ( operation_setoptions.tx_threshold() < -1) {
				result.set_code(protocol::ERRCODE_THRESHOLD_NOT_VALID);
				result.set_desc(utils::String::Format("Low threshold(" FMT_I64 ") is less than -1", operation_setoptions.tx_threshold()));
				break;
			}

			for (int32_t i = 0; i < operation_setoptions.type_thresholds_size(); i++) {
				const protocol::OperationTypeThreshold  &type_thresholds = operation_setoptions.type_thresholds(i);
				if (type_thresholds.type() > 100 || type_thresholds.type() <= 0) {
					result.set_code(protocol::ERRCODE_THRESHOLD_NOT_VALID);
					result.set_desc(utils::String::Format("Operation type(%u) not support", type_thresholds.type()));
					break;
				}

				if (type_thresholds.threshold()  < 0 ) {
					result.set_code(protocol::ERRCODE_THRESHOLD_NOT_VALID);
					result.set_desc(utils::String::Format("Operation type(%d) threshold(" FMT_I64 ") is less than 0", (int32_t)type_thresholds.type(), type_thresholds.threshold()));
					break;
				}
			}
			break;
		}
		case protocol::Operation_Type_PAY_COIN:
		{
			const protocol::OperationPayCoin &pay_coin = operation.pay_coin();
			if (pay_coin.amount() < 0){
				result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
				result.set_desc(utils::String::Format("Amount should be bigger than 0"));
			}

			if (source_address == pay_coin.dest_address()) {
				result.set_code(protocol::ERRCODE_ACCOUNT_SOURCEDEST_EQUAL);
				result.set_desc(utils::String::Format("Source address(%s) equal to dest address", source_address.c_str()));
				break;
			}

			if (!phantom::PublicKey::IsAddressValid(pay_coin.dest_address())) {
				result.set_code(protocol::ERRCODE_INVALID_ADDRESS);
				result.set_desc(utils::String::Format("Dest address should be a valid account address"));
				break;
			}
			break;
		}
		case protocol::Operation_Type_LOG:
		{
			const protocol::OperationLog &log = operation.log();
			if (log.topic().size() == 0 || log.topic().size() > General::TRANSACTION_LOG_TOPIC_MAXSIZE ){
				result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
				result.set_desc(utils::String::Format("Log's parameter topic should be (0,%d]", General::TRANSACTION_LOG_TOPIC_MAXSIZE));
				break;
			}
			for (int i = 0; i < log.datas_size();i++) {
				if (log.datas(i).size() == 0 || log.datas(i).size() > General::TRANSACTION_LOG_DATA_MAXSIZE){
					result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
					result.set_desc(utils::String::Format("Log's parameter data should be (0, %d]",General::TRANSACTION_LOG_DATA_MAXSIZE));
					break;
				}
			}
			break;
		}

		case protocol::Operation_Type_Operation_Type_INT_MIN_SENTINEL_DO_NOT_USE_:
			break;
		case protocol::Operation_Type_Operation_Type_INT_MAX_SENTINEL_DO_NOT_USE_:
			break;
		default:{
			result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
			result.set_desc(utils::String::Format("Operation type(%d) invalid", type));
			break;
		}
		}

		return result;
	}

	bool OperationFrm::CheckSignature(std::shared_ptr<Environment> txenvironment) {
		std::string source_address_ = operation_.source_address();
		if (source_address_.size() == 0) {
			source_address_ = transaction_->GetSourceAddress();
		}

		if (!txenvironment->GetEntry(source_address_, source_account_)) {
			result_.set_code(protocol::ERRCODE_ACCOUNT_NOT_EXIST);
			result_.set_desc(utils::String::Format("Source account(%s) not exist", source_address_.c_str()));
			return false;
		}

		utils::StringVector vec;
		vec.push_back(source_address_);
		if (!transaction_->SignerHashPriv(source_account_, operation_.type())) {
			LOG_ERROR("Check operation's signature failed");
			result_.set_code(protocol::ERRCODE_INVALID_SIGNATURE);
			result_.set_desc(utils::String::Format("Check operation's signature failed"));
			return false;
		}

		return true;
	}


	Result OperationFrm::Apply(std::shared_ptr<Environment>  environment) {
		std::string source_address = operation_.source_address();
		if (source_address.size() == 0) {
			source_address = transaction_->GetSourceAddress();
		}
		if (!environment->GetEntry(source_address, source_account_)) {
			result_.set_code(protocol::ERRCODE_ACCOUNT_NOT_EXIST);
			result_.set_desc(utils::String::Format("Source address(%s) not exist", source_address.c_str()));
			return result_;
		}
		auto type = operation_.type();
		switch (type) {
		case protocol::Operation_Type_UNKNOWN:
			break;
		case protocol::Operation_Type_CREATE_ACCOUNT:
			CreateAccount(environment);
			break;
		case protocol::Operation_Type_PAYMENT:
			Payment(environment);
			break;
		case protocol::Operation_Type_ISSUE_ASSET:
			IssueAsset(environment);
			break;
		case protocol::Operation_Type_SET_METADATA:
			SetMetaData(environment);
			break;
		case protocol::Operation_Type_SET_SIGNER_WEIGHT:
			SetSignerWeight(environment);
			break;
		case protocol::Operation_Type_SET_THRESHOLD:
			SetThreshold(environment);
			break;
		case protocol::Operation_Type_PAY_COIN:
			PayCoin(environment);
			break;
		case protocol::Operation_Type_LOG:
			Log(environment);
			break;
		case protocol::Operation_Type_Operation_Type_INT_MIN_SENTINEL_DO_NOT_USE_:
			break;
		case protocol::Operation_Type_Operation_Type_INT_MAX_SENTINEL_DO_NOT_USE_:
			break;
		default:
			result_.set_code(protocol::ERRCODE_INVALID_PARAMETER);
			result_.set_desc(utils::String::Format("type(%d) not support", type));
			break;
		}
		return result_;
	}

	void OperationFrm::CreateAccount(std::shared_ptr<Environment> environment) {
		//auto &environment = LedgerManager::Instance().execute_environment_;
		const protocol::OperationCreateAccount& createaccount = operation_.create_account();
		do {
			std::shared_ptr<AccountFrm> dest_account;

			if (environment->GetEntry(createaccount.dest_address(), dest_account)) {
				result_.set_code(protocol::ERRCODE_ACCOUNT_DEST_EXIST);
				result_.set_desc(utils::String::Format("Dest address(%s) already exist", createaccount.dest_address().c_str()));
				break;
			}

			int64_t base_reserve = LedgerManager::Instance().GetCurFeeConfig().base_reserve();
			if (createaccount.init_balance() < base_reserve) {
				result_.set_code(protocol::ERRCODE_ACCOUNT_INIT_LOW_RESERVE);
				std::string error_desc = utils::String::Format("Dest address init balance (" FMT_I64 ") not enough for base_reserve (" FMT_I64 ")", createaccount.init_balance(), base_reserve);
				result_.set_desc(error_desc);
				LOG_ERROR("%s", error_desc.c_str());
				break;
			}
			if (source_account_->GetAccountBalance() - base_reserve < createaccount.init_balance()) {
				result_.set_code(protocol::ERRCODE_ACCOUNT_LOW_RESERVE);
				std::string error_desc = utils::String::Format("Source account(%s) balance(" FMT_I64 ") - base_reserve(" FMT_I64 ") not enough for init balance(" FMT_I64 ")", 
				source_account_->GetAccountAddress().c_str(),source_account_->GetAccountBalance(), base_reserve, createaccount.init_balance());
				result_.set_desc(error_desc);
				LOG_ERROR("%s", error_desc.c_str());
				break;
			}
			source_account_->AddBalance(-1 * createaccount.init_balance());

			protocol::Account account;
			account.set_balance(createaccount.init_balance());
			account.mutable_priv()->CopyFrom(createaccount.priv());
			account.set_address(createaccount.dest_address());
			account.mutable_contract()->CopyFrom(createaccount.contract());
			dest_account = std::make_shared<AccountFrm>(account);

			bool success = true;
			for (int i = 0; i < createaccount.metadatas_size(); i++) {
				protocol::KeyPair kp;
				kp.CopyFrom(createaccount.metadatas(i));
				if (kp.version() != 0 && kp.version() != 1){
					success = false;
					break;
				}
				kp.set_version(1);
				dest_account->SetMetaData(kp);
			}
			if (!success){
				result_.set_code(protocol::ERRCODE_INVALID_DATAVERSION);
				result_.set_desc(utils::String::Format(
					"set meatadata while create account(%s) version should be 0 or 1 ",
					dest_account->GetAccountAddress().c_str()));
				
				break;
			}

			environment->AddEntry(dest_account->GetAccountAddress(), dest_account);

			std::string javascript = dest_account->GetProtoAccount().contract().payload();
			if (!javascript.empty()) {

				ContractParameter parameter;
				parameter.code_ = javascript;
				parameter.input_ = createaccount.init_input();
				parameter.this_address_ = createaccount.dest_address();
				parameter.sender_ = source_account_->GetAccountAddress();
				parameter.ope_index_ = index_;
				parameter.timestamp_ = transaction_->ledger_->value_->close_time();
				parameter.blocknumber_ = transaction_->ledger_->value_->ledger_seq();
				parameter.ledger_context_ = transaction_->ledger_->lpledger_context_;
				parameter.pay_coin_amount_ = 0;

				std::string err_msg;
				result_ = ContractManager::Instance().Execute(Contract::TYPE_V8, parameter, true);
			}

		} while (false);
	}

	void OperationFrm::IssueAsset(std::shared_ptr<Environment> environment) {


		const protocol::OperationIssueAsset& ope = operation_.issue_asset();
		do {

			protocol::AssetStore asset_e;
			protocol::AssetKey key;
			key.set_issuer(source_account_->GetAccountAddress());
			key.set_code(ope.code());
			if (!source_account_->GetAsset(key, asset_e)) {
				protocol::AssetStore asset;
				asset.mutable_key()->CopyFrom(key);
				asset.set_amount(ope.amount());
				source_account_->SetAsset(asset);
			}
			else {
				int64_t amount = asset_e.amount() + ope.amount();
				if (amount < asset_e.amount() || amount < ope.amount())
				{
					result_.set_code(protocol::ERRCODE_ACCOUNT_ASSET_AMOUNT_TOO_LARGE);
					result_.set_desc(utils::String::Format("IssueAsset asset(%s:%s:%d) overflow(" FMT_I64 " " FMT_I64 ")", key.issuer().c_str(), key.code().c_str(), key.type(), asset_e.amount(), ope.amount()));
					break;
				}
				asset_e.set_amount(amount);
				source_account_->SetAsset(asset_e);
			}

		} while (false);
	}

	void OperationFrm::Payment(std::shared_ptr<Environment> environment) {
		const protocol::OperationPayment& payment = operation_.payment();
		do {
			std::shared_ptr<AccountFrm> dest_account = nullptr;

			if (!environment->GetEntry(payment.dest_address(), dest_account)) {
				result_.set_code(protocol::ERRCODE_ACCOUNT_NOT_EXIST);
				result_.set_desc(utils::String::Format("Dest account(%s) not exist", payment.dest_address().c_str()));
				break;
			}

			if (payment.has_asset()){
				protocol::AssetStore asset_e;
				protocol::AssetKey key = payment.asset().key();
				if (!source_account_->GetAsset(key, asset_e)) {
					result_.set_code(protocol::ERRCODE_ACCOUNT_ASSET_LOW_RESERVE);
					result_.set_desc(utils::String::Format("asset(%s:%s:%d) low reserve", key.issuer().c_str(), key.code().c_str(), key.type()));
					break;
				}

				if (payment.asset().key().type() == 0){

					int64_t sender_amount = asset_e.amount() - payment.asset().amount();
					if (sender_amount < 0) {
						result_.set_code(protocol::ERRCODE_ACCOUNT_ASSET_LOW_RESERVE);
						result_.set_desc(utils::String::Format("asset(%s:%s:%d) low reserve", key.issuer().c_str(), key.code().c_str(), key.type()));
						break;
					}
					asset_e.set_amount(sender_amount);
					source_account_->SetAsset(asset_e);

					protocol::AssetStore dest_asset;
					if (!dest_account->GetAsset(key, dest_asset)) {
						dest_asset.mutable_key()->CopyFrom(key);
						dest_asset.set_amount(payment.asset().amount());
						dest_account->SetAsset(dest_asset);
					}
					else {
						int64_t receiver_amount = dest_asset.amount() + payment.asset().amount();
						if (receiver_amount < dest_asset.amount() || receiver_amount < payment.asset().amount())
						{
							result_.set_code(protocol::ERRCODE_ACCOUNT_ASSET_AMOUNT_TOO_LARGE);
							result_.set_desc(utils::String::Format("Payment asset(%s:%s:%d) overflow(" FMT_I64 " " FMT_I64 ")", key.issuer().c_str(), key.code().c_str(), key.type(), dest_asset.amount(), payment.asset().amount()));
							break;
						}
						dest_asset.set_amount(receiver_amount);
						dest_account->SetAsset(dest_asset);
					}
				}
				else{
					result_.set_code(protocol::ERRCODE_ASSET_INVALID);
					result_.set_desc(utils::String::Format("payment asset type must be 0"));
					break;
				}
			}
			
			std::string javascript = dest_account->GetProtoAccount().contract().payload();
			if (!javascript.empty()){
				ContractParameter parameter;
				parameter.code_ = javascript;
				parameter.input_ = payment.input();
				parameter.this_address_ = payment.dest_address();
				parameter.sender_ = source_account_->GetAccountAddress();
				parameter.ope_index_ = index_;
				parameter.timestamp_ = transaction_->ledger_->value_->close_time();
				parameter.blocknumber_ = transaction_->ledger_->value_->ledger_seq();
				parameter.ledger_context_ = transaction_->ledger_->lpledger_context_;
				parameter.pay_asset_amount_ = payment.asset();

				result_ = ContractManager::Instance().Execute(Contract::TYPE_V8, parameter);
			}
		} while (false);
	}

	void OperationFrm::SetMetaData(std::shared_ptr<Environment> environment) {

		do {
			auto ope = operation_.set_metadata();
			std::string key = ope.key();
			protocol::KeyPair keypair_e ;
			int64_t version = ope.version();
			bool delete_flag = ope.delete_flag();
			if (delete_flag){
				if (source_account_->GetMetaData(key, keypair_e)){
					if (version != 0) {
						if (keypair_e.version() != version) {
							result_.set_code(protocol::ERRCODE_INVALID_DATAVERSION);
							result_.set_desc(utils::String::Format("Data version(" FMT_I64 ") not valid", version));
							break;
						}
					}
					if (key.empty()){
						result_.set_code(protocol::ERRCODE_INVALID_PARAMETER);
						result_.set_desc(utils::String::Format("Data key is empty,key(%s)", key.c_str()));
						break;
					}
					source_account_->DeleteMetaData(keypair_e);
				}
				else{
					result_.set_code(protocol::ERRCODE_NOT_EXIST);
					result_.set_desc(utils::String::Format("DeleteMetaData not exist key(%s)", key.c_str()));
					break;
				}
			}
			else{
				if (source_account_->GetMetaData(key, keypair_e)) {

					if (version != 0) {
						if (keypair_e.version() + 1 != version) {
							result_.set_code(protocol::ERRCODE_INVALID_DATAVERSION);
							result_.set_desc(utils::String::Format("Data version(" FMT_I64 ") not valid", version));
							break;
						}
					}

					keypair_e.set_version(keypair_e.version() + 1);
					keypair_e.set_value(ope.value());
					source_account_->SetMetaData(keypair_e);

				}
				else {
					if (version != 1 && version != 0) {
						result_.set_code(protocol::ERRCODE_INVALID_DATAVERSION);
						result_.set_desc(utils::String::Format("Data version(" FMT_I64 ") not valid", version));
						break;
					}
					protocol::KeyPair keypair;
					keypair.set_value(ope.value());
					keypair.set_key(ope.key());
					keypair.set_version(1);
					source_account_->SetMetaData(keypair);
				}
			}			
		} while (false);

	}

	void OperationFrm::SetSignerWeight(std::shared_ptr<Environment> environment) {
		const protocol::OperationSetSignerWeight &ope = operation_.set_signer_weight();
		do {

			if (ope.master_weight() >= 0) {
				source_account_->SetProtoMasterWeight(ope.master_weight());
			}

			for (int32_t i = 0; i < ope.signers_size(); i++) {

				int64_t weight = ope.signers(i).weight();
				source_account_->UpdateSigner(ope.signers(i).address(), weight);
			}

		} while (false);
	}

	void OperationFrm::SetThreshold(std::shared_ptr<Environment> environment) {
		const protocol::OperationSetThreshold &ope = operation_.set_threshold();
		std::shared_ptr<AccountFrm> source_account = nullptr;

		do {
			if (ope.tx_threshold() >= 0) {
				source_account_->SetProtoTxThreshold(ope.tx_threshold());
			}

			for (int32_t i = 0; i < ope.type_thresholds_size(); i++) {
				source_account_->UpdateTypeThreshold(ope.type_thresholds(i).type(),
					ope.type_thresholds(i).threshold());
			}
		} while (false);
	}

	void OperationFrm::PayCoin(std::shared_ptr<Environment> environment) {
		auto ope = operation_.pay_coin();
		std::string address = ope.dest_address();
		std::shared_ptr<AccountFrm> dest_account_ptr = nullptr;
		int64_t reserve_coin = LedgerManager::Instance().GetCurFeeConfig().base_reserve();
		do {
			protocol::Account& proto_source_account = source_account_->GetProtoAccount();
			if (proto_source_account.balance() < ope.amount() + reserve_coin) {
				result_.set_code(protocol::ERRCODE_ACCOUNT_LOW_RESERVE);
				result_.set_desc(utils::String::Format("Account(%s) balance(" FMT_I64 ") - base_reserve(" FMT_I64 ") not enough for pay (" FMT_I64 ") ",
					proto_source_account.address().c_str(),
					proto_source_account.balance(),
					reserve_coin,
					ope.amount()					
					));
				break;
			}

			if (!environment->GetEntry(address, dest_account_ptr)) {
				if (ope.amount() < reserve_coin) {
					result_.set_code(protocol::ERRCODE_ACCOUNT_INIT_LOW_RESERVE);
					result_.set_desc(utils::String::Format("Account(%s) init balance(" FMT_I64 ") not enough for reserve(" FMT_I64 ")",
						address.c_str(),
						ope.amount(),
						reserve_coin
						));
					break;
				}

				protocol::Account account;
				account.set_balance(0);
				account.mutable_priv()->set_master_weight(1);
				account.mutable_priv()->mutable_thresholds()->set_tx_threshold(1);
				account.set_address(ope.dest_address());
				dest_account_ptr = std::make_shared<AccountFrm>(account);
				environment->AddEntry(ope.dest_address(), dest_account_ptr);

				// add create_account fee while dest_address is not exists
				ope_fee_ += FeeCompulate::OperationFee(transaction_->GetGasPrice(), protocol::Operation_Type::Operation_Type_CREATE_ACCOUNT);
			}
			protocol::Account& proto_dest_account = dest_account_ptr->GetProtoAccount();

			int64_t new_balance = proto_source_account.balance() - ope.amount();
			proto_source_account.set_balance(new_balance);
			proto_dest_account.set_balance(proto_dest_account.balance() + ope.amount());

			std::string javascript = dest_account_ptr->GetProtoAccount().contract().payload();
			if (!javascript.empty()) {

				ContractParameter parameter;
				parameter.code_ = javascript;
				parameter.input_ = ope.input();
				parameter.this_address_ = ope.dest_address();
				parameter.sender_ = source_account_->GetAccountAddress();
				parameter.ope_index_ = index_;
				parameter.timestamp_ = transaction_->ledger_->value_->close_time();
				parameter.blocknumber_ = transaction_->ledger_->value_->ledger_seq();
				parameter.ledger_context_ = transaction_->ledger_->lpledger_context_;
				parameter.pay_coin_amount_ = ope.amount();

				std::string err_msg;
				result_ = ContractManager::Instance().Execute(Contract::TYPE_V8, parameter);

			}
		} while (false);
	}

	void OperationFrm::Log(std::shared_ptr<Environment> environment) {}
}


