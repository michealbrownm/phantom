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

#include <utils/file.h>
#include <utils/logger.h>
#include "general.h"
#include "configure_base.h"

namespace phantom {
	DbConfigure::DbConfigure() {
		keyvalue_db_path_ = General::DEFAULT_KEYVALUE_DB_PATH;
		ledger_db_path_ = General::DEFAULT_LEDGER_DB_PATH;
		account_db_path_ = General::DEFAULT_ACCOUNT_DB_PATH;
		tmp_path_ = "tmp";
		async_write_sql_ = false; //default sync write sql
		async_write_kv_ = false; //default sync write kv
	}

	DbConfigure::~DbConfigure() {}

	bool DbConfigure::Load(const Json::Value &value) {
		ConfigureBase::GetValue(value, "keyvalue_path", keyvalue_db_path_);
		ConfigureBase::GetValue(value, "ledger_path", ledger_db_path_);
		ConfigureBase::GetValue(value, "account_path", account_db_path_);
		
		ConfigureBase::GetValue(value, "rational_string", rational_string_);
		ConfigureBase::GetValue(value, "rational_db_type", rational_db_type_);
		ConfigureBase::GetValue(value, "tmp_path", tmp_path_);
		ConfigureBase::GetValue(value, "async_write_sql", async_write_sql_);
		ConfigureBase::GetValue(value, "async_write_kv", async_write_kv_);


		std::string rational_decode;
		std::vector<std::string> nparas = utils::String::split(rational_string_, " ");
		for (std::size_t i = 0; i < nparas.size(); i++) {
			std::string str = nparas[i];
			std::vector<std::string> n = utils::String::split(str, "=");

			if (n.size() == 2 && n[0] == "password") {
				n[1] = utils::Aes::HexDecrypto(n[1], GetDataSecuretKey());
				str = utils::String::Format("%s=%s", n[0].c_str(), n[1].c_str());
			}
			rational_decode += " ";
			rational_decode += str;
		}
		rational_string_ = rational_decode;

		if (!utils::File::IsAbsolute(keyvalue_db_path_)) {
			keyvalue_db_path_ = utils::String::Format("%s/%s", utils::File::GetBinHome().c_str(), keyvalue_db_path_.c_str());
		}

		if (!utils::File::IsAbsolute(ledger_db_path_)) {
			ledger_db_path_ = utils::String::Format("%s/%s", utils::File::GetBinHome().c_str(), ledger_db_path_.c_str());
		}

		if (!utils::File::IsAbsolute(account_db_path_)) {
			account_db_path_ = utils::String::Format("%s/%s", utils::File::GetBinHome().c_str(), account_db_path_.c_str());
		}

		if (!utils::File::IsAbsolute(tmp_path_)) {
			tmp_path_ = utils::String::Format("%s/%s", utils::File::GetBinHome().c_str(), tmp_path_.c_str());
		}
		return true;
	}

	LoggerConfigure::LoggerConfigure() {
		path_ = General::LOGGER_FILE;
		dest_ = utils::LOG_DEST_OUT | utils::LOG_DEST_FILE;
		level_ = utils::LOG_LEVEL_ALL;
		time_capacity_ = 30;
		size_capacity_ = 100;
		expire_days_ = 10;
	}

	LoggerConfigure::~LoggerConfigure() {}

	bool LoggerConfigure::Load(const Json::Value &value) {
		ConfigureBase::GetValue(value, "path", path_);
		ConfigureBase::GetValue(value, "dest", dest_str_);
		ConfigureBase::GetValue(value, "level", level_str_);
		ConfigureBase::GetValue(value, "time_capacity", time_capacity_);
		ConfigureBase::GetValue(value, "size_capacity", size_capacity_);
		ConfigureBase::GetValue(value, "expire_days", expire_days_);

		time_capacity_ *= (3600 * 24);
		size_capacity_ *= utils::BYTES_PER_MEGA;

		// parse type string
		utils::StringVector dests, levels;
		dest_ = utils::LOG_DEST_NONE;
		dests = utils::String::Strtok(dest_str_, '|');

		for (auto &dest : dests) {
			std::string destitem = utils::String::ToUpper(dest);

			if (destitem == "ALL")         dest_ = utils::LOG_DEST_ALL;
			else if (destitem == "STDOUT") dest_ |= utils::LOG_DEST_OUT;
			else if (destitem == "STDERR") dest_ |= utils::LOG_DEST_ERR;
			else if (destitem == "FILE")   dest_ |= utils::LOG_DEST_FILE;
		}

		// parse level string
		level_ = utils::LOG_LEVEL_NONE;
		levels = utils::String::Strtok(level_str_, '|');

		for (auto &level : levels) {
			std::string levelitem = utils::String::ToUpper(level);

			if (levelitem == "ALL")          level_ = utils::LOG_LEVEL_ALL;
			else if (levelitem == "TRACE")   level_ |= utils::LOG_LEVEL_TRACE;
			else if (levelitem == "DEBUG")   level_ |= utils::LOG_LEVEL_DEBUG;
			else if (levelitem == "INFO")    level_ |= utils::LOG_LEVEL_INFO;
			else if (levelitem == "WARNING") level_ |= utils::LOG_LEVEL_WARN;
			else if (levelitem == "ERROR")   level_ |= utils::LOG_LEVEL_ERROR;
			else if (levelitem == "FATAL")   level_ |= utils::LOG_LEVEL_FATAL;
		}

		return true;
	}

	SSLConfigure::SSLConfigure() {}

	SSLConfigure::~SSLConfigure() {}

	bool SSLConfigure::Load(const Json::Value &value) {
		ConfigureBase::GetValue(value, "chain_file", chain_file_);
		ConfigureBase::GetValue(value, "private_key_file", private_key_file_);
		ConfigureBase::GetValue(value, "private_password", private_password_);
		ConfigureBase::GetValue(value, "dhparam_file", dhparam_file_);
		ConfigureBase::GetValue(value, "verify_file", verify_file_);

		private_password_ = utils::Aes::HexDecrypto(private_password_, phantom::GetDataSecuretKey());
		return true;
	}

	ConfigureBase::ConfigureBase() {}
	ConfigureBase::~ConfigureBase() {}

	void ConfigureBase::GetValue(const Json::Value &object, const std::string &key, std::string &value) {
		if (object.isMember(key)) {
			value = object[key].asString();
		}
	}

	void ConfigureBase::GetValue(const Json::Value &object, const std::string &key, uint32_t &value) {
		if (object.isMember(key)) {
			value = object[key].asUInt();
		}
	}

	void ConfigureBase::GetValue(const Json::Value &object, const std::string &key, int32_t &value) {
		if (object.isMember(key)) {
			value = object[key].asInt();
		}
	}

	void ConfigureBase::GetValue(const Json::Value &object, const std::string &key, int64_t &value) {
		if (object.isMember(key)) {
			value = object[key].asInt64();
		}
	}

	void ConfigureBase::GetValue(const Json::Value &object, const std::string &key, utils::StringList &list) {
		if (object.isMember(key)) {
			const Json::Value &array_value = object[key];
			for (size_t i = 0; i < array_value.size(); i++) {
				list.push_back(array_value[i].asString());
			}
		}
	}

	void ConfigureBase::GetValue(const Json::Value &object, const std::string &key, bool &value) {
		if (object.isMember(key)) {
			value = object[key].asBool();
		}
	}

	bool ConfigureBase::Load(const std::string &config_file_path) {
		do {
			utils::File config_file;
			if (!config_file.Open(config_file_path, utils::File::FILE_M_READ)) {
				break;
			}

			std::string data;
			config_file.ReadData(data, utils::BYTES_PER_MEGA);

			Json::Reader reader;
			Json::Value values;
			if (!reader.parse(data, values)) {
				LOG_STD_ERR("Parse config file failed, (%s)", reader.getFormatedErrorMessages().c_str());
				break;
			}

			return LoadFromJson(values);
		} while (false);

		return false;
	}
}