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


#include <utils/logger.h>
#include <common/pb2json.h>
#include <common/private_key.h>
#include "ledger_frm.h"
#include "ledger_manager.h"
#include "contract_manager.h"

    
namespace phantom{

	ContractParameter::ContractParameter() : ope_index_(-1), ledger_context_(NULL), pay_coin_amount_(0){}

	ContractParameter::~ContractParameter() {}

	ContractTestParameter::ContractTestParameter() : opt_type_(QUERY), fee_limit_(0), gas_price_(0), contract_balance_(0){}

	ContractTestParameter::~ContractTestParameter() {}

	TransactionTestParameter::TransactionTestParameter() {}
	TransactionTestParameter::~TransactionTestParameter() {}

	utils::Mutex Contract::contract_id_seed_lock_;
	int64_t Contract::contract_id_seed_ = 0; 
	Contract::Contract() {
		utils::MutexGuard guard(contract_id_seed_lock_);
		id_ = contract_id_seed_;
		contract_id_seed_++;
		tx_do_count_ = 0;
		readonly_ = false;
	}

	Contract::Contract(bool readonly, const ContractParameter &parameter) :
		readonly_(readonly), parameter_(parameter) {
		utils::MutexGuard guard(contract_id_seed_lock_);
		id_ = contract_id_seed_;
		contract_id_seed_++;
		tx_do_count_ = 0;
	}

	Contract::~Contract() {}

	bool Contract::Execute() {
		return true;
	}

	bool Contract::InitContract() {
		return true;
	}

	bool Contract::Cancel() {
		return true;
	}

	bool Contract::Query(Json::Value& jsResult) {
		return true;
	}

	bool Contract::SourceCodeCheck() {
		return true;
	}

	int64_t Contract::GetId() {
		return id_;
	}

	int32_t Contract::GetTxDoCount() {
		return tx_do_count_;
	}

	void Contract::IncTxDoCount() {
		tx_do_count_++;
	}

	const ContractParameter &Contract::GetParameter() {
		return parameter_;
	}

	bool Contract::IsReadonly() {
		return readonly_;
	}

	const utils::StringList &Contract::GetLogs() {
		return logs_;
	}

	void Contract::AddLog(const std::string &log) {
		logs_.push_back(log);
		if (logs_.size() > 10) logs_.pop_front();
	}

	Result &Contract::GetResult() {
		return result_;
	}

	void Contract::SetResult(Result &result) {
		result_ = result;
	}

	std::map<std::string, std::string> V8Contract::jslib_sources;
	std::map<std::string, v8::FunctionCallback> V8Contract::js_func_read_;
	std::map<std::string, v8::FunctionCallback> V8Contract::js_func_write_;
	std::string V8Contract::user_global_string_;

	const std::string V8Contract::sender_name_ = "sender";
	const std::string V8Contract::this_address_ = "thisAddress";
	const char* V8Contract::main_name_ = "main";
	const char* V8Contract::query_name_ = "query";
	const char* V8Contract::init_name_ = "init";
	const char* V8Contract::call_jslint_ = "callJslint";
	const std::string V8Contract::trigger_tx_name_ = "trigger";
	const std::string V8Contract::trigger_tx_index_name_ = "triggerIndex";
	const std::string V8Contract::this_header_name_ = "consensusValue";
	const std::string V8Contract::pay_coin_amount_name_ = "thisPayCoinAmount";
	const std::string V8Contract::pay_asset_amount_name_ = "thisPayAsset";
	const std::string V8Contract::block_timestamp_name_ = "blockTimestamp";
	const std::string V8Contract::block_number_name_ = "blockNumber";
	utils::Mutex V8Contract::isolate_to_contract_mutex_;
	std::unordered_map<v8::Isolate*, V8Contract *> V8Contract::isolate_to_contract_;

	v8::Platform* V8Contract::platform_ = nullptr;
	v8::Isolate::CreateParams V8Contract::create_params_;

	V8Contract::V8Contract(bool readonly, const ContractParameter &parameter) : Contract(readonly,parameter) {
		type_ = TYPE_V8;
		isolate_ = v8::Isolate::New(create_params_);

		utils::MutexGuard guard(isolate_to_contract_mutex_);
		isolate_to_contract_[isolate_] = this;
	}

	V8Contract::~V8Contract() {
		utils::MutexGuard guard(isolate_to_contract_mutex_);
		isolate_to_contract_.erase(isolate_);
		isolate_->Dispose();
		isolate_ = NULL;
	}

	bool V8Contract::LoadJsLibSource() {
		std::string lib_path = utils::String::Format("%s/jslib", utils::File::GetBinHome().c_str());
		utils::FileAttributes files;
		utils::File::GetFileList(lib_path, "*.js", files);
		for (utils::FileAttributes::iterator iter = files.begin(); iter != files.end(); iter++) {
			utils::FileAttribute attr = iter->second;
			utils::File file;
			std::string file_path = utils::String::Format("%s/%s", lib_path.c_str(), iter->first.c_str());
			if (!file.Open(file_path, utils::File::FILE_M_READ)) {
				LOG_ERROR_ERRNO("Failed to open js lib file, path(%s)", file_path.c_str(), STD_ERR_CODE, STD_ERR_DESC);
				continue;
			}

			std::string data;
			if (file.ReadData(data, 10 * utils::BYTES_PER_MEGA) < 0) {
				LOG_ERROR_ERRNO("Failed to read js lib file, path(%s)", file_path.c_str(), STD_ERR_CODE, STD_ERR_DESC);
				continue;
			}

			jslib_sources[iter->first] = data;
		}

		return true;
	}

	bool V8Contract::LoadJslintGlobalString(){
		user_global_string_ = utils::String::AppendFormat(user_global_string_, "%s", sender_name_.c_str());
		user_global_string_ = utils::String::AppendFormat(user_global_string_, ",%s", this_address_.c_str());
		user_global_string_ = utils::String::AppendFormat(user_global_string_, ",%s", trigger_tx_name_.c_str());
		user_global_string_ = utils::String::AppendFormat(user_global_string_, ",%s", trigger_tx_index_name_.c_str());
		user_global_string_ = utils::String::AppendFormat(user_global_string_, ",%s", this_header_name_.c_str());
		user_global_string_ = utils::String::AppendFormat(user_global_string_, ",%s", pay_coin_amount_name_.c_str());
		user_global_string_ = utils::String::AppendFormat(user_global_string_, ",%s", pay_asset_amount_name_.c_str());
		user_global_string_ = utils::String::AppendFormat(user_global_string_, ",%s", block_timestamp_name_.c_str());
		user_global_string_ = utils::String::AppendFormat(user_global_string_, ",%s", block_number_name_.c_str());
		std::map<std::string, v8::FunctionCallback>::iterator itr = js_func_read_.begin();
		for ( ; itr != js_func_read_.end(); itr++)
		{
			user_global_string_ = utils::String::AppendFormat(user_global_string_, ",%s", itr->first.c_str());
		}

		itr = js_func_write_.begin();
		for (; itr != js_func_write_.end(); itr++)
		{
			user_global_string_ = utils::String::AppendFormat(user_global_string_, ",%s", itr->first.c_str());
		}
		return true;
	}

	bool V8Contract::Initialize(int argc, char** argv) {
		//read func
		js_func_read_["log"] = V8Contract::CallBackLog;
		js_func_read_["getBalance"] = V8Contract::CallBackGetBalance;
		js_func_read_["getAccountAsset"] = V8Contract::CallBackGetAccountAsset;
		js_func_read_["storageLoad"] = V8Contract::CallBackStorageLoad;
		js_func_read_["getBlockHash"] = V8Contract::CallBackGetBlockHash;
		js_func_read_["contractQuery"] = V8Contract::CallBackContractQuery;
		js_func_read_["getValidators"] = V8Contract::CallBackGetValidators;
		js_func_read_[General::CHECK_TIME_FUNCTION] = V8Contract::InternalCheckTime;
		js_func_read_["stoI64Check"] = V8Contract::CallBackStoI64Check;
		js_func_read_["int64Add"] = V8Contract::CallBackInt64Add;
		js_func_read_["int64Sub"] = V8Contract::CallBackInt64Sub;
		js_func_read_["int64Mul"] = V8Contract::CallBackInt64Mul;
		js_func_read_["int64Mod"] = V8Contract::CallBackInt64Mod;
		js_func_read_["int64Div"] = V8Contract::CallBackInt64Div;
		js_func_read_["int64Compare"] = V8Contract::CallBackInt64Compare;
		js_func_read_["toBaseUnit"] = V8Contract::CallBackToBaseUnit;
		js_func_read_["assert"] = V8Contract::CallBackAssert;
		js_func_read_["addressCheck"] = V8Contract::CallBackAddressValidCheck;

		//write func
		js_func_write_["storageStore"] = V8Contract::CallBackStorageStore;
		js_func_write_["storageDel"] = V8Contract::CallBackStorageDel;
		//js_func_write_["doTransaction"] = V8Contract::CallBackDoTransaction;
		js_func_write_["configFee"] = V8Contract::CallBackConfigFee;
		js_func_write_["setValidators"] = V8Contract::CallBackSetValidators;
		js_func_write_["payCoin"] = V8Contract::CallBackPayCoin;
		js_func_write_["issueAsset"] = V8Contract::CallBackIssueAsset;
		js_func_write_["payAsset"] = V8Contract::CallBackPayAsset;
		js_func_write_["tlog"] = V8Contract::CallBackTopicLog;

		LoadJsLibSource();
		LoadJslintGlobalString();
		v8::V8::InitializeICUDefaultLocation(argv[0]);
		v8::V8::InitializeExternalStartupData(argv[0]);
		platform_ = v8::platform::CreateDefaultPlatform();
		v8::V8::InitializePlatform(platform_);
		if (!v8::V8::Initialize()) {
			LOG_ERROR("Failed to initialize V8.");
			return false;
		}
		create_params_.array_buffer_allocator =
			v8::ArrayBuffer::Allocator::NewDefaultAllocator();

		return true;
	}

	bool V8Contract::ExecuteCode(const char* fname){
		v8::Isolate::Scope isolate_scope(isolate_);
		v8::HandleScope handle_scope(isolate_);
		v8::TryCatch try_catch(isolate_);

		v8::Local<v8::Context> context = CreateContext(isolate_, false);

		v8::Context::Scope context_scope(context);

		//block number, timestamp, orginal

		auto string_sender = v8::String::NewFromUtf8(isolate_, parameter_.sender_.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
		context->Global()->Set(context,
			v8::String::NewFromUtf8(isolate_, sender_name_.c_str(), v8::NewStringType::kNormal).ToLocalChecked(),
			string_sender);

		auto string_contractor = v8::String::NewFromUtf8(isolate_, parameter_.this_address_.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
		context->Global()->Set(context,
			v8::String::NewFromUtf8(isolate_, this_address_.c_str(), v8::NewStringType::kNormal).ToLocalChecked(),
			string_contractor);

		v8::Local<v8::Integer> index_v8 = v8::Int32::New(isolate_, parameter_.ope_index_);
		context->Global()->Set(context,
			v8::String::NewFromUtf8(isolate_, trigger_tx_index_name_.c_str(), v8::NewStringType::kNormal).ToLocalChecked(),
			index_v8);

		auto coin_amount = v8::String::NewFromUtf8(isolate_, utils::String::ToString(parameter_.pay_coin_amount_).c_str(), v8::NewStringType::kNormal).ToLocalChecked();
		context->Global()->Set(context,
			v8::String::NewFromUtf8(isolate_, pay_coin_amount_name_.c_str(), v8::NewStringType::kNormal).ToLocalChecked(),
			coin_amount);

		if (parameter_.pay_asset_amount_.has_key()) {
			v8::Local<v8::Object> v8_asset = v8::Object::New(isolate_);
			v8::Local<v8::Object> v8_asset_property = v8::Object::New(isolate_);
			const protocol::AssetKey &asset_key = parameter_.pay_asset_amount_.key();
			v8_asset_property->Set(v8::String::NewFromUtf8(isolate_, "issuer"), v8::String::NewFromUtf8(isolate_, asset_key.issuer().c_str()));
			v8_asset_property->Set(v8::String::NewFromUtf8(isolate_, "code"), v8::String::NewFromUtf8(isolate_, asset_key.code().c_str()));
			v8_asset->Set(v8::String::NewFromUtf8(isolate_, "amount"), v8::String::NewFromUtf8(isolate_, utils::String::ToString(parameter_.pay_asset_amount_.amount()).c_str()));
			v8_asset->Set(v8::String::NewFromUtf8(isolate_, "key"), v8_asset_property);

			context->Global()->Set(context,
				v8::String::NewFromUtf8(isolate_, pay_asset_amount_name_.c_str(), v8::NewStringType::kNormal).ToLocalChecked(),
				v8_asset);
		}

		auto blocknumber_v8 = v8::Number::New(isolate_, (double)parameter_.blocknumber_);
		context->Global()->Set(context,
			v8::String::NewFromUtf8(isolate_, block_number_name_.c_str(), v8::NewStringType::kNormal).ToLocalChecked(),
			blocknumber_v8);

		auto timestamp_v8 = v8::Number::New(isolate_, (double)parameter_.timestamp_);
		context->Global()->Set(context,
			v8::String::NewFromUtf8(isolate_, block_timestamp_name_.c_str(), v8::NewStringType::kNormal).ToLocalChecked(),
			timestamp_v8);

		v8::Local<v8::String> v8src = v8::String::NewFromUtf8(isolate_, parameter_.code_.c_str());
		v8::Local<v8::Script> compiled_script;

		do {
			Json::Value error_random;
			if (!RemoveRandom(isolate_, error_random)) {
				result_.set_desc(error_random.toFastString());
				break;
			}

			v8::Local<v8::String> check_time_name(
				v8::String::NewFromUtf8(context->GetIsolate(), "__enable_check_time__",
				v8::NewStringType::kNormal).ToLocalChecked());
			v8::ScriptOrigin origin_check_time_name(check_time_name);

			if (!v8::Script::Compile(context, v8src, &origin_check_time_name).ToLocal(&compiled_script)) {
				result_.set_desc(ReportException(isolate_, &try_catch).toFastString());
				break;
			}

			v8::Local<v8::Value> result;
			if (!compiled_script->Run(context).ToLocal(&result)) {
				result_.set_desc(ReportException(isolate_, &try_catch).toFastString());
				break;
			}

			v8::Local<v8::String> process_name =
				v8::String::NewFromUtf8(isolate_, fname, v8::NewStringType::kNormal, strlen(fname))
				.ToLocalChecked();
			v8::Local<v8::Value> process_val;

			if (!context->Global()->Get(context, process_name).ToLocal(&process_val) ||
				!process_val->IsFunction()) {
				Json::Value json_result;
				json_result["exception"] = utils::String::Format("Lost of %s function", fname);
				result_.set_code(protocol::ERRCODE_CONTRACT_EXECUTE_FAIL);
				result_.set_desc(json_result.toFastString());
				LOG_ERROR("%s", result_.desc().c_str());
				break;
			}

			v8::Local<v8::Function> process = v8::Local<v8::Function>::Cast(process_val);

			const int argc = 1;
			v8::Local<v8::String> arg1 = v8::String::NewFromUtf8(isolate_, parameter_.input_.c_str(), v8::NewStringType::kNormal).ToLocalChecked();

			v8::Local<v8::Value> argv[argc];
			argv[0] = arg1;

			v8::Local<v8::Value> callresult;
			if (!process->Call(context, context->Global(), argc, argv).ToLocal(&callresult)) {
				if (result_.code() == 0) { //Set the code if it is not set.
					result_.set_code(protocol::ERRCODE_CONTRACT_EXECUTE_FAIL);
					result_.set_desc(ReportException(isolate_, &try_catch).toFastString());
				}
				//Otherwise break. For example doTransaction has set the code.
				break;
			}

			return true;
		} while (false);
		return false;
	}

	bool V8Contract::Execute() {
		return ExecuteCode(main_name_);
	}

	bool V8Contract::InitContract(){
		return ExecuteCode(init_name_);
	}

	bool V8Contract::Cancel() {
		v8::V8::TerminateExecution(isolate_);
		return true;
	}

	bool V8Contract::SourceCodeCheck() {
		v8::Isolate::Scope isolate_scope(isolate_);
		v8::HandleScope handle_scope(isolate_);
		v8::TryCatch try_catch(isolate_);

		v8::Local<v8::Context> context = CreateContext(isolate_, false);
		v8::Context::Scope context_scope(context);

		std::string jslint_file = "jslint.js";
		std::map<std::string, std::string>::iterator find_jslint_source = jslib_sources.find(jslint_file);
		if (find_jslint_source == jslib_sources.end()) {
			Json::Value json_result;
			json_result["exception"] = utils::String::Format("Failed to find the include file(%s) in jslib directory", jslint_file.c_str());
			result_.set_code(protocol::ERRCODE_CONTRACT_SYNTAX_ERROR);
			result_.set_desc(json_result.toFastString());
			LOG_ERROR("Failed to find the include file(%s) in jslib directory", jslint_file.c_str());
			return false;
		}

		v8::Local<v8::String> v8src = v8::String::NewFromUtf8(isolate_, find_jslint_source->second.c_str());
		v8::Local<v8::Script> compiled_script;
		if (!v8::Script::Compile(context, v8src).ToLocal(&compiled_script)) {
			result_.set_code(protocol::ERRCODE_CONTRACT_SYNTAX_ERROR);
			result_.set_desc(ReportException(isolate_, &try_catch).toFastString());
			LOG_ERROR("%s", result_.desc().c_str());
			return false;
		}
		Json::Value error_desc_f;
		v8::Local<v8::Value> result;
		if (!compiled_script->Run(context).ToLocal(&result)) {
			result_.set_code(protocol::ERRCODE_CONTRACT_SYNTAX_ERROR);
			result_.set_desc(ReportException(isolate_, &try_catch).toFastString());
			LOG_ERROR("%s", result_.desc().c_str());
			return false;
		}
		v8::Local<v8::String> process_name = v8::String::NewFromUtf8(
			isolate_, V8Contract::call_jslint_, v8::NewStringType::kNormal, strlen(V8Contract::call_jslint_)).ToLocalChecked();

		v8::Local<v8::Value> process_val;
		if (!context->Global()->Get(context, process_name).ToLocal(&process_val) ||
			!process_val->IsFunction()) {
			Json::Value json_result;
			json_result["exception"] = utils::String::Format("Failed to find jslint name(%s)", call_jslint_);
			result_.set_code(protocol::ERRCODE_CONTRACT_SYNTAX_ERROR);
			result_.set_desc(json_result.toFastString());
			LOG_ERROR("%s", result_.desc().c_str());
			return false;
		}

		v8::Local<v8::Function> process = v8::Local<v8::Function>::Cast(process_val);
		const int argc = 2;
		v8::Local<v8::Value>  argv[argc];
		v8::Local<v8::String> arg1 = v8::String::NewFromUtf8(isolate_, parameter_.code_.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
		v8::Local<v8::String> arg2 = v8::String::NewFromUtf8(isolate_, user_global_string_.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
		argv[0] = arg1;
		argv[1] = arg2;

		v8::Local<v8::Value> callRet;
		if (!process->Call(context, context->Global(), argc, argv).ToLocal(&callRet)) {
			result_.set_code(protocol::ERRCODE_CONTRACT_SYNTAX_ERROR);
			result_.set_desc(ReportException(isolate_, &try_catch).toFastString());
			LOG_ERROR("%s", result_.desc().c_str());
			return false;
		}
		if (!callRet->IsString()) { 
			Json::Value json_result;
			json_result["exception"] = utils::String::Format("The result of jslint calling is not a string!");
			result_.set_code(protocol::ERRCODE_CONTRACT_SYNTAX_ERROR);
			result_.set_desc(json_result.toFastString());
			LOG_ERROR("%s", result_.desc().c_str());
			return false;
		}

		Json::Reader reader;
		Json::Value call_result_json;
		if (!reader.parse(std::string(ToCString(v8::String::Utf8Value(callRet))), call_result_json)) {
			Json::Value json_result;
			json_result["exception"] = utils::String::Format("Failed to parse jslint result, (%s)", reader.getFormatedErrorMessages().c_str());
			result_.set_code(protocol::ERRCODE_CONTRACT_SYNTAX_ERROR);
			result_.set_desc(json_result.toFastString());
			LOG_ERROR("%s", result_.desc().c_str());
			return false;
		}
		if (!call_result_json.empty())
		{
			Json::Value json_result;
			json_result["exception"] = utils::String::Format("Failed to parse jslint result, (%s)", reader.getFormatedErrorMessages().c_str());
			result_.set_code(protocol::ERRCODE_CONTRACT_SYNTAX_ERROR);
			result_.set_desc(call_result_json.toFastString());
			LOG_ERROR("%s", result_.desc().c_str());
			return false;
		}

		LOG_INFO("Parse jslint successfully!");
		return true;
	}

	bool V8Contract::Query(Json::Value& js_result) {
		v8::Isolate::Scope isolate_scope(isolate_);
		v8::HandleScope    handle_scope(isolate_);
		v8::TryCatch       try_catch(isolate_);

		v8::Local<v8::Context>       context = CreateContext(isolate_, true);
		v8::Context::Scope            context_scope(context);

		auto string_sender = v8::String::NewFromUtf8(isolate_, parameter_.sender_.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
		context->Global()->Set(context,
			v8::String::NewFromUtf8(isolate_, sender_name_.c_str(), v8::NewStringType::kNormal).ToLocalChecked(),
			string_sender);

		auto string_contractor = v8::String::NewFromUtf8(isolate_, parameter_.this_address_.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
		context->Global()->Set(context,
			v8::String::NewFromUtf8(isolate_, this_address_.c_str(), v8::NewStringType::kNormal).ToLocalChecked(),
			string_contractor);

		v8::Local<v8::Integer> index_v8 = v8::Int32::New(isolate_, parameter_.ope_index_);
		context->Global()->Set(context,
			v8::String::NewFromUtf8(isolate_, trigger_tx_index_name_.c_str(), v8::NewStringType::kNormal).ToLocalChecked(),
			index_v8);

		auto blocknumber_v8 = v8::Number::New(isolate_, (double)parameter_.blocknumber_);
		context->Global()->Set(context,
			v8::String::NewFromUtf8(isolate_, block_number_name_.c_str(), v8::NewStringType::kNormal).ToLocalChecked(),
			blocknumber_v8);

		auto timestamp_v8 = v8::Number::New(isolate_, (double)parameter_.timestamp_);
		context->Global()->Set(context,
			v8::String::NewFromUtf8(isolate_, block_timestamp_name_.c_str(), v8::NewStringType::kNormal).ToLocalChecked(),
			timestamp_v8);


		v8::Local<v8::String> v8src = v8::String::NewFromUtf8(isolate_, parameter_.code_.c_str());
		v8::Local<v8::Script> compiled_script;

		Json::Value error_desc_f;
		Json::Value temp_result;
		do {
			if (!RemoveRandom(isolate_, error_desc_f)) {
				break;
			}

			v8::Local<v8::String> check_time_name(
				v8::String::NewFromUtf8(context->GetIsolate(), "__enable_check_time__",
				v8::NewStringType::kNormal).ToLocalChecked());
			v8::ScriptOrigin origin_check_time_name(check_time_name);

			if (!v8::Script::Compile(context, v8src, &origin_check_time_name).ToLocal(&compiled_script)) {
				error_desc_f = ReportException(isolate_, &try_catch);
				break;
			}

			v8::Local<v8::Value> result;
			if (!compiled_script->Run(context).ToLocal(&result)) {
				error_desc_f = ReportException(isolate_, &try_catch);
				break;
			}

			v8::Local<v8::String> process_name = v8::String::NewFromUtf8(
				isolate_, query_name_, v8::NewStringType::kNormal, strlen(query_name_)).ToLocalChecked();

			v8::Local<v8::Value> process_val;
			if (!context->Global()->Get(context, process_name).ToLocal(&process_val) ||
				!process_val->IsFunction()) {
				Json::Value &exception = error_desc_f["exception"];
				exception = utils::String::Format("Lost of %s function", query_name_);
				LOG_ERROR("%s", exception.asCString());
				break;
			}

			v8::Local<v8::Function> process = v8::Local<v8::Function>::Cast(process_val);

			const int argc = 1;
			v8::Local<v8::Value>  argv[argc];
			v8::Local<v8::String> arg1 = v8::String::NewFromUtf8(isolate_, parameter_.input_.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
			argv[0] = arg1;

			v8::Local<v8::Value> callRet;
			if (!process->Call(context, context->Global(), argc, argv).ToLocal(&callRet)) {
				error_desc_f = ReportException(isolate_, &try_catch);
				LOG_ERROR("Failed to execute %s function.", query_name_);
				break;
			}

			JsValueToCppJson(context, callRet, temp_result);
			js_result["result"] = temp_result;
			return true;
		} while (false);

		Json::Value &error_obj = js_result["error"];
		error_obj ["data"] = error_desc_f;
		return false;
	}


	V8Contract *V8Contract::GetContractFrom(v8::Isolate* isolate) {
		utils::MutexGuard guard(isolate_to_contract_mutex_);
		std::unordered_map<v8::Isolate*, V8Contract *>::iterator iter = isolate_to_contract_.find(isolate);
		if (iter != isolate_to_contract_.end()){
			return iter->second;
		}

		return NULL;
	}

	bool V8Contract::RemoveRandom(v8::Isolate* isolate, Json::Value &error_msg) {
		v8::TryCatch try_catch(isolate);
		std::string js_file = "delete String.prototype.localeCompare; delete Date; delete Math;";

		v8::Local<v8::String> source = v8::String::NewFromUtf8(isolate, js_file.c_str());
		v8::Local<v8::Script> script;
		
		v8::Local<v8::String> check_time_name(
			v8::String::NewFromUtf8(isolate->GetCurrentContext()->GetIsolate(), "__enable_check_time__",
			v8::NewStringType::kNormal).ToLocalChecked());
		v8::ScriptOrigin origin_check_time_name(check_time_name);

		if (!v8::Script::Compile(isolate->GetCurrentContext(), source, &origin_check_time_name).ToLocal(&script)) {
			error_msg = ReportException(isolate, &try_catch);
			return false;
		}

		v8::Local<v8::Value> result;
		if (!script->Run(isolate->GetCurrentContext()).ToLocal(&result)) {
			error_msg = ReportException(isolate, &try_catch);
			return false;
		}

		return true;
	}

	v8::Local<v8::Context> V8Contract::CreateContext(v8::Isolate* isolate, bool readonly) {
		// Create a template for the global object.
		v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);
		std::map<std::string, v8::FunctionCallback>::iterator itr = js_func_read_.begin();
		for (; itr != js_func_read_.end(); itr++) {
			global->Set(
				v8::String::NewFromUtf8(isolate, itr->first.c_str(), v8::NewStringType::kNormal)
				.ToLocalChecked(),
				v8::FunctionTemplate::New(isolate, itr->second));
		}
		if (!readonly){
			itr = js_func_write_.begin();
			for (; itr != js_func_write_.end(); itr++) {
				global->Set(
					v8::String::NewFromUtf8(isolate, itr->first.c_str(), v8::NewStringType::kNormal)
					.ToLocalChecked(),
					v8::FunctionTemplate::New(isolate, itr->second));
			}
		}

		return v8::Context::New(isolate, NULL, global);
	}

	Json::Value V8Contract::ReportException(v8::Isolate* isolate, v8::TryCatch* try_catch) {
		v8::HandleScope handle_scope(isolate);
		v8::String::Utf8Value exception(try_catch->Exception());
		const char* exception_string = ToCString(exception);
		std::string exec_string(exception_string);
		exec_string.resize(256);
		Json::Value json_result;

		std::string contract_address = "";
		V8Contract *v8_contract = GetContractFrom(isolate);
		if (v8_contract!=nullptr ) {
			contract_address = v8_contract->parameter_.this_address_;
		}

		v8::Local<v8::Message> message = try_catch->Message();
		std::string error_msg;
		if (message.IsEmpty()) {
			// V8 didn't provide any extra information about this error; just
			// print the exception.
			json_result["exception"] = exec_string;
			json_result["contract"] = contract_address;
		}
		else {
			// Print (filename):(line number): (message).
			v8::String::Utf8Value filename(message->GetScriptOrigin().ResourceName());
			v8::Local<v8::Context> context(isolate->GetCurrentContext());
			const char* filename_string = ToCString(filename);
			int linenum = message->GetLineNumber(context).FromJust();
			//json_result["filename"] = filename_string;
			json_result["linenum"] = linenum;
			json_result["exception"] = exec_string;
			json_result["contract"] = contract_address;

			//Print error stack
			v8::Local<v8::Value> stack_trace_string;
			if (try_catch->StackTrace(context).ToLocal(&stack_trace_string) &&
				stack_trace_string->IsString() &&
				v8::Local<v8::String>::Cast(stack_trace_string)->Length() > 0) {
				v8::String::Utf8Value stack_trace(stack_trace_string);
				const char* stack_trace_string = ToCString(stack_trace);
				json_result["stack"] = stack_trace_string;
			}
		}

		LOG_ERROR("Faild to execute script: %s", json_result.toFastString().c_str());
		return json_result;
	}

	bool V8Contract::CppJsonToJsValue(v8::Isolate* isolate, Json::Value& jsonvalue, v8::Local<v8::Value>& jsvalue) {
		std::string type = jsonvalue["type"].asString();
		if (type == "string") {
			jsvalue = v8::String::NewFromUtf8(isolate, jsonvalue["value"].asCString());
		}
		else if (type == "bool") {
			jsvalue = v8::Boolean::New(isolate, jsonvalue["value"].asBool());
		}

		return true;
	}

	bool V8Contract::JsValueToCppJson(v8::Handle<v8::Context>& context, v8::Local<v8::Value>& jsvalue, Json::Value& jsonvalue) {
		if (jsvalue->IsBoolean()) {
			jsonvalue["type"] = "bool";
			jsonvalue["value"] = jsvalue->BooleanValue();
		}
		else if (jsvalue->IsString()) {
			jsonvalue["type"] = "string";
			jsonvalue["value"] = std::string(ToCString(v8::String::Utf8Value(jsvalue)));
		}
		else {
			jsonvalue["type"] = "bool";
			jsonvalue["value"] = false;
		}

		return true;
	}

	void V8Contract::CallBackConfigFee(const v8::FunctionCallbackInfo<v8::Value>& args){
		std::string error_desc;
		do{
			if (args.Length() < 1) {
				error_desc = "parameter number error";
				break;
			}
			if (!args[0]->IsString()) {
				error_desc = "parameter should be a string";
				break;
			}

			v8::HandleScope scope(args.GetIsolate());
			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			if (!v8_contract || !v8_contract->parameter_.ledger_context_) {
				error_desc = "Failed to find contract object by isolate id";
				break;
			}

			if (v8_contract->parameter_.this_address_ != General::CONTRACT_FEE_ADDRESS) {
				error_desc = "This address has no priority.";
				break;
			}

			v8::String::Utf8Value  utf8(args[0]);
			Json::Value json;
			if (!json.fromCString(ToCString(utf8))) {
				error_desc = "Failed to execute fromCString function, fatal error.";
				break;
			}

			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			LOG_INFO("UpdateFee: bottom tx(%s), top tx(%s), result(%s)", utils::String::BinToHexString(ledger_context->GetBottomTx()->GetContentHash()).c_str(),utils::String::BinToHexString(ledger_context->GetTopTx()->GetContentHash()).c_str(), json.toFastString().c_str());
			ledger_context->GetTopTx()->environment_->UpdateFeeConfig(json);
			args.GetReturnValue().Set(true);
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	void V8Contract::CallBackAssert(const v8::FunctionCallbackInfo<v8::Value>& args) {
		std::string error_desc = "Assertion exception occurred";
		do {
			if (args.Length() < 1 || args.Length() > 2) {
				error_desc.append(",parameter nums error");
				break;
			}
			if (!args[0]->IsBoolean()) {
				error_desc.append("parameter 0 should be boolean");
				break;
			}

			v8::HandleScope scope(args.GetIsolate());
			if (args.Length() == 2) {
				if (!args[1]->IsString()) {
					error_desc.append("parameter 1 should be string");
					break;
				}
				else {
					v8::String::Utf8Value str1(args[1]);
					error_desc = ToCString(str1);
				}
			}
			if (args[0]->BooleanValue() == false){
				break;
			}
			args.GetReturnValue().Set(true);
			return;
		}while (false);
        LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	V8Contract* V8Contract::UnwrapContract(v8::Local<v8::Object> obj) {
		v8::Local<v8::External> field = v8::Local<v8::External>::Cast(obj->GetInternalField(0));
		void* ptr = field->Value();
		return static_cast<V8Contract*>(ptr);
	}

	const char* V8Contract::ToCString(const v8::String::Utf8Value& value) {
		return *value ? *value : "<string conversion error>";
	}

	void V8Contract::Include(const v8::FunctionCallbackInfo<v8::Value>& args) {
		do {
			if (args.Length() != 1) {
				LOG_TRACE("Including parameter error, the length(%d) of args is not equal 1", args.Length());
				args.GetReturnValue().Set(false);
				break;
			}

			if (!args[0]->IsString()) {
				LOG_TRACE("Including parameter error, parameter should be a String");
				args.GetReturnValue().Set(false);
				break;
			}
			v8::String::Utf8Value str(args[0]);

			std::map<std::string, std::string>::iterator find_source = jslib_sources.find(*str);
			if (find_source == jslib_sources.end()) {
				LOG_TRACE("Failed to find the include file(%s) in jslib directory", *str);
				args.GetReturnValue().Set(false);
				break;
			}

			v8::TryCatch try_catch(args.GetIsolate());
			std::string js_file = find_source->second; //load_file(*str);

			v8::Local<v8::String> source = v8::String::NewFromUtf8(args.GetIsolate(), js_file.c_str());
			v8::Local<v8::Script> script;

			v8::Local<v8::String> check_time_name(
				v8::String::NewFromUtf8(args.GetIsolate()->GetCurrentContext()->GetIsolate(), "__enable_check_time__",
				v8::NewStringType::kNormal).ToLocalChecked());
			v8::ScriptOrigin origin_check_time_name(check_time_name);

			if (!v8::Script::Compile(args.GetIsolate()->GetCurrentContext(), source, &origin_check_time_name).ToLocal(&script)) {
				ReportException(args.GetIsolate(), &try_catch);
				break;
			}

			v8::Local<v8::Value> result;
			if (!script->Run(args.GetIsolate()->GetCurrentContext()).ToLocal(&result)) {
				ReportException(args.GetIsolate(), &try_catch);
			}
		} while (false);
		//return v8::Undefined(args.GetIsolate());
	}
	void V8Contract::InternalCheckTime(const v8::FunctionCallbackInfo<v8::Value>& args) {
		phantom::AccountFrm::pointer account_frm = nullptr;
		V8Contract *v8_contract = GetContractFrom(args.GetIsolate());

		if (v8_contract && v8_contract->GetParameter().ledger_context_){
			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			TransactionFrm::pointer ptr = ledger_context->GetBottomTx();
			ptr->ContractStepInc(1);

			//Check the storage
			v8::HeapStatistics stats;
			args.GetIsolate()->GetHeapStatistics(&stats);
			ptr->SetMemoryUsage(stats.used_heap_size());

			//Check the stack
			v8::V8InternalInfo internal_info;
			args.GetIsolate()->GetV8InternalInfo(internal_info);

			ptr->SetStackRemain(internal_info.remain_stack_size);
			//LOG_INFO("v8 remain stack:%d, now step:%d\n", internal_info.remain_stack_size, ptr->GetContractStep());

			std::string error_info;
			if (ptr->IsExpire(error_info)) {
				args.GetIsolate()->ThrowException(
					v8::String::NewFromUtf8(args.GetIsolate(), error_info.c_str(),
					v8::NewStringType::kNormal).ToLocalChecked());
			} 
		} 

	}

	void V8Contract::CallBackLog(const v8::FunctionCallbackInfo<v8::Value>& args) {
		if (args.Length() < 1) {
			args.GetReturnValue().Set(false);
			return;
		}

		v8::HandleScope scope(args.GetIsolate());
		V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
		if (!v8_contract || !v8_contract->parameter_.ledger_context_) {
			LOG_TRACE("Failed to find contract object by isolate id");
			return;
		}
		std::string this_contract = v8_contract->parameter_.this_address_;
		v8::String::Utf8Value str1(args[0]);
		const char* cstr = ToCString(str1);
		LOG_TRACE("V8contract log[%s:%s]\n%s", this_contract.c_str(), v8_contract->parameter_.sender_.c_str(), cstr);
		//v8_contract->AddLog(cstr);

		return;
	}

	void V8Contract::CallBackTopicLog(const v8::FunctionCallbackInfo<v8::Value>& args) {
		std::string error_desc;
		do {
			if (args.Length() < 2 || args.Length() > 6) {
				error_desc = "tlog parameter number error";
				break;
			}

			if (!args[0]->IsString()) { 
				error_desc = "tlog parameter 0 should be a String";
				break;
			}

			for (int i = 1; i < args.Length();i++) {
				if (!(args[i]->IsString() || args[i]->IsNumber() || args[i]->IsBoolean())) {
					error_desc = utils::String::Format("tlog parameter %d should be a String , Number or Boolean", i);
					break;
				}
			}

			if (!error_desc.empty() ) {
				break;
			} 

			std::string topic = ToCString(v8::String::Utf8Value(args[0]));

			v8::HandleScope scope(args.GetIsolate());
			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			if (!v8_contract || !v8_contract->parameter_.ledger_context_) {
				error_desc = "tlog couldn't find contract object by isolate id";
				break;
			}
			LedgerContext *ledger_context = v8_contract->parameter_.ledger_context_;
			ledger_context->GetBottomTx()->ContractStepInc(100);
			std::string this_contract = v8_contract->parameter_.this_address_;

			//Add to transaction
			protocol::TransactionEnv txenv;
			txenv.mutable_transaction()->set_source_address(this_contract);
			protocol::Operation *ope = txenv.mutable_transaction()->add_operations();

			ope->set_type(protocol::Operation_Type_LOG);
			ope->mutable_log()->set_topic(topic);
			for (int i = 1; i < args.Length(); i++) {
				std::string data;
				if (args[i]->IsString()) {
					data = ToCString(v8::String::Utf8Value(args[i]));
				}
				else {
					data = ToCString(v8::String::Utf8Value(args[i]->ToString()));
				}
				*ope->mutable_log()->add_datas() = data;
			}

			Result tmp_result = LedgerManager::Instance().DoTransaction(txenv, ledger_context);
			if (tmp_result.code() > 0) {
				v8_contract->SetResult(tmp_result);
				error_desc = utils::String::Format("Failed to process transaction(%s)", tmp_result.desc().c_str());
				break;
			}

			args.GetReturnValue().Set(tmp_result.code() == 0);
			return;
		} while (false);

		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	void V8Contract::CallBackGetAccountAsset(const v8::FunctionCallbackInfo<v8::Value>& args) {
		if (args.Length() != 2) {
			LOG_TRACE("parameter error");
			args.GetReturnValue().Set(false);
			return;
		}

		do {
			v8::HandleScope handle_scope(args.GetIsolate());
			if (!args[0]->IsString()) {
				LOG_TRACE("Contract execution error, CallBackGetAccountAsset, parameter 1 should be a String");
				break;
			}
			std::string address = ToCString(v8::String::Utf8Value(args[0]));

			if (!args[1]->IsObject()) {
				LOG_TRACE("Contract execution error, CallBackGetAccountAsset parameter 2 should be a object");
				break;
			}

			protocol::AssetKey asset_key;
			v8::Local<v8::Object> v8_asset_property = args[1]->ToObject();
			v8::Local<v8::Value> v8_issue = v8_asset_property->Get(v8::String::NewFromUtf8(args.GetIsolate(), "issuer"));
			v8::Local<v8::Value> v8_code = v8_asset_property->Get(v8::String::NewFromUtf8(args.GetIsolate(), "code"));
			asset_key.set_issuer(ToCString(v8::String::Utf8Value(v8_issue)));
			asset_key.set_code(ToCString(v8::String::Utf8Value(v8_code)));

			phantom::AccountFrm::pointer account_frm = nullptr;
			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			ledger_context->GetBottomTx()->ContractStepInc(100);

			bool getAccountSucceed = false;
			std::shared_ptr<Environment> environment = ledger_context->GetTopTx()->environment_;
			if (!environment->GetEntry(address, account_frm)) {
				LOG_TRACE("Failed to find account %s.", address.c_str());
				break;
			}
			else {
				getAccountSucceed = true;
			}

			if (!getAccountSucceed) {
				if (!Environment::AccountFromDB(address, account_frm)) {
					LOG_TRACE("Failed to find account %s.", address.c_str());
					break;
				}
			}

			protocol::AssetStore asset;
			if (!account_frm->GetAsset(asset_key, asset)) {
				break;
			}

			args.GetReturnValue().Set(v8::String::NewFromUtf8(args.GetIsolate(), utils::String::ToString(asset.amount()).c_str()));
// 			v8::Local<v8::Object> ret = v8::Object::New(args.GetIsolate());
// 			ret->Set(v8::String::NewFromUtf8(args.GetIsolate(), "amount"), v8::String::NewFromUtf8(args.GetIsolate(), utils::String::ToString(asset.amount()).c_str()));
// 			ret->Set(v8::String::NewFromUtf8(args.GetIsolate(), "property"), v8_asset_property);
//			args.GetReturnValue().Set(ret);
			return;
		} while (false);

		args.GetReturnValue().Set(false);
	}

	void V8Contract::CallBackContractQuery(const v8::FunctionCallbackInfo<v8::Value>& args) {
		v8::HandleScope handle_scope(args.GetIsolate());
		v8::Local<v8::Object> obj = v8::Object::New(args.GetIsolate());
		v8::Local<v8::Boolean> flag_false = v8::Boolean::New(args.GetIsolate(), false);
		obj->Set(v8::String::NewFromUtf8(args.GetIsolate(), "success"), flag_false);

		do {
			if (args.Length() != 2) {
				LOG_TRACE("parameter error");
				args.GetReturnValue().Set(false);
				break;
			}

			if (!args[0]->IsString()) { //the called contract address
				LOG_TRACE("contract execution error,CallBackContractQuery, parameter 0 should be a String");
				break;
			}

			if (!args[1]->IsString()) {
				LOG_TRACE("contract execution error,CallBackContractQuery, parameter 1 should be a String");
				break;
			}

			std::string address = ToCString(v8::String::Utf8Value(args[0]));
			std::string input = ToCString(v8::String::Utf8Value(args[1]));

			phantom::AccountFrm::pointer account_frm = nullptr;
			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			ledger_context->GetBottomTx()->ContractStepInc(1000);

			std::shared_ptr<Environment> environment = ledger_context->GetTopTx()->environment_;
			if (!environment->GetEntry(address, account_frm)) {
				LOG_TRACE("Failed to find account %s.", address.c_str());
				break;
			}
			else {
				if (!Environment::AccountFromDB(address, account_frm)) {
					LOG_TRACE("Failed to find account %s.", address.c_str());
					break;
				}
			}

			if (!account_frm->GetProtoAccount().has_contract()) {
				LOG_TRACE("The account(%s) has no contract.", address.c_str());
				break;
			}

			protocol::Contract contract = account_frm->GetProtoAccount().contract();
			if (contract.payload().size() == 0) {
				LOG_TRACE("The account(%s) has no contract.", address.c_str());
				break;
			}

			ContractParameter parameter;
			parameter.code_ = contract.payload();
			parameter.sender_ = v8_contract->GetParameter().this_address_;
			parameter.this_address_ = address;
			parameter.input_ = input;
			parameter.ope_index_ = 0;
			parameter.timestamp_ = v8_contract->GetParameter().timestamp_;
			parameter.blocknumber_ = v8_contract->GetParameter().blocknumber_;
			parameter.consensus_value_ = v8_contract->GetParameter().consensus_value_;
			parameter.ledger_context_ = v8_contract->GetParameter().ledger_context_;
			//Query

			Json::Value query_result;
			bool ret = ContractManager::Instance().Query(contract.type(), parameter, query_result);
			
			//Just like this, {"success": true, "result": "abcde"}
			if (!ret) {
				v8::Local<v8::Boolean> flag = v8::Boolean::New(args.GetIsolate(), true);
				obj->Set(v8::String::NewFromUtf8(args.GetIsolate(), "error"), flag);
			}
			else {
				Json::Value js_object = query_result["result"];
				v8::Local<v8::Value> v8_result;
				CppJsonToJsValue(args.GetIsolate(), js_object, v8_result);
				obj->Set(v8::String::NewFromUtf8(args.GetIsolate(), "result"), v8_result);
			}


		} while (false);

		args.GetReturnValue().Set(obj);
	}

	void V8Contract::CallBackGetValidators(const v8::FunctionCallbackInfo<v8::Value>& args)
	{
		do {
			if (args.Length() != 0) 
			{
				LOG_TRACE("parameter error");
				args.GetReturnValue().Set(false);
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());
			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());

			Json::Value jsonValidators;
			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			jsonValidators = ledger_context->GetTopTx()->environment_->GetValidators();

			std::string strvalue = jsonValidators.toFastString();
			v8::Local<v8::String> returnvalue = v8::String::NewFromUtf8(args.GetIsolate(), strvalue.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
			args.GetReturnValue().Set(v8::JSON::Parse(returnvalue));

			return;
		} while (false);
		args.GetReturnValue().Set(false);
	}

	void V8Contract::CallBackSetValidators(const v8::FunctionCallbackInfo<v8::Value>& args)
	{
		std::string error_desc;
		do
		{
			if (args.Length() != 1)
			{
				error_desc = "parameter number error";
				break;
			}

			if (!args[0]->IsString()) {
				error_desc = "arg0 should be string";
				break;
			}

			v8::HandleScope handle_scope(args.GetIsolate());
			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			if (!v8_contract || !v8_contract->parameter_.ledger_context_) {
				error_desc = "Can't find contract object by isolate id";
				break;
			}

			if (v8_contract->parameter_.this_address_ != General::CONTRACT_VALIDATOR_ADDRESS)
			{
				error_desc = utils::String::Format("contract(%s) has no permission to call callBackSetValidators interface.", v8_contract->parameter_.this_address_.c_str());
				break;
			}

			v8::String::Utf8Value  utf8(args[0]);
			Json::Value json;
			if (!json.fromCString(ToCString(utf8))) {
				error_desc = "Failed to execute fromCString function, fatal error.";
				break;
			}

			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			ledger_context->GetTopTx()->environment_->UpdateNewValidators(json);
			args.GetReturnValue().Set(true);
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	void V8Contract::CallBackAddressValidCheck(const v8::FunctionCallbackInfo<v8::Value>& args)
	{
		std::string error_desc;
		do
		{
			if (args.Length() != 1){
				error_desc = "parameter number error";
				break;
			}

			if (!args[0]->IsString()) {
				error_desc = "arg0 should be string";
				break;
			}

			v8::HandleScope handle_scope(args.GetIsolate());

			v8::String::Utf8Value utf8(args[0]);
			std::string address = std::string(ToCString(utf8));
			bool ret = PublicKey::IsAddressValid(address);

			args.GetReturnValue().Set(ret);
			return;
		} while (false);

		args.GetReturnValue().Set(false);
	}

	void V8Contract::CallBackPayCoin(const v8::FunctionCallbackInfo<v8::Value>& args) {
		std::string error_desc;
		do {
			if (args.Length() < 2) {
				error_desc ="Parameter number error";
				args.GetReturnValue().Set(false);
				break;
			}

			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString()) {
				error_desc = "Contract execution error, payCoin parameter 0 should be a string";
				break;
			}

			if (!args[1]->IsString()) {
				error_desc = "Contract execution error, payCoin parameter 1 should be a string";
				break;
			}

			std::string input;
			if (args.Length() > 2) {
				input = ToCString(v8::String::Utf8Value(args[2]));
			}

			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			if (!v8_contract || !v8_contract->parameter_.ledger_context_) {
				error_desc = "Failed to find contract object by isolate id";
				break;
			}
			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			ledger_context->GetBottomTx()->ContractStepInc(100);

			if (v8_contract->IsReadonly()) {
				error_desc = "The contract is readonly.";
				break;
			}

			std::string contractor = v8_contract->parameter_.this_address_;

			std::string dest_address = std::string(ToCString(v8::String::Utf8Value(args[0])));
			std::string arg_1 = std::string(ToCString(v8::String::Utf8Value(args[1])));
			int64_t pay_amount = 0;
			if (!utils::String::SafeStoi64(arg_1, pay_amount) || pay_amount < 0){
				error_desc = utils::String::Format("Failed to execute paycoin function in contract, dest_address:%s, amount:%s.", dest_address.c_str(), arg_1.c_str());
				break;
			}

			protocol::TransactionEnv txenv;
			txenv.mutable_transaction()->set_source_address(contractor);
			protocol::Operation *ope = txenv.mutable_transaction()->add_operations();

			ope->set_type(protocol::Operation_Type_PAY_COIN);
			ope->mutable_pay_coin()->set_dest_address(dest_address);
			ope->mutable_pay_coin()->set_amount(pay_amount);
			ope->mutable_pay_coin()->set_input(input);

			Result tmp_result = LedgerManager::Instance().DoTransaction(txenv, ledger_context);
			if (tmp_result.code() > 0) {
				v8_contract->SetResult(tmp_result);
				error_desc = utils::String::Format("Failed to process transaction(%s)", tmp_result.desc().c_str());				
				break;
			}

			args.GetReturnValue().Set(tmp_result.code() == 0);
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	void V8Contract::CallBackIssueAsset(const v8::FunctionCallbackInfo<v8::Value>& args){
		std::string error_desc;
		do {
			if (args.Length() < 2) {
				error_desc = "Parameter number error";
				args.GetReturnValue().Set(false);
				break;
			}

			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString()) {
				error_desc = "Contract execution error, issueAsset parameter 0 should be a string";
				break;
			}

			if (!args[1]->IsString()) {
				error_desc = "Contract execution error, issueAsset parameter 1 should be a string";
				break;
			}

			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			if (!v8_contract || !v8_contract->parameter_.ledger_context_) {
				error_desc = "Failed to find contract object by isolate id";
				break;
			}

			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			ledger_context->GetBottomTx()->ContractStepInc(100);

			if (v8_contract->IsReadonly()) {
				error_desc = "The contract is readonly";
				break;
			}

			std::string contractor = v8_contract->parameter_.this_address_;

			std::string assetCode = std::string(ToCString(v8::String::Utf8Value(args[0])));
			std::string amount = std::string(ToCString(v8::String::Utf8Value(args[1])));
			int64_t issueAmount = 0;
			if (!utils::String::SafeStoi64(amount, issueAmount) || issueAmount < 0){
				error_desc = utils::String::Format("Failed to execute issueAsset function in contract, asset code:%s, asset amount:%s.", assetCode.c_str(), amount.c_str());
				break;
			}

			protocol::TransactionEnv txenv;
			txenv.mutable_transaction()->set_source_address(contractor);
			protocol::Operation *ope = txenv.mutable_transaction()->add_operations();

			ope->set_type(protocol::Operation_Type_ISSUE_ASSET);
			ope->mutable_issue_asset()->set_code(assetCode);
			ope->mutable_issue_asset()->set_amount(issueAmount);

			Result tmp_result = LedgerManager::Instance().DoTransaction(txenv, ledger_context);
			if (tmp_result.code() > 0) {
				v8_contract->SetResult(tmp_result);
				error_desc = utils::String::Format("Failed to process transaction(%s)", tmp_result.desc().c_str());
				break;
			}

			args.GetReturnValue().Set(tmp_result.code() == 0);
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	void V8Contract::CallBackPayAsset(const v8::FunctionCallbackInfo<v8::Value>& args){
		std::string error_desc;
		do {
			if (args.Length() < 4) {
				error_desc = "Parameter number error";
				args.GetReturnValue().Set(false);
				break;
			}

			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString()) {
				error_desc = "Contract execution error, payAsset parameter 0 should be a string";
				break;
			}

			if (!args[1]->IsString()) {
				error_desc = "Contract execution error, payAsset parameter 1 should be a string";
				break;
			}

			if (!args[2]->IsString()) {
				error_desc = "Contract execution error, payAsset parameter 2 should be a string";
				break;
			}

			if (!args[3]->IsString()) {
				error_desc = "Contract execution error, payAsset parameter 3 should be a string";
				break;
			}


			std::string input;
			if (args.Length() > 4) {
				input = ToCString(v8::String::Utf8Value(args[4]));
			}

			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			if (!v8_contract || !v8_contract->parameter_.ledger_context_) {
				error_desc = "Failed to find contract object by isolate id";
				break;
			}
			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			ledger_context->GetBottomTx()->ContractStepInc(100);

			if (v8_contract->IsReadonly()) {
				error_desc = "The contract is readonly";
				break;
			}

			std::string contractor = v8_contract->parameter_.this_address_;

			std::string dest_address = std::string(ToCString(v8::String::Utf8Value(args[0])));
			std::string issuer    = std::string(ToCString(v8::String::Utf8Value(args[1])));
			std::string assetCode = std::string(ToCString(v8::String::Utf8Value(args[2])));
			std::string amount    = std::string(ToCString(v8::String::Utf8Value(args[3])));
			int64_t pay_amount = 0;
			if (!utils::String::SafeStoi64(amount, pay_amount) || pay_amount < 0){
				error_desc = utils::String::Format("Failed to execute payAsset function in contract, dest_address:%s, amount:%s.", dest_address.c_str(), amount.c_str());
				break;
			}

			protocol::TransactionEnv txenv;
			txenv.mutable_transaction()->set_source_address(contractor);
			protocol::Operation *ope = txenv.mutable_transaction()->add_operations();

			ope->set_type(protocol::Operation_Type_PAY_ASSET);

			ope->mutable_pay_asset()->set_dest_address(dest_address);
			ope->mutable_pay_asset()->mutable_asset()->mutable_key()->set_issuer(issuer);
			ope->mutable_pay_asset()->mutable_asset()->mutable_key()->set_code(assetCode);
			ope->mutable_pay_asset()->mutable_asset()->set_amount(pay_amount);
			ope->mutable_pay_asset()->set_input(input);

			Result tmp_result = LedgerManager::Instance().DoTransaction(txenv, ledger_context);
			if (tmp_result.code() > 0) {
				v8_contract->SetResult(tmp_result);
				error_desc = utils::String::Format("Failed to process transaction(%s)", tmp_result.desc().c_str());
				break;
			}

			args.GetReturnValue().Set(tmp_result.code() == 0);
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	//Get the balance of the given account 
	void V8Contract::CallBackGetBalance(const v8::FunctionCallbackInfo<v8::Value>& args) {
		do {
			if (args.Length() != 1) {
				LOG_TRACE("parameter error");
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());
			if (!args[0]->IsString()) {
				LOG_TRACE("contract execution error, parameter 0 should be a string");
				break;
			}

			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			if (!v8_contract || !v8_contract->parameter_.ledger_context_) {
				LOG_TRACE("Failed to find contract object by isolate id");
				break;
			}
			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			ledger_context->GetBottomTx()->ContractStepInc(100);

			std::string address = ToCString(v8::String::Utf8Value(args[0]));
			AccountFrm::pointer account_frm = NULL;

			std::shared_ptr<Environment> environment = ledger_context->GetTopTx()->environment_;
			if (!environment->GetEntry(address, account_frm)) {
				Environment::AccountFromDB(address, account_frm);
			}

			std::string balance = "0";
			if (account_frm) {
				balance = utils::String::ToString(account_frm->GetAccountBalance());
			}

			args.GetReturnValue().Set(v8::String::NewFromUtf8(args.GetIsolate(), balance.c_str(),
				v8::NewStringType::kNormal).ToLocalChecked());
			return;
		} while (false);

		args.GetReturnValue().Set(false);
	}

// 	//Get the hash value of one of the 1024 most recent complete blocks
	void V8Contract::CallBackGetBlockHash(const v8::FunctionCallbackInfo<v8::Value>& args) {
		do {
			if (args.Length() != 1) {
				LOG_TRACE("parameter error");
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());
			if (!args[0]->IsNumber()) {
				LOG_TRACE("contract execution error, parameter 0 should be a string");
				break;
			}
			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			if (!v8_contract || !v8_contract->parameter_.ledger_context_) {
				LOG_TRACE("Failed to find contract object by isolate id");
				break;
			}
			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			ledger_context->GetBottomTx()->ContractStepInc(100);

			protocol::LedgerHeader lcl = LedgerManager::Instance().GetLastClosedLedger();
			int64_t seq = lcl.seq() - (int64_t)args[0]->NumberValue();
			if (seq <= lcl.seq() - 1024 || seq > lcl.seq()) {
				LOG_TRACE("The parameter seq(" FMT_I64 ") <= " FMT_I64 " or > " FMT_I64, seq, lcl.seq() - 1024, lcl.seq());
				break;
			}

			LedgerFrm lfrm;
			if (lfrm.LoadFromDb(seq)) {
				args.GetReturnValue().Set(v8::String::NewFromUtf8(
					args.GetIsolate(), utils::String::BinToHexString(lfrm.GetProtoHeader().hash()).c_str(), v8::NewStringType::kNormal).ToLocalChecked());
			}
			else {
				break;
			}

			return;
		} while (false);
		args.GetReturnValue().Set(false);
	}

// 	//Sends a message with arbitrary date to a given address path
// 	void CallBackCall(const v8::FunctionCallbackInfo<v8::Value>& args);
// 	//Sends a message with arbitrary date to a given address path
	void V8Contract::CallBackStorageStore(const v8::FunctionCallbackInfo<v8::Value>& args) {
		SetMetaData(args);
	}

	void V8Contract::CallBackStorageDel(const v8::FunctionCallbackInfo<v8::Value>& args) {
		SetMetaData(args, true);
	}

	void V8Contract::SetMetaData(const v8::FunctionCallbackInfo<v8::Value>& args, bool is_del) {
		std::string error_desc;
		do {
			uint8_t para_num = 2;
			bool para_type_valid = !args[0]->IsString();
			if (is_del) {
				para_num = 1;
			}
			else {
				para_type_valid = para_type_valid || !args[1]->IsString();
			}

			if (args.Length() != para_num) {
				error_desc = "Parameter number error";
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());

			if (para_type_valid) {
				error_desc = "Storage operation parameter should be string";
				break;
			}

			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			if (!v8_contract || !v8_contract->parameter_.ledger_context_) {
				error_desc = "Failed to find contract object by isolate id";
				break;
			}
			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			ledger_context->GetBottomTx()->ContractStepInc(100);

			if (v8_contract->IsReadonly()) {
				error_desc = "The contract is readonly";
				break;
			}

			std::string contractor = v8_contract->parameter_.this_address_;
			std::string  key = ToCString(v8::String::Utf8Value(args[0]));
			std::string  value = "";
			if (!is_del){
				value = ToCString(v8::String::Utf8Value(args[1]));
			}
			if (key.empty()) {
				error_desc = "Key is empty.";
				break;
			}

			protocol::TransactionEnv txenv;
			txenv.mutable_transaction()->set_source_address(contractor);
			protocol::Operation *ope = txenv.mutable_transaction()->add_operations();

			ope->set_type(protocol::Operation_Type_SET_METADATA);
			protocol::OperationSetMetadata *meta_data = ope->mutable_set_metadata();
			meta_data->set_key(key);
			meta_data->set_value(value);
			meta_data->set_delete_flag(is_del);

			Result tmp_result = LedgerManager::Instance().DoTransaction(txenv, ledger_context);
			if (tmp_result.code() > 0) {
				v8_contract->SetResult(tmp_result);
				error_desc = utils::String::Format("Failed to process transaction(%s)", tmp_result.desc().c_str());				
				break;
			}

			args.GetReturnValue().Set(tmp_result.code() == 0);
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	void V8Contract::CallBackStorageLoad(const v8::FunctionCallbackInfo<v8::Value>& args) {
		do {
			if (args.Length() != 1) {
				LOG_TRACE("parameter error");
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString()) {
				LOG_TRACE("contract execution error,Storage load, parameter 0 should be a String");
				break;
			}

			V8Contract *v8_contract = GetContractFrom(args.GetIsolate());
			if (!v8_contract || !v8_contract->parameter_.ledger_context_) {
				LOG_TRACE("Failed to find contract object by isolate id");
				break;
			}
			LedgerContext *ledger_context = v8_contract->GetParameter().ledger_context_;
			ledger_context->GetTopTx()->ContractStepInc(100);

			std::string key = ToCString(v8::String::Utf8Value(args[0]));
			phantom::AccountFrm::pointer account_frm = nullptr;
			std::shared_ptr<Environment> environment = ledger_context->GetTopTx()->environment_;
			if (!environment->GetEntry(v8_contract->parameter_.this_address_, account_frm)) {
				if (!Environment::AccountFromDB(v8_contract->parameter_.this_address_, account_frm)) {
					LOG_ERROR("Failed to find account %s.", v8_contract->parameter_.this_address_.c_str());
					break;
				}
			}

			protocol::KeyPair kp;
			if (!account_frm->GetMetaData(key, kp)) {
				break;
			}
			args.GetReturnValue().Set(v8::String::NewFromUtf8(
				args.GetIsolate(), kp.value().c_str(), v8::NewStringType::kNormal).ToLocalChecked());
			return;
		} while (false);
		args.GetReturnValue().Set(false);
	}

	//str to int64 check
	void V8Contract::CallBackStoI64Check(const v8::FunctionCallbackInfo<v8::Value>& args){
		std::string error_desc;
		do {
			if (args.Length() != 1) {
				error_desc = "Parameter number error";
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString()) {
				error_desc = "Contract execution error, stoI64Check, parameter 0 should be a String or Number";
				break;
			}

			std::string arg0 = ToCString(v8::String::Utf8Value(args[0]));

			int64_t iarg0 = 0;
			if (!utils::String::SafeStoi64(arg0, iarg0)){
				error_desc = "Contract execution error, stoI64Check, parameter 0 illegal, maybe exceed the limit value of int64.";
				break;
			}

			args.GetReturnValue().Set(true);
			return;
		} while (false);
		args.GetReturnValue().Set(false);
	}

// 	//Int64 add
	void V8Contract::CallBackInt64Add(const v8::FunctionCallbackInfo<v8::Value>& args) {
		std::string error_desc;
		do {
			if (args.Length() != 2) {
				error_desc = "Parameter number error";
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString() && !args[0]->IsNumber()) {
				error_desc = "Contract execution error, int64Add, parameter 0 should be a String or Number";
				break;
			}
			if (!args[1]->IsString() && !args[1]->IsNumber()) {
				error_desc = "Contract execution error, int64Add, parameter 1 should be a String or Number";
				break;
			}

			std::string arg0 = ToCString(v8::String::Utf8Value(args[0]));
			std::string arg1 = ToCString(v8::String::Utf8Value(args[1]));
			int64_t iarg0 = 0;
			int64_t iarg1 = 0;

			if (!utils::String::SafeStoi64(arg0, iarg0)){
				error_desc = "Contract execution error, int64Add, parameter 0 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (!utils::String::SafeStoi64(arg1, iarg1)){
				error_desc = "Contract execution error, int64Add, parameter 1 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (!utils::SafeIntAdd(iarg0, iarg1, iarg0)){
				error_desc = "Contract execution error, int64Add, parameter 0 + parameter 1 overflowed";
				break;
			}

			args.GetReturnValue().Set(v8::String::NewFromUtf8(
				args.GetIsolate(), utils::String::ToString(iarg0).c_str(), v8::NewStringType::kNormal).ToLocalChecked());
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	//Int64 sub
	void V8Contract::CallBackInt64Sub(const v8::FunctionCallbackInfo<v8::Value>& args) {
		std::string error_desc;
		do {
			if (args.Length() != 2) {
				error_desc ="Parameter number error";
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString() && !args[0]->IsNumber()) {
				error_desc  ="Contract execution error, int64Sub, parameter 0 should be a String or Number";
				break;
			}
			if (!args[1]->IsString() && !args[1]->IsNumber()) {
				error_desc ="Contract execution error, int64Sub, parameter 1 should be a String or Number";
				break;
			}

			std::string arg0 = ToCString(v8::String::Utf8Value(args[0]));
			std::string arg1 = ToCString(v8::String::Utf8Value(args[1]));
			int64_t iarg0 = 0;
			int64_t iarg1 = 0;

			if (!utils::String::SafeStoi64(arg0, iarg0)){
				error_desc = "Contract execution error, int64Sub, parameter 0 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (!utils::String::SafeStoi64(arg1, iarg1)){
				error_desc = "Contract execution error, int64Sub, parameter 1 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (!utils::SafeIntSub(iarg0, iarg1, iarg0)){
				error_desc = "Contract execution error, int64Sub, parameter 0 - parameter 1 overflowed";
				break;
			}

			args.GetReturnValue().Set(v8::String::NewFromUtf8(
				args.GetIsolate(), utils::String::ToString(iarg0).c_str(), v8::NewStringType::kNormal).ToLocalChecked());
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

// 	//Int64 compare
	void V8Contract::CallBackInt64Compare(const v8::FunctionCallbackInfo<v8::Value>& args) {
		std::string error_desc;
		do {
			if (args.Length() != 2) {
				error_desc = " Parameter number error";
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString() && !args[0]->IsNumber()) {
				error_desc  = "Contract execution error, int64Compare, parameter 0 should be a String or Number";
				break;
			}
			if (!args[1]->IsString() && !args[1]->IsNumber()) {
				error_desc = "Contract execution error, int64Compare, parameter 1 should be a String or Number";
				break;
			}

			std::string arg0 = ToCString(v8::String::Utf8Value(args[0]));
			std::string arg1 = ToCString(v8::String::Utf8Value(args[1]));
			int64_t iarg0 = 0;
			int64_t iarg1 = 0;

			if (!utils::String::SafeStoi64(arg0, iarg0)){
				error_desc = "Contract execution error, int64Compare, parameter 0 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (!utils::String::SafeStoi64(arg1, iarg1)){
				error_desc = "Contract execution error, int64Compare, parameter 1 illegal, maybe exceed the limit value of int64.";
				break;
			}

			int32_t compare1 = 0;
			if (iarg0 > iarg1) compare1 = 1;
			else if (iarg0 == iarg1) {
				compare1 = 0;
			} else{
				compare1 = -1;
			}

			args.GetReturnValue().Set(v8::Int32::New(args.GetIsolate(), compare1));
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	void V8Contract::CallBackInt64Div(const v8::FunctionCallbackInfo<v8::Value>& args) {
		std::string error_desc;
		do {
			if (args.Length() != 2) {
				error_desc = "Parameter number error";
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString() && !args[0]->IsNumber()) {
				error_desc = "Contract execution error, int64Div, parameter 0 should be a String or Number";
				break;
			}
			if (!args[1]->IsString() && !args[1]->IsNumber()) {
				error_desc = "Contract execution error, int64Div, parameter 1 should be a String or Number";
				break;
			}

			std::string arg0 = ToCString(v8::String::Utf8Value(args[0]));
			std::string arg1 = ToCString(v8::String::Utf8Value(args[1]));
			int64_t iarg0 = 0;
			int64_t iarg1 = 0;

			if (!utils::String::SafeStoi64(arg0, iarg0)){
				error_desc = "Contract execution error, int64Div, parameter 0 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (!utils::String::SafeStoi64(arg1, iarg1)){
				error_desc = "Contract execution error, int64Div, parameter 1 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (iarg1 <= 0 || iarg0 < 0) {
				error_desc = "Parameter arg <= 0";
				break;
			}

			args.GetReturnValue().Set(v8::String::NewFromUtf8(
				args.GetIsolate(), utils::String::ToString(iarg0 / iarg1).c_str(), v8::NewStringType::kNormal).ToLocalChecked());
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}
	//Int64 mod
	void V8Contract::CallBackInt64Mod(const v8::FunctionCallbackInfo<v8::Value>& args) {
		std::string error_desc;
		do {
			if (args.Length() != 2) {
				error_desc = "Parameter number error";
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString() && !args[0]->IsNumber()) {
				error_desc = "Contract execution error, int64Mod, parameter 0 should be a String or Number";
				break;
			}
			if (!args[1]->IsString() && !args[1]->IsNumber()) {
				error_desc = "Contract execution error, int64Mod, parameter 1 should be a String or Number";
				break;
			}

			std::string arg0 = ToCString(v8::String::Utf8Value(args[0]));
			std::string arg1 = ToCString(v8::String::Utf8Value(args[1]));
			int64_t iarg0 = 0;
			int64_t iarg1 = 0;

			if (!utils::String::SafeStoi64(arg0, iarg0)){
				error_desc = "Contract execution error, int64Mod, parameter 0 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (!utils::String::SafeStoi64(arg1, iarg1)){
				error_desc = "Contract execution error, int64Mod, parameter 1 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (iarg1 <= 0 || iarg0 < 0) {
				error_desc  ="Parameter arg <= 0";
				break;
			}

			args.GetReturnValue().Set(v8::String::NewFromUtf8(
				args.GetIsolate(), utils::String::ToString(iarg0 % iarg1).c_str(), v8::NewStringType::kNormal).ToLocalChecked());
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	//Int64 mod
	void V8Contract::CallBackInt64Mul(const v8::FunctionCallbackInfo<v8::Value>& args) {
		std::string error_desc;
		do {
			if (args.Length() != 2) {
				error_desc = "Parameter number error";
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString() && !args[0]->IsNumber()) {
				error_desc ="Contract execution error, int64Mul, parameter 0 should be a String or Number";
				break;
			}
			if (!args[1]->IsString() && !args[1]->IsNumber()) {
				error_desc = "Contract execution error, int64Mul, parameter 1 should be a String or Number";
				break;
			}

			std::string arg0 = ToCString(v8::String::Utf8Value(args[0]));
			std::string arg1 = ToCString(v8::String::Utf8Value(args[1]));
			int64_t iarg0 = 0;
			int64_t iarg1 = 0;

			if (!utils::String::SafeStoi64(arg0, iarg0)){
				error_desc = "Contract execution error, int64Mul, parameter 0 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (!utils::String::SafeStoi64(arg1, iarg1)){
				error_desc = "Contract execution error, int64Mul, parameter 1 illegal, maybe exceed the limit value of int64.";
				break;
			}

			if (!utils::SafeIntMul(iarg0, iarg1, iarg0)){
				error_desc = "Contract execution error, int64Mul, parameter 0 * parameter 1 overflowed";
				break;
			}

			args.GetReturnValue().Set(v8::String::NewFromUtf8(
				args.GetIsolate(), utils::String::ToString(iarg0).c_str(), v8::NewStringType::kNormal).ToLocalChecked());
			return;
		} while (false);
		LOG_ERROR("%s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	void V8Contract::CallBackToBaseUnit(const v8::FunctionCallbackInfo<v8::Value>& args) {
		std::string error_desc;
		do {
			if (args.Length() != 1) {
				error_desc = utils::String::Format("Parameter number error:%d", args.Length());
				break;
			}
			v8::HandleScope handle_scope(args.GetIsolate());

			if (!args[0]->IsString()) {
				error_desc = "contract execution error, toBaseUnit, parameter 0 should be a String";
				break;
			}

			std::string arg0 = ToCString(v8::String::Utf8Value(args[0]));
			if (!utils::String::IsDecNumber(arg0, General::BU_DECIMALS)) {
				error_desc = utils::String::Format("Not a decimal number:%s", arg0.c_str());
				break;
			} 

			std::string multi_result = utils::String::MultiplyDecimal(arg0, General::BU_DECIMALS).c_str();
			int64_t multi_i64 = 0;
			if (!utils::String::SafeStoi64(multi_result, multi_i64)){
				error_desc = utils::String::Format("CallBackToBaseUnit error, int64 overflow:%s", multi_result.c_str());
				break;
			}

			args.GetReturnValue().Set(v8::String::NewFromUtf8(
				args.GetIsolate(), multi_result.c_str(), v8::NewStringType::kNormal).ToLocalChecked());
			return;
		} while (false);
		LOG_ERROR("Failed to convert string to base unit, %s", error_desc.c_str());
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), error_desc.c_str(),
			v8::NewStringType::kNormal).ToLocalChecked());
	}

	QueryContract::QueryContract():contract_(NULL){}
	QueryContract::~QueryContract() {
	}
	bool QueryContract::Init(int32_t type, const ContractParameter &paramter) {
		parameter_ = paramter;
		if (type == Contract::TYPE_V8) {
			
		}
		else {
			std::string error_msg = utils::String::Format("Contract type(%d) not supported", type);
			LOG_ERROR("%s", error_msg.c_str());
			return false;
		}
		return true;
	}

	void QueryContract::Cancel() {
		utils::MutexGuard guard(mutex_);
		if (contract_) {
			contract_->Cancel();
		} 
	}

	bool QueryContract::GetResult(Json::Value &result) {
		result = result_;
		return ret_;
	}

	void QueryContract::Run() {
		do {
			utils::MutexGuard guard(mutex_);
			contract_ = new V8Contract(true, parameter_);
		} while (false);

		ret_ = contract_->Query(result_);

		do {
			utils::MutexGuard guard(mutex_);
			delete contract_;
			contract_ = NULL;
		} while (false);
	}

	ContractManager::ContractManager() {}
	ContractManager::~ContractManager() {}

	bool ContractManager::Initialize(int argc, char** argv) {
		V8Contract::Initialize(argc, argv);
		return true;
	}

	bool ContractManager::Exit() {
		return true;
	}

	Result ContractManager::SourceCodeCheck(int32_t type, const std::string &code) {
		ContractParameter parameter;
		parameter.code_ = code;
		Contract *contract = NULL;
		Result tmp_result;
		if (type == Contract::TYPE_V8) {
			contract = new V8Contract(false, parameter);
		}
		else {
			tmp_result.set_code(protocol::ERRCODE_CONTRACT_SYNTAX_ERROR);
			tmp_result.set_desc(utils::String::Format("Contract type(%d) not supported", type));
			LOG_ERROR("%s", tmp_result.desc().c_str());
			return tmp_result;
		}

		bool ret = contract->SourceCodeCheck();
		tmp_result = contract->GetResult();
		delete contract;
		return tmp_result;
	}

	Result ContractManager::Execute(int32_t type, const ContractParameter &paramter, bool init_execute) {
		Result ret;
		do {
			Contract *contract;
			if (type == Contract::TYPE_V8) {
				utils::MutexGuard guard(contracts_lock_);
				contract = new V8Contract(false, paramter);
				//paramter->ledger_context_ 
				//Add the contract id. Use this ID when cancelling the contract in the future. 

				contracts_[contract->GetId()] = contract;
			}
			else {
				ret.set_code(protocol::ERRCODE_CONTRACT_EXECUTE_FAIL);
				LOG_ERROR("Contract type(%d) not supported", type);
				break;
			}

			LedgerContext *ledger_context = contract->GetParameter().ledger_context_;
			ledger_context->PushContractId(contract->GetId());
			if (init_execute)
				contract->InitContract();
			else
				contract->Execute();
			ret = contract->GetResult();
			ledger_context->PopContractId();
			ledger_context->PushLog(contract->GetParameter().this_address_, contract->GetLogs());
			do {
				//Delete the contract from map
				contracts_.erase(contract->GetId());
				delete contract;
			} while (false);

		} while (false);
		return ret;
	}

	bool ContractManager::Query(int32_t type, const ContractParameter &paramter, Json::Value &result) {
		do {
			Contract *contract;
			if (type == Contract::TYPE_V8) {
				utils::MutexGuard guard(contracts_lock_);
				contract = new V8Contract(true, paramter);
				//paramter->ledger_context_ 
				//Add the contract id. Use this ID when cancelling the contract in the future.

				contracts_[contract->GetId()] = contract;
			}
			else {
				LOG_ERROR("Contract type(%d) not supported", type);
				break;
			}

			LedgerContext *ledger_context = contract->GetParameter().ledger_context_;
			ledger_context->PushContractId(contract->GetId());
			bool ret = contract->Query(result);
			ledger_context->PopContractId();
			ledger_context->PushLog(contract->GetParameter().this_address_, contract->GetLogs());
			ledger_context->PushRet(contract->GetParameter().this_address_, result);
			do {
				//Delete the contract from map
				contracts_.erase(contract->GetId());
				delete contract;
			} while (false);

			return ret;
		} while (false);
		return false;
	}

	bool ContractManager::Cancel(int64_t contract_id) {
		//Start another thread to delete the contract sandbox after running the contract.
		Contract *contract = NULL;
		do {
			utils::MutexGuard guard(contracts_lock_);
			ContractMap::iterator iter = contracts_.find(contract_id);
			if (iter!= contracts_.end()) {
				contract = iter->second;
			} 
		} while (false);

		if (contract){
			contract->Cancel();
		} 

		return true;
	}

	Contract *ContractManager::GetContract(int64_t contract_id) {
		do {
			utils::MutexGuard guard(contracts_lock_);
			ContractMap::iterator iter = contracts_.find(contract_id);
			if (iter != contracts_.end()) {
				return iter->second;
			}
		} while (false);
		return NULL;
	}

}
