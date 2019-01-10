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

#include <utils/headers.h>
#include <common/private_key.h>
#include <common/storage.h>
#include <main/configure.h>
#include <ledger/ledger_manager.h>
#include <ledger/contract_manager.h>
#include <consensus/consensus_manager.h>
#include <glue/glue_manager.h>
#include "web_server.h"
#include <ledger/kv_trie.h>

namespace phantom {
	void WebServer::GetAccountBase(const http::server::request &request, std::string &reply) {
		std::string address = request.GetParamValue("address");

		int32_t error_code = protocol::ERRCODE_SUCCESS;
		AccountFrm::pointer acc = NULL;
		int64_t balance = 0;
		Json::Value reply_json = Json::Value(Json::objectValue);
		Json::Value record = Json::Value(Json::arrayValue);
		Json::Value &result = reply_json["result"];

		if (!Environment::AccountFromDB(address, acc)) {
			error_code = protocol::ERRCODE_NOT_EXIST;
			LOG_TRACE("GetAccount fail, account(%s) not exist", address.c_str());
		}
		else {
			acc->ToJson(result);
		}

		reply_json["error_code"] = error_code;
		reply = reply_json.toStyledString();
	}

	void WebServer::GetAccount(const http::server::request &request, std::string &reply) {
		std::string address = request.GetParamValue("address");
		std::string storagekey = request.GetParamValue("key");

		std::string issuer = request.GetParamValue("issuer");
		std::string code = request.GetParamValue("code");
		std::string asset_type_str = request.GetParamValue("type");
		int32_t asset_type = 0;
		if (!asset_type_str.empty()){
			char* p;
			asset_type = strtol(asset_type_str.c_str(), &p, 10);
			if (*p) asset_type = 0;
		}

		int32_t error_code = protocol::ERRCODE_SUCCESS;
		AccountFrm::pointer acc = NULL;
		int64_t balance = 0;
		Json::Value reply_json = Json::Value(Json::objectValue);
		Json::Value record = Json::Value(Json::arrayValue);
		Json::Value &result = reply_json["result"];

		if (!Environment::AccountFromDB(address, acc)) {
			error_code = protocol::ERRCODE_NOT_EXIST;
			LOG_TRACE("GetAccount fail, account(%s) not exist", address.c_str());
		}
		else {
			acc->ToJson(result);
			Json::Value& metadatas = result["metadatas"];
			if (!storagekey.empty()) {
				protocol::KeyPair value_ptr;
				if (acc->GetMetaData(storagekey, value_ptr)) {
					metadatas[(Json::UInt)0] = phantom::Proto2Json(value_ptr);
				}
			}
			else {
				std::vector<protocol::KeyPair> metadata;
				acc->GetAllMetaData(metadata);

				for (size_t i = 0; i < metadata.size(); i++) {
					metadatas[i] = Proto2Json(metadata[i]);
				}
			}

			Json::Value& jsonassets = result["assets"];
			if (!issuer.empty() && !code.empty()) {
				protocol::AssetKey p;
				p.set_issuer(issuer);
				p.set_code(code);
				p.set_type(asset_type);
				protocol::AssetStore asset;
				if (acc->GetAsset(p, asset)) {
					jsonassets[(Json::UInt)0] = Proto2Json(asset);
				}
			}
			else {
				std::vector<protocol::AssetStore> assets;
				acc->GetAllAssets(assets);
				for (size_t i = 0; i < assets.size(); i++) {
					jsonassets[i] = Proto2Json(assets[i]);
				}
			}
		}

		reply_json["error_code"] = error_code;
		reply = reply_json.toStyledString();
	}


	void WebServer::GetGenesisAccount(const http::server::request &request, std::string &reply) {
		std::string address;
		Storage::Instance().account_db()->Get(phantom::General::KEY_GENE_ACCOUNT, address);
		http::server::request req;
		req.parameter.insert({ std::string("address"), address });
		GetAccount(req, reply);
	}

	void WebServer::GetAccountMetaData(const http::server::request &request, std::string &reply) {
		std::string address = request.GetParamValue("address");
		std::string metadata_key = request.GetParamValue("key");
		int32_t error_code = protocol::ERRCODE_SUCCESS;
		AccountFrm::pointer acc = NULL;
		int64_t balance = 0;
		Json::Value reply_json = Json::Value(Json::objectValue);
		Json::Value record = Json::Value(Json::arrayValue);
		Json::Value &result = reply_json["result"];

		if (!Environment::AccountFromDB(address, acc)) {
			error_code = protocol::ERRCODE_NOT_EXIST;
			LOG_TRACE("account(%s) not exist", address.c_str());
		}
		else {
			if (!metadata_key.empty()) {
				protocol::KeyPair value_ptr;
				if (acc->GetMetaData(metadata_key, value_ptr)) {
					result[metadata_key] = phantom::Proto2Json(value_ptr);
				}
			}
			else {
				std::vector<protocol::KeyPair> metadata;
				acc->GetAllMetaData(metadata);
				for (size_t i = 0; i < metadata.size(); i++) {
					result[metadata[i].key()] = Proto2Json(metadata[i]);
				}
			}
		}

		reply_json["error_code"] = error_code;
		reply = reply_json.toStyledString();
	}

	void WebServer::Debug(const http::server::request &request, std::string &reply) {
		std::string key = request.GetParamValue("key");
		auto location = utils::String::HexStringToBin(key);
		std::vector<std::string> values;
		auto x = LedgerManager::Instance().tree_->GetNode(location);
		Json::Value ret;
		ret["ret"] = phantom::Proto2Json(x);
		ret["NEW"] = NodeFrm::NEWCOUNT;
		ret["DEL"] = NodeFrm::DELCOUNT;

		reply = ret.toStyledString();
	}
	void WebServer::GetAccountAssets(const http::server::request &request, std::string &reply) {
		std::string address = request.GetParamValue("address");

		std::string issuer = request.GetParamValue("issuer");
		std::string code = request.GetParamValue("code");
		std::string asset_type_str = request.GetParamValue("type");
		int32_t asset_type = 0;
		if (!asset_type_str.empty()){
			char* p;
			asset_type = strtol(asset_type_str.c_str(), &p, 10);
			if (*p) asset_type = 0;
		}

		int32_t error_code = protocol::ERRCODE_SUCCESS;
		AccountFrm::pointer acc = NULL;
		int64_t balance = 0;
		Json::Value reply_json = Json::Value(Json::objectValue);
		Json::Value record = Json::Value(Json::arrayValue);
		Json::Value &result = reply_json["result"];

		if (!Environment::AccountFromDB(address, acc)) {
			error_code = protocol::ERRCODE_NOT_EXIST;
			LOG_TRACE("GetAccount fail, account(%s) not exist", address.c_str());
		}
		else {

			if (!issuer.empty() && !code.empty() ) {
				protocol::AssetKey p;
				p.set_issuer(issuer);
				p.set_code(code);
				p.set_type(asset_type);
				protocol::AssetStore asset;
				if (acc->GetAsset(p, asset)) {
					result["asset"] = Proto2Json(asset);
				}
			}
			else {
				std::vector<protocol::AssetStore> assets;
				acc->GetAllAssets(assets);
				for (size_t i = 0; i < assets.size(); i++) {
					result[i] = Proto2Json(assets[i]);
				}
			}
		}

		reply_json["error_code"] = error_code;
		reply = reply_json.toStyledString();
	}

	void WebServer::GetTransactionBlob(const http::server::request &request, std::string &reply) {
		Result result;
		Json::Value reply_json = Json::Value(Json::objectValue);
		Json::Value &js_result = reply_json["result"];
		do {
			Json::Value body;
			if (!body.fromString(request.body)) {
				result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
				result.set_desc("");
				break;
			}

			protocol::Transaction tran;
			std::string error_msg;
			if (!Json2Proto(body, tran, error_msg)) {
				result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
				result.set_desc(error_msg);
				break;
			}

			std::string SerializeString;
			if (!tran.SerializeToString(&SerializeString)) {
				result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
				result.set_desc("");
				LOG_INFO("SerializeToString Transaction Failed");
				break;
			}

			std::string crypto = utils::encode_b16(SerializeString);
			js_result["transaction_blob"] = crypto;
			js_result["hash"] = utils::encode_b16(HashWrapper::Crypto(SerializeString));
		} while (false);

		reply_json["error_code"] = result.code();
		reply_json["error_desc"] = result.desc();
		reply = reply_json.toStyledString();
	}

	void WebServer::GetTransactionFromBlob(const http::server::request &request, std::string &reply) {
		Result result_e;
		result_e.set_code(protocol::ERRCODE_SUCCESS);
		Json::Value reply_json = Json::Value(Json::objectValue);
		Json::Value &result = reply_json["result"];

		std::string blob = request.GetParamValue("blob");
		std::string env = request.GetParamValue("env");
		do {
			std::string decodeblob;
			if (!utils::String::HexStringToBin(blob, decodeblob)) {
				result_e.set_code(protocol::ERRCODE_INVALID_PARAMETER);
				result_e.set_desc("'transaction_blob' value must be Hex");
				break;
			}

			protocol::TransactionEnv tran_env;
			if (env == "true") {
				if (!tran_env.ParseFromString(decodeblob)) {
					result_e.set_code(protocol::ERRCODE_INVALID_PARAMETER);
					result_e.set_desc("Parse From env String from decodeblob invalid");
					LOG_ERROR("ParseFromString from decodeblob invalid");
					break;
				}
			}
			else {
				protocol::Transaction *tran = tran_env.mutable_transaction();
				if (!tran->ParseFromString(decodeblob)) {
					result_e.set_code(protocol::ERRCODE_INVALID_PARAMETER);
					result_e.set_desc("Parse From String from decodeblob invalid");
					LOG_ERROR("ParseFromString from decodeblob invalid");
					break;
				}
			}

			TransactionFrm frm(tran_env);
			frm.ToJson(reply_json);

		} while (false);

		reply_json["error_code"] = result_e.code();
		reply_json["error_desc"] = result_e.desc();
		reply = reply_json.toStyledString();
	}

	void WebServer::GetTransactionHistory(const http::server::request &request, std::string &reply) {
		WebServerConfigure &web_config = Configure::Instance().webserver_configure_;
		phantom::KeyValueDb *db = phantom::Storage::Instance().ledger_db();

		std::string seq = request.GetParamValue("ledger_seq");
		std::string hash = request.GetParamValue("hash");
		std::string start = request.GetParamValue("start");
		std::string limit = request.GetParamValue("limit");

		int32_t start_int = utils::String::Stoi(start);
		int32_t limit_int = utils::String::Stoi(limit);

		if (start_int < 0) start_int = 0;
		if (limit_int <= 0) limit_int = 1000;

		int32_t error_code = protocol::ERRCODE_SUCCESS;
		Json::Value reply_json = Json::Value(Json::objectValue);

		Json::Value &result = reply_json["result"];
		Json::Value &txs = result["transactions"];
		txs = Json::Value(Json::arrayValue);
		result["total_count"] = 0;
		do {

			protocol::EntryList list;
			//avoid scan the whole table
			protocol::LedgerHeader header = LedgerManager::Instance().GetLastClosedLedger();
			if (!seq.empty()) {
				std::string hashlist;
				if (db->Get(ComposePrefix(General::LEDGER_TRANSACTION_PREFIX, seq), hashlist) <= 0) {
					error_code = protocol::ERRCODE_NOT_EXIST;
					break;
				}

				list.ParseFromString(hashlist);
				if (list.entry_size() == 0) {
					error_code = protocol::ERRCODE_NOT_EXIST;
					break;
				}

				result["total_count"] = list.entry_size();
			}
			else if (!hash.empty()) {
				result["total_count"] = 1;
				list.add_entry(utils::String::HexStringToBin(hash));
			}
			else {
				std::string hashlist;
				if (db->Get(General::LAST_TX_HASHS, hashlist) <= 0) {
					error_code = protocol::ERRCODE_NOT_EXIST;
					break;
				}

				list.ParseFromString(hashlist);
				if (list.entry_size() == 0) {
					error_code = protocol::ERRCODE_NOT_EXIST;
					break;
				}

				result["total_count"] = list.entry_size();
			}

			for (int32_t i = start_int;
				i < list.entry_size() &&
				error_code == protocol::ERRCODE_SUCCESS &&
				i < start_int + limit_int;
			i++) {
				TransactionFrm txfrm;
				if (txfrm.LoadFromDb(list.entry(i)) > 0) {
					result["total_count"] = 0;
					error_code = protocol::ERRCODE_NOT_EXIST;
					break;
				}
				Json::Value m;
				txfrm.ToJson(m);
				txs[txs.size()] = m;
			}
		} while (false);

		reply_json["error_code"] = error_code;
		if (error_code == protocol::ERRCODE_NOT_EXIST){
			reply_json["error_desc"] = "query result not exist";
		}
		reply = reply_json.toFastString();
	}

	void WebServer::GetTransactionCache(const http::server::request &request, std::string &reply) {
		WebServerConfigure &web_config = Configure::Instance().webserver_configure_;
		
		std::string hash = request.GetParamValue("hash");
		std::string limit_str = request.GetParamValue("limit");

		int32_t error_code = protocol::ERRCODE_SUCCESS;
		Json::Value reply_json = Json::Value(Json::objectValue);

		Json::Value &result = reply_json["result"];
		Json::Value &txs = result["transactions"];
		txs = Json::Value(Json::arrayValue);
		result["total_count"] = 0;

		do 
		{
			std::vector<TransactionFrm::pointer> txs_arr;

			if (!hash.empty()){
				TransactionFrm::pointer tx;
				if (GlueManager::Instance().QueryTransactionCache(utils::String::HexStringToBin(hash), tx)) {
					result["total_count"] = 1;
					txs_arr.emplace_back(tx);
				}
				else{
					error_code = protocol::ERRCODE_NOT_EXIST;
				}
			}
			else{
				uint32_t limit = web_config.query_limit_;
				if (!limit_str.empty()){
					uint32_t limit_int = utils::String::Stoui(limit_str);
					if (limit_int == 0) limit_int = 1000;
					limit = MIN(limit_int, web_config.query_limit_);
				}

				txs_arr.reserve(limit);
				GlueManager::Instance().QueryTransactionCache(limit, txs_arr);
				result["total_count"] = (Json::UInt64)txs_arr.size();
				if (txs_arr.size() == 0) {
					error_code = protocol::ERRCODE_NOT_EXIST;
				}
			}

			for (auto t : txs_arr){
				Json::Value m;
				t->CacheTxToJson(m);
				txs[txs.size()] = m;
			}

		} while (false);

		reply_json["error_code"] = error_code;
		if (error_code == protocol::ERRCODE_NOT_EXIST){
			reply_json["error_desc"] = "query result not exist";
		}
		reply = reply_json.toFastString();

	}

	void WebServer::GetContractTx(const http::server::request &request, std::string &reply) {
		WebServerConfigure &web_config = Configure::Instance().webserver_configure_;

		std::string hash = request.GetParamValue("hash");
		std::string contractor = request.GetParamValue("contractor");
		std::string trigger = request.GetParamValue("trigger");

		std::string str_order = request.GetParamValue("order");
		std::string start_str = request.GetParamValue("start");
		std::string limit_str = request.GetParamValue("limit");

		if (str_order == "DESC" ||
			str_order == "desc" ||
			str_order == "asc" ||
			str_order == "ASC") {
		}
		else {
			str_order = "DESC";
		}

		int32_t error_code = protocol::ERRCODE_SUCCESS;
		Json::Value reply_json = Json::Value(Json::objectValue);

		Json::Value &result = reply_json["result"];
		Json::Value &txs = result["transactions"];
		txs = Json::Value(Json::arrayValue);
		do {
			if (start_str.empty()) start_str = "0";
			if (!utils::String::IsNumber(start_str) == 1) {
				error_code = protocol::ERRCODE_INVALID_PARAMETER;
				break;
			}
			uint32_t start = utils::String::Stoui(start_str);


			if (limit_str.empty()) limit_str = "20";
			if (!utils::String::IsNumber(limit_str) == 1) {
				error_code = protocol::ERRCODE_INVALID_PARAMETER;
				break;
			}
			uint32_t limit = utils::String::Stoui(limit_str);
			limit = MIN(limit, web_config.query_limit_);


			std::string condition = "WHERE 1=1 ";
			if (!hash.empty())
				condition += utils::String::Format("AND hash='%s' ", hash.c_str());

			if (!contractor.empty())
				condition += utils::String::Format("AND contractor='%s' ", contractor.c_str());

			if (!trigger.empty())
				condition += utils::String::Format("AND trigger_transaction='%s' ", trigger.c_str());
			Json::Value record;

			for (size_t i = 0; i < record.size() && error_code == protocol::ERRCODE_SUCCESS; i++) {

				Json::Value &item = record[i];
				protocol::Transaction prototx;
				prototx.ParseFromString(utils::String::HexStringToBin(item["body"].asString()));
				Json::Value m = item;
				m["body"] = Proto2Json(prototx);

				txs[(Json::UInt) i] = m;
			}
		} while (false);
		reply_json["error_code"] = error_code;
		reply = reply_json.toStyledString();
	}


	void WebServer::GetStatus(const http::server::request &request, std::string &reply) {
		uint32_t error_code = protocol::ERRCODE_SUCCESS;
		Json::Value reply_json = Json::Value(Json::objectValue);
		Json::Value &result = reply_json["result"];

		const protocol::LedgerHeader &ledger = LedgerManager::Instance().GetLastClosedLedger();
		result["transaction_count"] = ledger.tx_count();
		result["account_count"] = LedgerManager::Instance().GetAccountNum();

		reply_json["error_code"] = error_code;
		reply = reply_json.toStyledString();
	}


	void WebServer::GetModulesStatus(const http::server::request &request, std::string &reply) {
		utils::ReadLockGuard guard(phantom::StatusModule::status_lock_);
		Json::Value reply_json = *phantom::StatusModule::modules_status_;

		reply_json["keyvalue_db"] = Json::Value(Json::objectValue);
		phantom::Storage::Instance().keyvalue_db()->GetOptions(reply_json["keyvalue_db"]);
		reply_json["ledger_db"] = Json::Value(Json::objectValue);
		phantom::Storage::Instance().ledger_db()->GetOptions(reply_json["ledger_db"]);
		reply_json["account_db"] = Json::Value(Json::objectValue);
		phantom::Storage::Instance().account_db()->GetOptions(reply_json["account_db"]);

		reply = reply_json.toStyledString();
	}

	void WebServer::GetLedgerValidators(const http::server::request &request, std::string &reply) {
		int32_t error_code = protocol::ERRCODE_SUCCESS;
		Json::Value reply_json = Json::Value(Json::objectValue);
		Json::Value &result = reply_json["result"];

		std::string ledger_seq = request.GetParamValue("seq");
		if (ledger_seq.empty())
			ledger_seq = utils::String::ToString(LedgerManager::Instance().GetLastClosedLedger().seq());

		protocol::ValidatorSet set;
		if (!LedgerManager::Instance().GetValidators(utils::String::Stoi64(ledger_seq), set)) {
			error_code = protocol::ERRCODE_NOT_EXIST;
		}
		else {
			result = Proto2Json(set);
		}

		reply_json["error_code"] = error_code;
		reply = reply_json.toStyledString();
	}

	void WebServer::GetLedger(const http::server::request &request, std::string &reply) {
		std::string ledger_seq = request.GetParamValue("seq");
		std::string with_validator = request.GetParamValue("with_validator");
		std::string with_consvalue = request.GetParamValue("with_consvalue");
		std::string with_fee = request.GetParamValue("with_fee");
		std::string with_block_reward = request.GetParamValue("with_block_reward");


		/// default last closed ledger
		if (ledger_seq.empty())
			ledger_seq = utils::String::ToString(LedgerManager::Instance().GetLastClosedLedger().seq());


		int32_t error_code = protocol::ERRCODE_SUCCESS;
		Json::Value reply_json = Json::Value(Json::objectValue);
		Json::Value record = Json::Value(Json::arrayValue);
		Json::Value &result = reply_json["result"];

		LedgerFrm frm;
		do {
			int64_t seq = utils::String::Stoi64(ledger_seq);
			if (!frm.LoadFromDb(seq)) {
				error_code = protocol::ERRCODE_NOT_EXIST;
				break;
			}
			result = frm.ToJson();

			if (with_validator == "true") {
				protocol::ValidatorSet set;
				if (LedgerManager::Instance().GetValidators(seq, set)) {
					Json::Value validator = Proto2Json(set);
					result["validators"] = validator["validators"];
				}
				else {
					error_code = protocol::ERRCODE_NOT_EXIST;
					break;
				}
			}

			if (with_fee == "true") {
				protocol::FeeConfig set;
				if (LedgerManager::Instance().FeesConfigGet(frm.GetProtoHeader().fees_hash(), set)) {
					Json::Value fees = Proto2Json(set);
					result["fees"] = fees;
				}
				else {
					error_code = protocol::ERRCODE_NOT_EXIST;
					break;
				}
			}

			if (with_consvalue == "true") {
				protocol::ConsensusValue cons;
				if (LedgerManager::Instance().ConsensusValueFromDB(seq, cons)) {
					result["consensus_value"] = Proto2Json(cons);
				}
				else {
					error_code = protocol::ERRCODE_NOT_EXIST;
					break;
				}

				Json::Value &json_cons = result["consensus_value"];
				protocol::PbftProof pbft_evidence;
				if (!pbft_evidence.ParseFromString(cons.previous_proof())) {
					error_code = protocol::ERRCODE_INTERNAL_ERROR;
					break;
				}

				json_cons["previous_proof_plain"] = Proto2Json(pbft_evidence);
			}

			if (with_block_reward == "true"){
				int64_t blockReward = GetBlockReward(seq);
				result["block_reward"] = blockReward;

				protocol::ValidatorSet sets;
				if (!LedgerManager::Instance().GetValidators(seq - 1, sets)) {
					error_code = protocol::ERRCODE_NOT_EXIST;
					break;
				}

				if (sets.validators_size() == 0) {
					error_code = protocol::ERRCODE_INTERNAL_ERROR;
					break;
				}

				bool avgReward = false;
				int64_t totalPledges = 0;
				for (int32_t i = 0; i < sets.validators_size(); i++) {
					totalPledges += sets.validators(i).pledge_coin_amount();
				}

				if (totalPledges == 0) {
					avgReward = true;
				}

				int64_t average_reward = 0;
				Json::Value &validatorsReward = result["validators_reward"];
				if (avgReward){
					average_reward = blockReward / sets.validators_size();
				}

				int64_t leftReward = blockReward;
				for (int32_t i = 0; i < sets.validators_size(); i++) {
					if (avgReward) {
						validatorsReward[sets.validators(i).address()] = average_reward;
						leftReward -= average_reward;
					}
					else {
						int64_t reward = blockReward * sets.validators(i).pledge_coin_amount() / totalPledges;
						validatorsReward[sets.validators(i).address()] = reward;
						leftReward -= reward;
					}
				}

				int64_t randomIndex = seq % sets.validators_size();
				int64_t baseReward = validatorsReward[sets.validators(randomIndex).address()].asInt64();
				validatorsReward[sets.validators(randomIndex).address()] = baseReward + leftReward;
			}
			
		} while (false);


		reply_json["error_code"] = error_code;
		reply = reply_json.toStyledString();
	}

	void WebServer::GetConsensusInfo(const http::server::request &request, std::string &reply) {
		Json::Value root;
		ConsensusManager::Instance().GetConsensus()->GetModuleStatus(root);
		reply = root.toStyledString();
	}

	void WebServer::GetAddress(const http::server::request &request, std::string &reply) {
		std::string private_key = request.GetParamValue("private_key");
		std::string public_key = request.GetParamValue("public_key");
		Json::Value reply_json = Json::Value(Json::objectValue);

		if (!private_key.empty()) {
			PrivateKey key(private_key);
			if (key.IsValid()) {
				reply_json["error_code"] = protocol::ERRCODE_SUCCESS;
				Json::Value &result = reply_json["result"];
                result["public_key"] = key.GetEncPublicKey();
                result["private_key"] = key.GetEncPrivateKey();
                result["address"] = key.GetEncAddress();
				result["private_raw"] = key.GetRawPrivateKey();
                result["public_key_raw"] = EncodePublicKey(key.GetRawPublicKey());
				result["sign_type"] = GetSignTypeDesc(key.GetSignType());
			}
			else {
				reply_json["error_code"] = protocol::ERRCODE_INVALID_PARAMETER;
			}
		}
		else if (!public_key.empty()) {
			PublicKey key(public_key);
			if (key.IsValid()) {
				reply_json["error_code"] = protocol::ERRCODE_SUCCESS;
				Json::Value &result = reply_json["result"];
                result["public_key"] = key.GetEncPublicKey();
                result["address"] = key.GetEncAddress();
				result["sign_type"] = GetSignTypeDesc(key.GetSignType());
			}
			else {
				reply_json["error_code"] = protocol::ERRCODE_INVALID_PARAMETER;
			}
		}
		else {
			reply_json["error_code"] = protocol::ERRCODE_INVALID_PARAMETER;
		}

		reply = reply_json.toStyledString();
	}

	void WebServer::GetPeerNodeAddress(const http::server::request &request, std::string &reply) {
	}

	void WebServer::GetPeerAddresses(const http::server::request &request, std::string &reply) {
		uint32_t error_code = protocol::ERRCODE_SUCCESS;
		Json::Value reply_json = Json::Value(Json::objectValue);

		do {
			std::string peers;
			KeyValueDb *db = Storage::Instance().keyvalue_db();
			int32_t count = db->Get(General::PEERS_TABLE, peers);
			if (count < 0) {
				LOG_ERROR("Load peers info from db failed, error desc(%s)", db->error_desc().c_str());
				error_code = protocol::ERRCODE_INTERNAL_ERROR;
				break;
			}

			if (count == 0) {
				LOG_ERROR("Load peers info from db failed, not initialize");
				break;
			}

			protocol::Peers all;
			if (!all.ParseFromString(peers)) {
				LOG_ERROR("Parse peers string failed");
				break;
			}

			Json::Value &result = reply_json["result"];
			result = Proto2Json(all);
		} while (false);

		reply_json["error_code"] = error_code;
		reply = reply_json.toFastString();
	}

	void WebServer::ContractQuery(const http::server::request &request, std::string &reply) {
		std::string address = request.GetParamValue("address");
		std::string args = request.GetParamValue("input"); //eg. "arg1,arg2,arg3...",need user code(js) design,split and parse

		int32_t error_code = protocol::ERRCODE_SUCCESS;
		std::string error_desc;
		AccountFrm::pointer acc = NULL;

		Json::Value reply_json = Json::Value(Json::objectValue);
		Json::Value &result = reply_json["result"];

		do {
			if (!Environment::AccountFromDB(address, acc)) {
				error_code = protocol::ERRCODE_NOT_EXIST;
				error_desc = utils::String::Format("Account(%s) not exist", address.c_str());
				LOG_ERROR("%s", error_desc.c_str());
				break;
			}

			std::string code = acc->GetProtoAccount().contract().payload();
			if (code.empty()) {
				error_code = protocol::ERRCODE_NOT_EXIST;
				error_desc = utils::String::Format("Account(%s) has no contract code", address.c_str());
				LOG_ERROR("%s", error_desc.c_str());
				break;
			}

			ContractParameter parameter;
			parameter.code_ = code;
			parameter.input_ = args;
// 			if (!ContractManager::Instance().Query(Contract::TYPE_V8, parameter, result)) {
// 				error_code = protocol::ERRCODE_CONTRACT_EXECUTE_FAIL;
// 				error_desc = utils::String::Format("Account(%s) contract executed failed", address.c_str());
// 				LOG_ERROR("%s", error_desc.c_str());
// 				break;
// 			}

		} while (false);

		reply_json["error_code"] = error_code;
		reply_json["error_desc"] = error_desc;
		reply = reply_json.toStyledString();
	}

#if 0
	void WebServer::GetSignature(const http::server::request &request, std::string &reply) {

		uint32_t error_code = protocol::ERRCODE_SUCCESS;
		Json::Value reply_json = Json::Value(Json::objectValue);

		std::string strBody = request.body;
		Json::Value body;
		body.fromString(strBody);
		std::string prikey = body["private_key"].asString();
		std::string str_blob = body["transaction_blob"].asString();

		do {
			PrivateKey key(prikey);
			if (!key.IsValid()) {
				error_code = protocol::ERRCODE_INVALID_PRIKEY;
				break;
			}
			std::string blob_decode;
			utils::decode_b16(str_blob, blob_decode);

			std::string strOut = key.Sign(blob_decode);
			if (strOut.empty()) {
				error_code = protocol::ERRCODE_INTERNAL_ERROR;
			}

			Json::Value &result = reply_json["result"];

			result["signature"] = utils::encode_b16(strOut);
			//PublicKey pkey = key.GetPublicKey();
			result["public_key"] = key.GetBase16PublicKey();

		} while (false);

		reply_json["error_code"] = error_code;
		reply = reply_json.toStyledString();
	}

	void WebServer::GetAccountTree(const http::server::request &request, std::string &reply) {
		//Json::Value jsonRoot, stat;
		//auto tree = LedgerManager::Instance().tree_;
		//int n = 0;
		//tree->(nullptr, 0, jsonRoot, stat, n, true);
		//reply = jsonRoot.toStyledString();
	}
#endif
}