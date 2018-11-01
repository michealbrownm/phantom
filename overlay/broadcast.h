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

#ifndef BROADCAST_H_
#define BROADCAST_H_

#include <unordered_map>
namespace phantom{

	class IBroadcastDriver{
	public:
		IBroadcastDriver(){};
		virtual ~IBroadcastDriver(){};

		//Virtual bool SendMessage(int64_t peer_id, WsMessagePointer msg) = 0;
		virtual bool SendRequest(int64_t peer_id, int64_t type, const std::string &data) = 0;
		virtual std::set<int64_t> GetActivePeerIds() = 0;
	};

	class BroadcastRecord{
	public:
		typedef std::shared_ptr<BroadcastRecord> pointer;

		BroadcastRecord(int64_t type, const std::string &data, int64_t);
		~BroadcastRecord();

		int64_t type_;
		int64_t time_stamp_;
		std::set<int64_t> peers_;
	};

	typedef std::map<int64_t, std::string> BroadcastRecordCoupleMap;
	typedef std::unordered_map<std::string, BroadcastRecord::pointer> BroadcastRecordMap;

	class Broadcast {
	private:
		BroadcastRecordCoupleMap    records_couple_;
		BroadcastRecordMap records_;
		utils::Mutex mutex_msg_sending_;
		IBroadcastDriver *driver_;

	public:
		Broadcast(IBroadcastDriver *driver);
		~Broadcast();

		bool Add(int64_t type, const std::string &data, int64_t peer_id);
		void Send(int64_t type, const std::string &data);
		bool IsQueued(int64_t type, const std::string &data);
		void OnTimer();
		size_t GetRecordSize() const { return records_.size(); };
	};
};

#endif
