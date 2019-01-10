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

#include <common/storage.h>
#include "ledger_manager.h"
#include "environment.h"

namespace phantom{

	Environment::Environment(mapKV* data, settingKV* settings) :
		AtomMap<std::string, AccountFrm>(data), settings_(settings)
	{
		useAtomMap_ = Configure::Instance().ledger_configure_.use_atom_map_;
		parent_ = nullptr;
	}

	Environment::Environment(Environment* parent){

		useAtomMap_ = Configure::Instance().ledger_configure_.use_atom_map_;
		if (useAtomMap_)
		{
			parent_ = nullptr;
			return;
		}

		parent_ = parent;
		if (parent_){
			for (auto it = parent_->entries_.begin(); it != parent_->entries_.end(); it++){
				entries_[it->first] = std::make_shared<AccountFrm>(it->second);
			}
			settings_ = parent->settings_;
		}
	}

	bool Environment::GetEntry(const std::string &key, AccountFrm::pointer &frm){
		if (useAtomMap_)
			return Get(key, frm);

		if (entries_.find(key) == entries_.end()){
			if (AccountFromDB(key, frm)){
				entries_[key] = frm;
				return true;
			}
			else{
				return false;
			}
		}
		else{
			frm = entries_[key];
			return true;
		}
	}

	bool Environment::Commit(){
		if (useAtomMap_){
			return settings_.Commit() && AtomMap<std::string, AccountFrm>::Commit();
		}

		parent_->entries_ = entries_;
		parent_->settings_ = settings_;
		return true;
	}

	void Environment::ClearChangeBuf()
	{
		if (useAtomMap_){
			settings_.ClearChangeBuf();
			AtomMap<std::string, AccountFrm>::ClearChangeBuf();
		}
	}

	bool Environment::AddEntry(const std::string& key, AccountFrm::pointer frm){
		if (useAtomMap_ == true)
			return Set(key, frm);

		entries_[key] = frm;
		return true;
	}

	bool Environment::GetFromDB(const std::string &address, AccountFrm::pointer &account_ptr)
	{
		return AccountFromDB(address, account_ptr);
	}

	bool Environment::AccountFromDB(const std::string &address, AccountFrm::pointer &account_ptr){

		auto db = Storage::Instance().account_db();
		std::string index = DecodeAddress(address);
		std::string buff;
		if (!LedgerManager::Instance().tree_->Get(index, buff)){
			return false;
		}

		protocol::Account account;
		if (!account.ParseFromString(buff)){
			PROCESS_EXIT("fatal error, account(%s) ParseFromString failed", address.c_str());
		}
		account_ptr = std::make_shared<AccountFrm>(account);
		return true;

	}

	std::shared_ptr<Environment> Environment::NewStackFrameEnv()
	{
		mapKV& data	= GetActionBuf();
		settingKV& settings = settings_.GetActionBuf();
		std::shared_ptr<Environment> next = std::make_shared<Environment>(&data, &settings);

		return next;
	}

	bool Environment::UpdateFeeConfig(const Json::Value &feeConfig) {
		std::shared_ptr<Json::Value> fees;
		settings_.Get(feesKey, fees);

		if (!fees){
			fees = std::make_shared<Json::Value>(feeConfig);
			settings_.Set(feesKey, fees);
		}
		else{
			for (auto it = feeConfig.begin(); it != feeConfig.end(); it++) {
				(*fees)[it.memberName()] = feeConfig[it.memberName()];
			}
		}

		return true;
	}

	bool Environment::GetVotedFee(const protocol::FeeConfig &old_fee, protocol::FeeConfig& new_fee) {
		bool change = false;
		new_fee = old_fee;

		std::shared_ptr<Json::Value> fees;
		settings_.Get(feesKey, fees);
		if (!fees) return false;

		for (auto it = fees->begin(); it != fees->end(); it++) {
			int32_t fee_type = (protocol::FeeConfig_Type)utils::String::Stoi(it.memberName());
			int64_t price = (*fees)[it.memberName()].asInt64();

			switch ((protocol::FeeConfig_Type)fee_type) {
			case protocol::FeeConfig_Type_UNKNOWN:
				LOG_ERROR("FeeConfig type error");
				break;
			case protocol::FeeConfig_Type_GAS_PRICE:
				if (new_fee.gas_price() != price) {
					new_fee.set_gas_price(price);
					change = true;
				}
				break;
			case protocol::FeeConfig_Type_BASE_RESERVE:
				if (new_fee.base_reserve() != price) {
					new_fee.set_base_reserve(price);
					change = true;
				}
				break;
			default:
				LOG_ERROR("Fee config type(%d) error", fee_type);
				break;
			}

		}

		return change;
	}

	Json::Value& Environment::GetValidators(){
		std::shared_ptr<Json::Value> validators;
		settings_.Get(validatorsKey, validators);

		if (!validators){
			validators = std::make_shared<Json::Value>();
			auto sets = LedgerManager::Instance().Validators();

			for (int i = 0; i < sets.validators_size(); i++){
				auto validator = sets.mutable_validators(i);
				Json::Value value;
				value.append(validator->address());
				value.append(utils::String::ToString(validator->pledge_coin_amount()));
				validators->append(value);
			}
			settings_.Set(validatorsKey, validators);
		}

		return *validators;
	}

	bool Environment::UpdateNewValidators(const Json::Value& validators) {
		return settings_.Set(validatorsKey, std::make_shared<Json::Value>(validators));
	}

	bool Environment::GetVotedValidators(const protocol::ValidatorSet &old_validator, protocol::ValidatorSet& new_validator){
		std::shared_ptr<Json::Value> validators;
		bool ret = settings_.Get(validatorsKey, validators);
		if (!validators){
			new_validator = old_validator;
			return false;
		}

		for (Json::Value::UInt i = 0; i < validators->size(); i++){
			std::string address = (*validators)[i][(Json::Value::UInt)0].asString();
			int64_t pledge_amount = utils::String::Stoi64( (*validators)[i][1].asString() );

			auto validator = new_validator.add_validators();
			validator->set_address(address);
			validator->set_pledge_coin_amount(pledge_amount);
		}

		return true;
	}
}