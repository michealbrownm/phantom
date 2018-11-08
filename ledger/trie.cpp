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
#include "utils/strings.h"
#include "trie.h"

namespace phantom{

	int NodeFrm::NEWCOUNT;
	int NodeFrm::DELCOUNT;
	/*
	-----------------------------
	old\new |  add  | mod  | del
	add      | false | add  | ignore
	mod	     | false | mod  | del
	del	     |  mod  | false| false
	-----------------------------
	*/

	NodeFrm::NodeFrm(const Location& location)
		:leaf_(nullptr),  /*indb_(false),leaf_indb_(false),*/ leaf_deleted_(false), modified_(true), location_(location){
		for (int i = 0; i <= 16; i++){
			protocol::Child* ch = info_.add_children();
			if (i != 16)
				children_[i] = nullptr;
		}
		NEWCOUNT++;
	}

	void NodeFrm::SetValue(const std::string& v){
		modified_ = true;
		leaf_deleted_ = false;
		leaf_ = std::make_shared<std::string>(v);
		protocol::Child* ch16 = info_.mutable_children(16);
		ch16->set_childtype(protocol::LEAF);
		ch16->set_sublocation(location_);
	}

	void NodeFrm::MarkRemove(){
		modified_ = true;
		leaf_deleted_ = true;
		leaf_ = nullptr;
		protocol::Child* ch16 = info_.mutable_children(16);
		ch16->Clear();
	}

	void NodeFrm::SetChild(int branch, POINTER child){
		assert(branch < 16);
		modified_ = true;
		children_[branch] = child;
		info_.mutable_children(branch)->set_sublocation(child->location_);
		//info_.mutable_children(branch)->set_childtype();
	}

	NodeFrm::~NodeFrm(){
		DELCOUNT++;
	}

	Trie::Trie(){
		rootl = "";
		rootl.push_back(0);
	}


	Trie::~Trie(){
		FreeMemory(0);
	}

	void Trie::FreeMemory(int depth){
		Release(root_, depth);
	}

	void Trie::Release(NodeFrm::POINTER node, int depth){
		for (int i = 0; i < 16; i++){
			auto child = node->children_[i];
			if (child != nullptr){
				Release(child, depth - 1);
				if (depth <= 0){
					node->children_[i] = nullptr;
				}
			}
		}
	}

	NodeFrm::POINTER Trie::ChildMayFromDB(NodeFrm::POINTER node, int branch) {
		if (node->children_[branch] == nullptr){
			NodeFrm::POINTER frm = nullptr;
			const protocol::Child& chd = node->info_.children(branch);
			if (chd.childtype() == protocol::NONE){
				return nullptr;
			}

			frm = std::make_shared<NodeFrm>(chd.sublocation());
			frm->modified_ = false;

			if (chd.childtype() == protocol::LEAF){
				frm->info_.mutable_children(16)->CopyFrom(node->info_.children(branch));

			}
			else if (chd.childtype() == protocol::INNER){
				if (!storage_load(chd.sublocation(), frm->info_)){
					PROCESS_EXIT("load:%s failed", utils::String::BinToHexString(chd.sublocation()).c_str());
				}
			}
			node->children_[branch] = frm;
		}
		return node->children_[branch];
	}


	Location Trie::CommonPrefix(const Location& s1, const Location& s2){
		Location out = "";
		char pre = EVEN_PREFIX;
		out.push_back(pre);
		for (std::size_t i = 1; i < s1.length() && i < s2.length(); i++){
			char c1 = s1.at(i);
			char c2 = s2.at(i);
			if ((c1 & 0xf0) != (c2 & 0xf0)){
				break;
			}

			if (i == s1.length() - 1 && s1.at(0) == ODD_PREFIX){
				char ch = c1 & 0xf0;
				out.push_back(ch);
				pre = ODD_PREFIX;
				break;
			}

			if (i == s2.length() - 1 && s2.at(0) == ODD_PREFIX){
				char ch = c1 & 0xf0;
				out.push_back(ch);
				pre = ODD_PREFIX;
				break;
			}

			if ((c1 & 0x0f) != (c2 & 0x0f)){
				char ch = c1 & 0xf0;
				out.push_back(ch);
				pre = ODD_PREFIX;
				break;
			}

			char ch = c1;
			out.push_back(ch);
		}
		out[0] = pre;
		return out;
	}


	int Trie::NextBranch(const Location& s1, const Location& s2){
		int len1 = s1.length();
		if (s1.at(0) == ODD_PREFIX){
			char ch = s2.at(len1 - 1);
			return ch & 0x0f;
		}
		else {
			char ch = s2.at(len1);
			return (ch >> 4) & 0x0f;
		}
	}

	Location Trie::Key2Location(const std::string& key){
		Location location = "";
		location.push_back(EVEN_PREFIX);
		return location + key;
	}

	protocol::Child Trie::update_hash(NodeFrm::POINTER node){

		int branch_count = 0;
		int onlybranch = -1;
		int64_t children_count = 0;

		//////////////////////////////////////////////////////////////
		if (!node->leaf_deleted_){
			if (node->leaf_ != nullptr){
				protocol::Child* this_child = node->info_.mutable_children(16);
#ifdef COUNT
				this_child->set_count(1);
#endif
				this_child->set_sublocation(node->location_);
				this_child->set_hash(HashCrypto(*(node->leaf_)));
				this_child->set_childtype(protocol::LEAF);
				StorageSaveLeaf(node);
			}
		}
		else{
			protocol::Child* ch = node->info_.mutable_children(16);
			ch->Clear();
			StorageDeleteLeaf(node);
		}

		if (node->info_.children(16).childtype() != protocol::CHILDTYPE::NONE){
			branch_count++;
			onlybranch = 16;
			children_count++;
		}

		for (int i = 0; i < 16; i++){
			NodeFrm::POINTER child = node->children_[i];
			if ((child != nullptr) && (child->modified_)){
				protocol::Child childresult = update_hash(child);
				node->info_.mutable_children(i)->CopyFrom(childresult);
			}

			if (node->info_.children(i).childtype() != protocol::CHILDTYPE::NONE){
				branch_count++;
				onlybranch = i;
#ifdef COUNT
				if (node->info_.children(i).count() == 0){
					printf("fatel error");
				}
				children_count += node->info_.children(i).count();
#endif
			}
		}


		protocol::Child result;
#ifdef COUNT
		result.set_count(children_count);
#endif		
		if (branch_count == 0 && node->location_ != rootl){
			StorageDeleteNode(node);
			//node->indb_ = false;
		}
		else if (branch_count == 1 && node->location_ != rootl){
			StorageDeleteNode(node);
			//node->indb_ = false;
			result.CopyFrom(node->info_.children(onlybranch));
		}
		else {
			StorageSaveNode(node);
			result.set_hash(HashCrypto(node->info_.SerializeAsString()));
			result.set_sublocation(node->location_);
			result.set_childtype(protocol::CHILDTYPE::INNER);
#ifdef COUNT
			result.set_count(children_count);
#endif
		}
		node->modified_ = false;
		return result;
	}

	bool Trie::SetItem(NodeFrm::POINTER node, const Location& location, const std::string &data, int depth){

		node->modified_ = true;
		Location location1 = node->location_;

		if (location1 == location){
			node->SetValue(data);
			return false;
		}

		Location common = CommonPrefix(location1, location);
		int branch = NextBranch(common, location);

		NodeFrm::POINTER node2 = ChildMayFromDB(node, branch);
		protocol::Child child2 = node->info_.children(branch);
		if (node2 == nullptr){
			NodeFrm::POINTER newnode = std::make_shared<NodeFrm>(location);
			newnode->SetValue(data);

			node->SetChild(branch, newnode);
			node->info_.mutable_children(branch)->set_childtype(protocol::LEAF);
			
			return true;
		}

		Location location2 = node2->location_;
		std::string newcommon = CommonPrefix(location, location2);
		if (newcommon == location2){
			return SetItem(node2, location, data, depth + 1);
		}

		if (newcommon == location){
			/*newcommon < node2.key_*/
			/*
				node1
				|
				|
				newnode
				|
				|
				node2
				*/
			NodeFrm::POINTER newnode = std::make_shared< NodeFrm>(location);
			newnode->SetValue(data);
			int b1 = NextBranch(newcommon, location2);
			newnode->SetChild(b1, node2);
			newnode->info_.mutable_children(b1)->CopyFrom(child2);

			node->SetChild(branch, newnode);
			node->info_.mutable_children(branch)->set_childtype(protocol::INNER);
			return true;
		}
		else {

			/************************************************************************/
			/*
						  node1
						  |
						  |
						  mnode
						  / \
						  /   \
						  node2  newnode

						  */
			/************************************************************************/

			NodeFrm::POINTER mnode = std::make_shared< NodeFrm>(newcommon);
			NodeFrm::POINTER newnode = std::make_shared< NodeFrm>(location);
			newnode->SetValue(data);

			int b1 = NextBranch(newcommon, location);
			int b2 = NextBranch(newcommon, location2);
			mnode->SetChild(b1, newnode);
			mnode->SetChild(b2, node2);
			mnode->info_.mutable_children(b2)->CopyFrom(child2);
			node->SetChild(branch, mnode);
			return true;
		}
	}

	bool Trie::DeleteItem(NodeFrm::POINTER node, const Location& location){

		Location location1 = node->location_;
		if (location1.length() > location.length()){
			return false;
		}

		if (location == location1){
			node->MarkRemove();
			return true;
		}

		Location common = CommonPrefix(location1, location);

		int branch = NextBranch(common, location);
		NodeFrm::POINTER node2 = ChildMayFromDB(node, branch);
		if (node2 == nullptr){
			return false;
		}

		Location location2 = node2->location_;
		Location common2 = CommonPrefix(location2, location);
		if (location2 != common2){
			return false;
		}
		bool ret = DeleteItem(node2, location);
		node->modified_ |= ret;
		return ret;
	}

	void Trie::GetAllItem(const Location& node, const Location& location, std::vector<std::string>& result){
		protocol::Node info;
		if (!storage_load(node, info)){
			return;
		}
		Location common = CommonPrefix(node, location);
		if (common == location){
			StorageAssociated(node, result);
			return;
		}

		if (common == node){
			int nextbranch = NextBranch(common, location);
			Location location2 = info.children(nextbranch).sublocation();
			GetAllItem(location2, location, result);
		}

	}

	bool Trie::Set(const std::string& key, const std::string &value){
		Location location = Key2Location(key);
		return SetItem(root_, location, value, 0);
	}

	bool Trie::Get(const std::string& key, std::string& value){
		Location location = Key2Location(key);
		if (!Exists(root_, location))
			return false;
		return	StorageGetLeaf(location, value);
	}

	bool Trie::Exists(NodeFrm::POINTER node, const Location& key) {

		if (node->location_ == key)
			return true;

		auto common = CommonPrefix(node->location_, key);
		int branch = NextBranch(common, key);

		if (node->info_.children(branch).childtype() == protocol::CHILDTYPE::NONE){
			return false;
		}

		Location location2 = node->info_.children(branch).sublocation();

		auto common2 = CommonPrefix(location2, key);
		if (common2 != location2){
			return false;
		}
		auto child = ChildMayFromDB(node, branch);
		return Exists(child, key);
	}

	void Trie::GetAll(const std::string& key, std::vector<std::string>& values){
		Location location = Key2Location(key);
		Location node = Key2Location("");
		GetAllItem(node, location, values);
	}


	HASH Trie::GetRootHash(){
		return root_hash_;
	}

	void Trie::UpdateHash(){
		root_hash_ = update_hash(root_).hash();
	}

	bool Trie::Delete(const std::string& key){
		Location location = Key2Location(key);
		return DeleteItem(root_, location);
	}


	void Trie::StorageAssociated(const Location& location, std::vector<std::string>& result){
		protocol::Node info;
		if (!storage_load(location, info)){
			return;
		}
		if (info.children(16).childtype() == protocol::CHILDTYPE::LEAF){
			std::string v;
			StorageGetLeaf(location, v);
			result.push_back(v);
		}

		for (int i = 0; i < 16; i++){
			protocol::Child chd = info.children(i);
			protocol::CHILDTYPE type = chd.childtype();
			switch (type)
			{
			case protocol::NONE:
				break;
			case protocol::INNER:
				StorageAssociated(chd.sublocation(), result);
				break;
			case protocol::LEAF:
				std::string value;
				StorageGetLeaf(chd.sublocation(), value);
				result.push_back(value);
				break;
			}
		}
	}


	///////////////////////////////////////////////////////
	//std::string Trie::ToJson(){
	//	return NodeToJson(root_).toStyledString();
	//}

	//std::string Trie::BinToHexString(const std::string &value, bool uppercase /* = false */){

	//	std::string result;
	//	result.resize(value.size() * 2);
	//	for (size_t i = 0; i < value.size(); i++){
	//		uint8_t item = value[i];
	//		uint8_t high = (item >> 4);
	//		uint8_t low = (item & 0x0F);
	//		if (uppercase) {
	//			result[2 * i] = (high >= 0 && high <= 9) ? (high + '0') : (high - 10 + 'A');
	//			result[2 * i + 1] = (low >= 0 && low <= 9) ? (low + '0') : (low - 10 + 'A');
	//		}
	//		else {
	//			result[2 * i] = (high >= 0 && high <= 9) ? (high + '0') : (high - 10 + 'a');
	//			result[2 * i + 1] = (low >= 0 && low <= 9) ? (low + '0') : (low - 10 + 'a');
	//		}
	//	}
	//	return result;
	//}

	protocol::Node Trie::GetNode(const Location& location)	{
		Location lc = location;
		if (lc == ""){
			lc.push_back(0);
		}
		return getNode(root_, lc);
	}

	protocol::Node Trie::getNode(NodeFrm::POINTER node, const Location& location){
		if (node->location_ == location){
			return node->info_;
		}

		Location common = CommonPrefix(location, node->location_);
		int branch = NextBranch(common, location);

		NodeFrm::POINTER frm = ChildMayFromDB(node, branch);
		if (frm != nullptr){
			return getNode(frm, location);
		}
		else
			return protocol::Node();
	}
}