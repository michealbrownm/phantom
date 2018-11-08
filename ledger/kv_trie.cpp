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

#include "kv_trie.h"

namespace phantom{

	KVTrie::KVTrie(){
		//leafcount_ = 0;
	}

	KVTrie::~KVTrie(){
		FreeMemory(0);
	}


	bool KVTrie::Init(phantom::KeyValueDb* db, std::shared_ptr<WRITE_BATCH> batch, const std::string& prefix, int depth){
		mdb_ = db;
		prefix_ = prefix;
		batch_ = batch;
		Location location;
		location.push_back(0);
		root_ = std::make_shared< NodeFrm>(location);

		protocol::Node info;
		if (storage_load(location, info)){
			root_->info_.CopyFrom(info);
			Load(root_, depth);
		}
		return true;
	}

	void KVTrie::Load(NodeFrm::POINTER node, int depth){
		if (depth < 0){
			return;
		}
		for (int i = 0; i < 16; i++){
			NodeFrm::POINTER child = ChildMayFromDB(node, i);
			if (child != nullptr){
				Load(child, depth - 1);
			}
		}
	}

	//int KVTrie::LeafCount(){
	//	return 0;
	//}

	bool KVTrie::AddToDB(){
		bool b = mdb_->WriteBatch(*batch_);
		batch_->Clear();
		return b;
	}

	void KVTrie::StorageSaveNode(NodeFrm::POINTER node) {
		std::string buff = node->info_.SerializeAsString();
		std::string key = Location2DBkey(node->location_, false);
		batch_->Put(key, buff);
		//LOG_DEBUG("save INNER(%s)", utils::String::BinToHexString(key).c_str());
	}

	void  KVTrie::StorageSaveLeaf(NodeFrm::POINTER node){
		std::string key = Location2DBkey(node->location_, true);
		batch_->Put(key, *node->leaf_);
		//LOG_DEBUG("save LEAF(%s)", utils::String::BinToHexString(key).c_str());
	}

	bool KVTrie::storage_load(const Location& location, protocol::Node& info)  {
		int64_t t1 = utils::Timestamp::HighResolution();
		std::string key = Location2DBkey(location, false);
		std::string buff;
		//LOG_DEBUG("LOAD INNER:%s", utils::String::BinToHexString(key).c_str());
		int32_t stat = mdb_->Get(key, buff);
		int64_t t2 = utils::Timestamp::HighResolution();

		time_ += (t2 - t1);

		if (stat == 1){
			info.ParseFromString(buff);
			return true;
		}
		else if (stat == 0)
			return false;
		else{
			PROCESS_EXIT("Failed to read database. %s", mdb_->error_desc().c_str());
		}
	}

	void KVTrie::StorageDeleteNode(NodeFrm::POINTER node) {
		std::string key = Location2DBkey(node->location_, false);
		//LOG_DEBUG("DELETE INNER %s", utils::String::BinToHexString(key).c_str());
		batch_->Delete(key);
	}

	void KVTrie::StorageDeleteLeaf(NodeFrm::POINTER node){
		std::string key = Location2DBkey(node->location_, true);
		//LOG_DEBUG("DELETE LEAF %s", utils::String::BinToHexString(key).c_str());
		batch_->Delete(key);
	}

	bool KVTrie::StorageGetLeaf(const Location& location, std::string& value) {
		std::string key = Location2DBkey(location, true);
		//LOG_DEBUG("GET LEAF %s", utils::String::BinToHexString(key).c_str());
		int32_t stat = mdb_->Get(key, value);
		if (stat == 1){
			return true;
		}
		else if (stat == 0){
			return false;
		}
		else{
			PROCESS_EXIT("Failed to read storage. %s", mdb_->error_desc().c_str());
		}
	}

	std::string KVTrie::HashCrypto(const std::string& input){
		return HashWrapper::Crypto(input);
	}

	std::string KVTrie::Location2DBkey(const Location& location, bool leaf){
		std::string key = location;
		if (leaf){
			key[0] = LEAF_PREFIX;
		}
		return prefix_ + key;
	}
}

