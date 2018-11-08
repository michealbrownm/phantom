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

#ifndef TRIE_H_
#define TRIE_H_

#include <utils/sm3.h>
#include "proto/cpp/merkeltrie.pb.h"

namespace phantom{
	typedef std::string Location;
	typedef std::string HASH;

	class NodeFrm{
	public:
		typedef std::shared_ptr<NodeFrm> POINTER;
		Location location_;
		POINTER children_[16];
		
		protocol::Node info_;
		
		bool modified_;
		bool leaf_deleted_;
		std::shared_ptr<std::string> leaf_;//nullptr default

		static int NEWCOUNT;
		static int DELCOUNT;
	public:
		NodeFrm(const Location& location);

		~NodeFrm();

		void SetValue(const std::string& v);
		void MarkRemove();
		void SetChild(int branch, POINTER child);
	};

	class Trie
	{

		bool SetItem(NodeFrm::POINTER node, const Location &key, const std::string &value, int depth);
		bool DeleteItem(NodeFrm::POINTER node, const Location& key);
		protocol::Child update_hash(NodeFrm::POINTER node);

		void Release(NodeFrm::POINTER node, int depth);
		
		void GetAllItem(const Location& node, const Location& location, std::vector<std::string>& result);
		void StorageAssociated(const Location& location, std::vector<std::string>& result);
	protected:
		NodeFrm::POINTER root_;
		HASH root_hash_;
		Location rootl ;
		NodeFrm::POINTER ChildMayFromDB(NodeFrm::POINTER node, int branch);

		virtual bool storage_load(const Location& location, protocol::Node& info) = 0;

		virtual void StorageSaveNode(NodeFrm::POINTER node) = 0;
		virtual void StorageSaveLeaf(NodeFrm::POINTER node) = 0;
		virtual	void StorageDeleteNode(NodeFrm::POINTER node) = 0;
		virtual void StorageDeleteLeaf(NodeFrm::POINTER node) = 0;

		virtual bool StorageGetLeaf(const Location& location, std::string& value) = 0;
		virtual std::string HashCrypto(const std::string& input) = 0;
		
		protocol::Node getNode(NodeFrm::POINTER node, const Location& location);
	public:
		static const char EVEN_PREFIX = 0x00;
		static const char ODD_PREFIX = 0x01;
		static const char LEAF_PREFIX = 0x02;

		Trie();
		~Trie();
		
		// static std::string  BinToHexString(const std::string &value, bool uppercase = false);

		//add or set a k/v
		bool Set(const std::string &key, const std::string &value);

		//Return false if it is not existed; otherwise, return true.
		bool Get(const std::string& key, std::string& value);

		bool Exists(NodeFrm::POINTER node, const Location& key);

		void GetAll(const std::string& key, std::vector<std::string>& values);

		//Return false if it is not existed; otherwise, return true.
		bool Delete(const std::string& key);

		HASH GetRootHash();

		void UpdateHash();

		void FreeMemory(int depth);
	
		protocol::Node GetNode(const Location& key);

	public:
		static Location CommonPrefix(const Location& s1, const Location& s2);
		static int NextBranch(const Location &s1, const Location& s2);
		static Location Key2Location(const std::string& key);
	};
}

#endif
