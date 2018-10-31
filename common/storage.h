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

#ifndef STORAGE_H_
#define STORAGE_H_

#include <unordered_map>
#include <utils/headers.h>
#include <json/json.h>
#include "general.h"
#include "configure_base.h"
#ifdef WIN32
#include <leveldb/leveldb.h>
#else
#include <rocksdb/db.h>
#endif

namespace phantom {
#ifdef WIN32
#define KVDB leveldb
#define WRITE_BATCH leveldb::WriteBatch
#define WRITE_BATCH_DATA(batch) (((std::string*)(&batch))->c_str())
#define WRITE_BATCH_DATA_SIZE(batch) (((std::string*)(&batch))->size())
#define SLICE       leveldb::Slice
#else 
#define KVDB rocksdb
#define WRITE_BATCH rocksdb::WriteBatch
#define WRITE_BATCH_DATA(batch) (batch.Data().c_str())
#define WRITE_BATCH_DATA_SIZE(batch) (batch.GetDataSize())
#define SLICE       rocksdb::Slice
#endif

	class KeyValueDb {
	protected:
		utils::Mutex mutex_;
		std::string error_desc_;
	public:
		KeyValueDb();
		~KeyValueDb();
		virtual bool Open(const std::string &db_path, int max_open_files) = 0;
		virtual bool Close() = 0;
		virtual int32_t Get(const std::string &key, std::string &value) = 0;
		virtual bool Put(const std::string &key, const std::string &value) = 0;
		virtual bool Delete(const std::string &key) = 0;
		virtual bool GetOptions(Json::Value &options) = 0;
		std::string error_desc() {
			return error_desc_;
		}
		virtual bool WriteBatch(WRITE_BATCH &values) = 0;

		virtual void* NewIterator() = 0;
	};

#ifdef WIN32
	class LevelDbDriver : public KeyValueDb {
	private:
		leveldb::DB* db_;

	public:
		LevelDbDriver();
		~LevelDbDriver();

		bool Open(const std::string &db_path, int max_open_files);
		bool Close();
		int32_t Get(const std::string &key, std::string &value);
		bool Put(const std::string &key, const std::string &value);
		bool Delete(const std::string &key);
		bool GetOptions(Json::Value &options);
		bool WriteBatch(WRITE_BATCH &values);

		void* NewIterator();
	};
#else
	class RocksDbDriver : public KeyValueDb {
	private:
		rocksdb::DB* db_;

	public:
		RocksDbDriver();
		~RocksDbDriver();

		bool Open(const std::string &db_path, int max_open_files);
		bool Close();
		int32_t Get(const std::string &key, std::string &value);
		bool Put(const std::string &key, const std::string &value);
		bool Delete(const std::string &key);
		bool GetOptions(Json::Value &options);
		bool WriteBatch(WRITE_BATCH &values);

		void* NewIterator();
	};
#endif

	class Storage : public utils::Singleton<phantom::Storage>, public TimerNotify {
		friend class utils::Singleton<Storage>;
	private:
		Storage();
		~Storage();

		KeyValueDb *keyvalue_db_;
		KeyValueDb *ledger_db_;
		KeyValueDb *account_db_;

		bool CloseDb();
		bool DescribeTable(const std::string &name, const std::string &sql_create_table);
		bool ManualDescribeTables();

		KeyValueDb *NewKeyValueDb(const DbConfigure &db_config);
	public:
		bool Initialize(const DbConfigure &db_config, bool bdropdb);
		bool Exit();

		KeyValueDb *keyvalue_db();   //Store other data except account, legder and transaction.
		KeyValueDb *account_db();   //Store account tree.
		KeyValueDb *ledger_db();    //Store transactions and ledgers.

		//Lock the account db and ledger db to make the databases in synchronization.
		utils::ReadWriteLock account_ledger_lock_;

		virtual void OnTimer(int64_t current_time) {};
		virtual void OnSlowTimer(int64_t current_time);
	};
}

#endif
