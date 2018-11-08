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

#ifndef KV_TRIE_H_
#define KV_TRIE_H_

#include <common/storage.h>
#include "trie.h"

namespace phantom{

	class KVTrie :public Trie{
		KeyValueDb* mdb_;
		std::string prefix_;
	public:
		std::shared_ptr<WRITE_BATCH> batch_;
		int64_t time_;
	public:
		KVTrie();
		~KVTrie();
		bool Init(phantom::KeyValueDb* db, std::shared_ptr<WRITE_BATCH>, const std::string& prefix, int depth);

		//int LeafCount();
		bool AddToDB();
	private:
		void Load(NodeFrm::POINTER node, int depth);
	    std::string Location2DBkey(const Location& location, bool leaf);
	protected:
		virtual void StorageSaveNode(NodeFrm::POINTER node) override;
		virtual void StorageSaveLeaf(NodeFrm::POINTER node) override;
		
		virtual void StorageDeleteNode(NodeFrm::POINTER node) override;
		virtual void StorageDeleteLeaf(NodeFrm::POINTER node) override;

		virtual bool storage_load(const Location& location, protocol::Node& info) override;
		virtual bool StorageGetLeaf(const Location& location, std::string& value)override;
		virtual std::string HashCrypto(const std::string& input) override;
	};
}

#endif
